#include "../include/logger_node.h"
#include "../../logger/logger.h"
#include "../../common.h"
#include <stdlib.h>
#include <stdio.h>

Node* logger_node_create(const char* name, int log_interval)
{
    if (!name)
    {
        return NULL;
    }

    LoggerContext* ctx = (LoggerContext*)calloc(1, sizeof(LoggerContext));
    if (!ctx)
    {
        return NULL;
    }

    ctx->log_interval = log_interval;
    ctx->packet_count = 0;

    Node* node = node_create(name, NODE_TYPE_CUSTOM, logger_node_process, ctx);
    if (!node)
    {
        free(ctx);
        return NULL;
    }

    return node;
}

static void log_packet_hex(const char* node_name, unsigned char* buffer, ssize_t size, unsigned long pkt_num)
{
    char full_dump[8192];
    int total_offset = 0;
    int n = 0;
    int truncated = 0;

    // Формируем заголовок
    n = snprintf(full_dump + total_offset, sizeof(full_dump) - total_offset,
                 "=== %s: Packet #%lu (%zd bytes) ===\n",
                 node_name, pkt_num, size);
    if (n < 0 || n >= (int)(sizeof(full_dump) - total_offset)) {
        truncated = 1;
        goto truncated_end;
    }
    total_offset += n;

    // Формируем hex dump построчно
    for (ssize_t i = 0; i < size; i += 16)
    {
        n = snprintf(full_dump + total_offset, sizeof(full_dump) - total_offset,
                     "%04zx: ", i);
        if (n < 0 || n >= (int)(sizeof(full_dump) - total_offset)) {
            truncated = 1;
            goto truncated_end;
        }
        total_offset += n;

        // Hex bytes
        for (int j = 0; j < 16; j++)
        {
            if (i + j < size)
            {
                n = snprintf(full_dump + total_offset, sizeof(full_dump) - total_offset,
                             "%02x ", buffer[i + j]);
            }
            else
            {
                n = snprintf(full_dump + total_offset, sizeof(full_dump) - total_offset,
                             "   ");
            }
            if (n < 0 || n >= (int)(sizeof(full_dump) - total_offset)) {
                truncated = 1;
                goto truncated_end;
            }
            total_offset += n;
        }

        // ASCII representation
        n = snprintf(full_dump + total_offset, sizeof(full_dump) - total_offset, " |");
        if (n < 0 || n >= (int)(sizeof(full_dump) - total_offset)) {
            truncated = 1;
            goto truncated_end;
        }
        total_offset += n;
        for (int j = 0; j < 16 && i + j < size; j++)
        {
            unsigned char c = buffer[i + j];
            n = snprintf(full_dump + total_offset, sizeof(full_dump) - total_offset,
                         "%c", (c >= 32 && c <= 126) ? c : '.');
            if (n < 0 || n >= (int)(sizeof(full_dump) - total_offset)) {
                truncated = 1;
                goto truncated_end;
            }
            total_offset += n;
        }
        n = snprintf(full_dump + total_offset, sizeof(full_dump) - total_offset, "|");
        if (n < 0 || n >= (int)(sizeof(full_dump) - total_offset)) {
            truncated = 1;
            goto truncated_end;
        }
        total_offset += n;

        if (i + 16 < size)
        {
            n = snprintf(full_dump + total_offset, sizeof(full_dump) - total_offset, "\n");
            if (n < 0 || n >= (int)(sizeof(full_dump) - total_offset)) {
                truncated = 1;
                goto truncated_end;
            }
            total_offset += n;
        }
    }

truncated_end:
    if (truncated && total_offset < (int)sizeof(full_dump)) {
        // Optionally append a truncation notice
        snprintf(full_dump + total_offset, sizeof(full_dump) - total_offset, "\n...(truncated)...\n");
    }
    printL(INFO, INITIATOR, "%s", full_dump);
}

void* logger_node_process(void* node_ptr)
{
    if (!node_ptr)
    {
        return NULL;
    }

    Node* node = (Node*)node_ptr;
    LoggerContext* ctx = (LoggerContext*)node->context;

    if (!ctx)
    {
        printL(ERROR, INITIATOR, "%s: Invalid context", node->name);
        return NULL;
    }

    unsigned char buffer[ETHERNET_FRAME_LENGTH] = {0};
    ssize_t packet_size = 0;

    printL(INFO, INITIATOR, "Logger node started: %s (log interval: %d)",
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

        ctx->packet_count++;

        // Логируем если интервал <= 1 (все пакеты) или каждый N-й
        if (ctx->log_interval <= 1 || ctx->packet_count % ctx->log_interval == 0)
        {
            log_packet_hex(node->name, buffer, packet_size, ctx->packet_count);
        }

        // Отправляем дальше по pipeline
        node_send_to_outputs(node, buffer, packet_size);
    }

    printL(INFO, INITIATOR, "Logger node stopped: %s (logged %lu packets)",
           node->name, ctx->packet_count);
    return NULL;
}
