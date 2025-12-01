#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/resource.h>
#include <errno.h>

#include "pipeline/pipeline.h"
#include "nodes/sniffer_node.h"
#include "nodes/tagger_node.h"
#include "nodes/sender_node.h"
#include "nodes/packet_counter_node.h"
#include "logger/logger.h"
#include "config_parser/config_parser.h"

static Pipeline* global_pipeline = NULL;
static int sock_r = -1;
static tag_rules_t* global_tag_rules = NULL;

static void signal_handler(int signal)
{
    printL(INFO, INITIATOR, "Received signal %d, stopping pipeline", signal);

    if (global_pipeline)
    {
        pipeline_stop(global_pipeline);
    }
}

static void cleanup(void)
{
    if (global_pipeline)
    {
        pipeline_destroy(global_pipeline);
        global_pipeline = NULL;
    }

    if (sock_r >= 0)
    {
        close(sock_r);
        printL(INFO, INITIATOR, "Socket closed.");
    }

    if (global_tag_rules)
    {
        tag_rules_clear(&global_tag_rules);
        printL(INFO, INITIATOR, "Tag rules cleared.");
    }

    stop_log();
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: <name of the network interface>\n");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if (pid < 0)
    {
        perror("FORK");
        exit(EXIT_FAILURE);
    }

    if (pid > 0)
    {
        printf("Daemon started with PID: %d\n", pid);
        exit(EXIT_SUCCESS);
    }

    if (setsid() < 0)
    {
        perror("SETSID");
        exit(EXIT_FAILURE);
    }

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    struct rlimit rlim;
    rlim.rlim_cur = 8 * 1024 * 1024;
    rlim.rlim_max = 8 * 1024 * 1024;

    start_log();

    if (setrlimit(RLIMIT_STACK, &rlim) != 0)
    {
        printL(ERROR, INITIATOR, "Error setting stack size limit");
        cleanup();
        exit(EXIT_FAILURE);
    }

    if (signal(SIGINT, signal_handler) == SIG_ERR ||
        signal(SIGSEGV, signal_handler) == SIG_ERR ||
        signal(SIGTERM, signal_handler) == SIG_ERR)
    {
        printL(ERROR, INITIATOR, "Signal processing error");
        cleanup();
        exit(EXIT_FAILURE);
    }

    sock_r = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sock_r < 0)
    {
        printL(ERROR, INITIATOR, "Socket opening error (error code: %d)!", errno);
        cleanup();
        exit(EXIT_FAILURE);
    }

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, argv[1], sizeof(ifr.ifr_name));

    if (ioctl(sock_r, SIOCGIFINDEX, &ifr) == -1)
    {
        printL(ERROR, INITIATOR, "Error with interface id");
        cleanup();
        exit(EXIT_FAILURE);
    }

    struct sockaddr_ll sa;
    memset(&sa, 0, sizeof(sa));
    sa.sll_family = AF_PACKET;
    sa.sll_protocol = htons(ETH_P_ALL);
    sa.sll_ifindex = ifr.ifr_ifindex;

    if (bind(sock_r, (struct sockaddr *)&sa, sizeof(sa)) == -1)
    {
        printL(ERROR, INITIATOR, "Error setting up network interface for socket (error code: %d)!", errno);
        cleanup();
        exit(EXIT_FAILURE);
    }

    struct sockaddr_ll saddr = {0};
    saddr.sll_family = AF_PACKET;
    saddr.sll_protocol = htons(ETH_P_ALL);
    int saddr_len = sizeof(saddr);

    if (config_file_check() != 0)
    {
        printL(ERROR, PARSER, "Error opening/closing config file!");
        cleanup();
        exit(EXIT_FAILURE);
    }

    if (tag_rules_init(&global_tag_rules, 64) != 0)
    {
        printL(ERROR, PARSER, "Error allocating memory for tag rules!");
        cleanup();
        exit(EXIT_FAILURE);
    }

    int size = config_file_read(global_tag_rules, 64);
    if (size < 0)
    {
        printL(ERROR, PARSER, "Error reading config file!");
        cleanup();
        exit(EXIT_FAILURE);
    }

    if (tag_rules_check_collisions((const tag_rules_t *)global_tag_rules, size) != 0)
    {
        printL(ERROR, PARSER, "Error checking config file for collisions!");
        cleanup();
        exit(EXIT_FAILURE);
    }

    tag_rules_convert_to_host_order(global_tag_rules, size);

    global_pipeline = pipeline_create();
    if (!global_pipeline)
    {
        printL(ERROR, INITIATOR, "Failed to create pipeline");
        cleanup();
        exit(EXIT_FAILURE);
    }

    // Создаем узлы для сложного pipeline
    Node* sniffer = sniffer_node_create("Sniffer", &sock_r, &saddr, &saddr_len);
    Node* tagger1 = tagger_node_create("Tagger1", global_tag_rules, size);
    Node* tagger2 = tagger_node_create("Tagger2", global_tag_rules, size);
    Node* counter = packet_counter_node_create("PacketCounter", 10);  // Логировать каждые 10 пакетов
    Node* sender = sender_node_create("Sender", &sock_r, &saddr, &saddr_len);

    if (!sniffer || !tagger1 || !tagger2 || !counter || !sender)
    {
        printL(ERROR, INITIATOR, "Failed to create nodes");
        cleanup();
        exit(EXIT_FAILURE);
    }

    // Добавляем все узлы в pipeline
    pipeline_add_node(global_pipeline, sniffer);
    pipeline_add_node(global_pipeline, tagger1);
    pipeline_add_node(global_pipeline, tagger2);
    pipeline_add_node(global_pipeline, counter);
    pipeline_add_node(global_pipeline, sender);

    // Соединяем узлы:
    // Sniffer -> [Tagger1, Tagger2] (параллельно)
    pipeline_connect(global_pipeline, sniffer, tagger1);
    pipeline_connect(global_pipeline, sniffer, tagger2);

    // [Tagger1, Tagger2] -> PacketCounter
    pipeline_connect(global_pipeline, tagger1, counter);
    pipeline_connect(global_pipeline, tagger2, counter);

    // PacketCounter -> Sender
    pipeline_connect(global_pipeline, counter, sender);

    printL(INFO, INITIATOR, "Pipeline configured:");
    printL(INFO, INITIATOR, "  Sniffer -> [Tagger1, Tagger2] -> PacketCounter -> Sender");

    if (pipeline_start(global_pipeline) != 0)
    {
        printL(ERROR, INITIATOR, "Failed to start pipeline");
        cleanup();
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < global_pipeline->node_count; i++)
    {
        pthread_join(global_pipeline->nodes[i]->thread, NULL);
    }

    cleanup();
    exit(EXIT_SUCCESS);
}
