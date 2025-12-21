#include <arpa/inet.h>
#include <linux/if_ether.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "tagger_node.h"
#include "node.h"
#include "queue.h"
#include "logger.h"
#include "common.h"

extern const char *FILE_LOG_NAME;

static void assert_true(const int condition, const char *message) {
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(EXIT_FAILURE);
    }
}

static size_t build_ipv4_packet(uint8_t *buffer, const char *dst_ip) {
    memset(buffer, 0, ETHERNET_FRAME_LENGTH);

    const uint8_t dst_mac[ETH_ALEN] = {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
    const uint8_t src_mac[ETH_ALEN] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
    memcpy(buffer, dst_mac, ETH_ALEN);
    memcpy(buffer + ETH_ALEN, src_mac, ETH_ALEN);

    const uint16_t eth_type = htons(ETH_P_IP);
    memcpy(buffer + 12, &eth_type, sizeof(eth_type));

    const size_t ip_offset = ETH_HLEN;
    buffer[ip_offset] = 0x45; // Version=4, IHL=5
    buffer[ip_offset + 8] = 64; // TTL
    buffer[ip_offset + 9] = 6; // Protocol = TCP

    struct in_addr dst_addr = {0};
    assert_true(inet_aton(dst_ip, &dst_addr) != 0, "inet_aton for dst_ip");
    memcpy(buffer + ip_offset + 16, &dst_addr.s_addr, sizeof(dst_addr.s_addr));

    const uint16_t total_len = htons(20 + 4);
    memcpy(buffer + ip_offset + 2, &total_len, sizeof(total_len));

    return ETH_HLEN + 24;
}

static void prepare_dummy_rule(tag_rule_t *rule) {
    assert_true(inet_aton("192.168.1.0", &rule->ip_left) != 0, "inet_aton left");
    assert_true(inet_aton("192.168.1.255", &rule->ip_right) != 0, "inet_aton right");
    rule->tag = 100;
}

static Node *init_dummy_tagger_node(const tag_rule_t *rule, int *pexit_flag, Queue_t *input_queue, Queue_t *output_queue) {
    Node *res = tagger_node_create("TaggerTest", rule, 1);

    if (!res) {
        return NULL;
    }

    assert_true(init(input_queue) == 0, "init input queue");
    assert_true(init(output_queue) == 0, "init output queue");

    assert_true(node_set_input(res, input_queue) == 0, "set input queue");
    assert_true(node_add_output(res, output_queue) == 0, "add output queue");

    *pexit_flag = 0;
    res->should_exit = pexit_flag;

    assert_true(node_start(res) == 0, "start tagger thread");

    return res;
}

static void analyze_packet(const uint8_t *packet, const tag_rule_t *rule) {
    uint16_t tpid;
    memcpy(&tpid, packet + 12, sizeof(tpid));
    assert_true(ntohs(tpid) == ETH_P_8021Q, "TPID is 0x8100");

    uint16_t tci;
    memcpy(&tci, packet + 14, sizeof(tci));
    assert_true((ntohs(tci) & 0x0fff) == rule->tag, "TCI carries VLAN ID");

    uint16_t inner_type;
    memcpy(&inner_type, packet + 16, sizeof(inner_type));
    assert_true(ntohs(inner_type) == ETH_P_IP, "encapsulated EtherType is IPv4");

}

int main(void) {
    // LOG
    unlink(FILE_LOG_NAME);
    assert_true(start_log() == 0, "start_log succeeds");

    // Init rules
    tag_rule_t rule = {0};
    prepare_dummy_rule(&rule);

    // Create and init tagger node
    Queue_t input_queue;  // Memset will be later
    Queue_t output_queue;
    int exit_flag = 0;
    Node *tagger = init_dummy_tagger_node(&rule, &exit_flag, &input_queue, &output_queue);
    assert_true(tagger != NULL, "tagger_node_create returns node");

    // Create a fake packet
    uint8_t packet[ETHERNET_FRAME_LENGTH];
    const size_t packet_size = build_ipv4_packet(packet, "192.168.1.10");

    // Put fake packet in input queue
    assert_true(push(&input_queue, packet, (uint16_t) packet_size) == (ssize_t) packet_size, "push test packet");

    // get tagger result
    uint8_t tagged_packet[ETHERNET_FRAME_LENGTH];
    ssize_t tagged_size = pop(&output_queue, tagged_packet);
    assert_true(tagged_size == (ssize_t) (packet_size + 4), "tagged packet size increased by 4");

    // Analyze result
    analyze_packet(tagged_packet, &rule);


    // Stop tagger node
    exit_flag = 1;
    uint8_t dummy = 0;
    push(&input_queue, &dummy, 1);
    node_stop(tagger);

    // Destroy tagger node
    assert_true(queue_destroy(&input_queue) == 0, "destroy input queue");
    assert_true(queue_destroy(&output_queue) == 0, "destroy output queue");
    node_destroy(tagger);
    assert_true(stop_log() == 0, "stop_log succeeds");

    printf("All tagger_node tests passed.\n");
    return EXIT_SUCCESS;
}
