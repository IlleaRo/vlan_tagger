#include "tagger_node.h"

#include <stdio.h>

#include "../logger/logger.h"
#include "../common.h"
#include <stdlib.h>
#include <string.h>
#include <linux/if_ether.h>

#ifndef VLAN_VID_MASK
#define VLAN_VID_MASK 0x0fff   /* VLAN ID: 12 бит */
#endif

#ifndef VLAN_HLEN
#define VLAN_HLEN 4            /* (TPID + TCI) */
#endif
#define IP_HEADER_MIN_LEN 20

Node *tagger_node_create(const char *name, tag_rules_t *tag_rules, const int tag_rules_size) {
    if (!name || !tag_rules || tag_rules_size <= 0) {
        return NULL;
    }

    TaggerContext *ctx = calloc(1, sizeof(TaggerContext));
    if (!ctx) {
        return NULL;
    }

    ctx->tag_rules = tag_rules;
    ctx->tag_rules_size = tag_rules_size;

    Node *node = node_create(name, NODE_TYPE_TAGGER, tagger_node_process, ctx);
    if (!node) {
        free(ctx);
        return NULL;
    }

    return node;
}

static int tagger_node_get_tag(const uint32_t addr, const tag_rules_t *tag_rules_obj, const int num_rules) {
    for (int i = 0; i < num_rules; ++i) {
        if (tag_rules_obj[i].ip_left.s_addr <= addr && tag_rules_obj[i].ip_right.s_addr >= addr) {
            return tag_rules_obj[i].tag;
        }
    }

    return 0;
}

static uint8_t *tagger_node_get_ip_offset(uint8_t *buffer) {
    constexpr uint16_t offset = ETH_HLEN;

    uint16_t ether_type;
    memcpy(&ether_type, buffer + 12, sizeof(ether_type));
    ether_type = ntohs(ether_type);

    if (ether_type <= ETH_DATA_LEN) {
        // Novell raw IEEE 802.3 non-standard variation frame
        return NULL;
    }

    switch (ether_type) {
        case ETH_P_IPV6: // Doesn't support
        case ETH_P_8021Q: // Doesn't support
            // offset += 4;
            return NULL;
        case ETH_P_IP:
            return buffer + offset;
        default:
            return NULL;
    }
}

static uint32_t tagger_node_get_ip(uint8_t *ip_buffer) {
    const uint8_t *ip_offset = tagger_node_get_ip_offset(ip_buffer);

    if (!ip_offset) {
        return 0;
    }

    const uint8_t *dst_addr = ip_offset + 16;
    uint32_t addr;

    memcpy(&addr, dst_addr, sizeof(addr));

    return ntohl(addr);
}

static uint16_t tagger_node_edit_packet(const uint8_t *input_buffer,
                                        uint8_t *output_buffer,
                                        const uint16_t vlan_id,
                                        const uint16_t packet_size)
{
    // 1. Copy MACs (6 + 6 = 12)
    memcpy(output_buffer, input_buffer, 12);

    // 2. TPID = 802.1Q VLAN
    const uint16_t tpid = htons(ETH_P_8021Q);
    memcpy(output_buffer + 12, &tpid, sizeof(tpid));

    // 3. TCI: PCP=0, DEI=0, VLAN ID = vlan_id & 0x0FFF
    const uint16_t tci = htons(vlan_id & VLAN_VID_MASK);
    memcpy(output_buffer + 14, &tci, sizeof(tci));

    // 4. Other data
    memcpy(output_buffer + 16, input_buffer + 12, packet_size - 12);

    // 5. Добавили 4 байта VLAN-тега
    return packet_size + VLAN_HLEN;
}

void *tagger_node_process(void *node_ptr) {
    if (!node_ptr) {
        return NULL;
    }

    Node *node = node_ptr;
    TaggerContext *ctx = node->context;

    if (!ctx || !ctx->tag_rules || ctx->tag_rules_size <= 0) {
        printL(ERROR, TAGGER, "%s: Invalid context", node->name);
        return NULL;
    }

    uint8_t buffer[ETHERNET_FRAME_LENGTH];
    uint8_t output_buffer[ETHERNET_FRAME_LENGTH];
    uint32_t addr = 0;
    ssize_t packet_size = 0;

    printL(INFO, TAGGER, "Tagger node started: %s", node->name);


    while (!node->should_exit || !*node->should_exit) {
        if (!node->input_queue) {
            printL(ERROR, TAGGER, "%s: No input queue", node->name);
            break;
        }

        packet_size = pop(node->input_queue, buffer);

        if (packet_size < 1) {
            if (packet_size == 0) {
                continue;
            }

            printL(ERROR, TAGGER, "%s: pop error", node->name);
            break;
        }

        if (packet_size < ETH_HLEN + IP_HEADER_MIN_LEN) {
            continue;  // The packet is too short
        }

        addr = tagger_node_get_ip(buffer);

        if (!addr) {
            continue;  // Can't get IPv4
        }

        const int tag = tagger_node_get_tag(addr, ctx->tag_rules, ctx->tag_rules_size);

        if (tag == 0) {
            continue;  // No tag
        }

        packet_size = tagger_node_edit_packet(buffer, output_buffer, tag, packet_size);
        node_send_to_outputs(node, output_buffer, packet_size);
    }

    printL(INFO, TAGGER, "Tagger node stopped: %s", node->name);
    return NULL;
}
