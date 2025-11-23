#include "packet_counter_node.h"
#include "../logger/logger.h"
#include "../common.h"
#include <stdlib.h>
#include <string.h>

Node* packet_counter_node_create(const char* name, int log_interval)
{
    if (!name || log_interval <= 0)
    {
        return NULL;
    }

    PacketCounterContext* ctx = (PacketCounterContext*)calloc(1, sizeof(PacketCounterContext));
    if (!ctx)
    {
        return NULL;
    }

    ctx->log_interval = log_interval;

    Node* node = node_create(name, NODE_TYPE_CUSTOM, packet_counter_node_process, ctx);
    if (!node)
    {
        free(ctx);
        return NULL;
    }

    return node;
}

static int is_vlan_packet(unsigned char* buffer, ssize_t size)
{
    if (size < 16)
    {
        return 0;
    }

    // Проверяем TPID (0x8100 для 802.1Q VLAN)
    unsigned short tpid = (buffer[12] << 8) | buffer[13];
    return (tpid == 0x8100);
}

void* packet_counter_node_process(void* node_ptr)
{
    if (!node_ptr)
    {
        return NULL;
    }

    Node* node = (Node*)node_ptr;
    PacketCounterContext* ctx = (PacketCounterContext*)node->context;

    if (!ctx)
    {
        printL(ERROR, INITIATOR, "%s: Invalid context", node->name);
        return NULL;
    }

    unsigned char buffer[ETHERNET_FRAME_LENGTH] = {0};
    ssize_t packet_size = 0;

    printL(INFO, INITIATOR, "PacketCounter node started: %s (log every %d packets)",
           node->name, ctx->log_interval);

    while (!node->should_exit || !*node->should_exit)
    {
        if (!node->input_queue)
        {
            printL(ERROR, INITIATOR, "%s: No input queue", node->name);
            break;
        }

        packet_size = pop(node->input_queue, buffer);

        if (packet_size < 1)
        {
            if (packet_size == 0)
            {
                continue;
            }
            printL(ERROR, INITIATOR, "%s: pop error", node->name);
            break;
        }

        // Обновляем статистику
        ctx->total_packets++;
        ctx->total_bytes += packet_size;

        if (is_vlan_packet(buffer, packet_size))
        {
            ctx->vlan_packets++;
        }
        else
        {
            ctx->non_vlan_packets++;
        }

        // Логируем статистику каждые N пакетов
        if (ctx->total_packets % ctx->log_interval == 0)
        {
            printL(INFO, INITIATOR,
                   "%s: Total=%lu pkts (%lu bytes), VLAN=%lu, Non-VLAN=%lu, Avg size=%lu bytes",
                   node->name,
                   ctx->total_packets,
                   ctx->total_bytes,
                   ctx->vlan_packets,
                   ctx->non_vlan_packets,
                   ctx->total_packets > 0 ? ctx->total_bytes / ctx->total_packets : 0);
        }

        // Отправляем дальше по pipeline
        node_send_to_outputs(node, buffer, packet_size);
    }

    // Финальная статистика
    printL(INFO, INITIATOR,
           "%s: FINAL STATS - Total=%lu pkts (%lu bytes), VLAN=%lu, Non-VLAN=%lu",
           node->name,
           ctx->total_packets,
           ctx->total_bytes,
           ctx->vlan_packets,
           ctx->non_vlan_packets);

    printL(INFO, INITIATOR, "PacketCounter node stopped: %s", node->name);
    return NULL;
}
