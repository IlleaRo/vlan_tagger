#include "../include/dispatcher_node.h"
#include "../../logger/logger.h"
#include "../../common.h"
#include <stdlib.h>
#include <time.h>

Node* dispatcher_node_create(const char* name, DispatchStrategy strategy)
{
    if (!name)
    {
        return NULL;
    }

    DispatcherContext* ctx = (DispatcherContext*)calloc(1, sizeof(DispatcherContext));
    if (!ctx)
    {
        return NULL;
    }

    ctx->strategy = strategy;
    ctx->next_index = 0;
    ctx->packets_sent = 0;

    Node* node = node_create(name, NODE_TYPE_CUSTOM, dispatcher_node_process, ctx);
    if (!node)
    {
        free(ctx);
        return NULL;
    }

    return node;
}

void* dispatcher_node_process(void* node_ptr)
{
    if (!node_ptr)
    {
        return NULL;
    }

    Node* node = (Node*)node_ptr;
    DispatcherContext* ctx = (DispatcherContext*)node->context;

    if (!ctx)
    {
        printL(ERROR, INITIATOR, "%s: Invalid context", node->name);
        return NULL;
    }

    unsigned char buffer[ETHERNET_FRAME_LENGTH] = {0};
    ssize_t packet_size = 0;

    const char* strategy_name = (ctx->strategy == DISPATCH_ROUND_ROBIN) ? "Round-Robin" : "Random";
    printL(INFO, INITIATOR, "Dispatcher node started: %s (strategy: %s)", node->name, strategy_name);

    // Инициализация для случайного распределения
    if (ctx->strategy == DISPATCH_RANDOM)
    {
        srand(time(NULL));
    }

    while (!node->should_exit || !*node->should_exit)
    {
        if (!node->input_queue)
        {
            printL(ERROR, INITIATOR, "%s: No input queue", node->name);
            break;
        }

        if (node->output_count == 0)
        {
            printL(ERROR, INITIATOR, "%s: No output queues configured", node->name);
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

        // Выбираем целевую очередь в зависимости от стратегии
        int target_index = 0;

        switch (ctx->strategy)
        {
            case DISPATCH_ROUND_ROBIN:
                target_index = ctx->next_index % node->output_count;
                ctx->next_index = (ctx->next_index + 1) % node->output_count;
                break;

            case DISPATCH_RANDOM:
                target_index = rand() % node->output_count;
                break;

            default:
                target_index = 0;
        }

        // Отправляем в выбранную очередь
        if (node->output_queues[target_index])
        {
            if (push(node->output_queues[target_index], buffer, packet_size) != packet_size)
            {
                printL(WARNING, INITIATOR, "%s: failed to push to output %d", node->name, target_index);
            }
            else
            {
                ctx->packets_sent++;
            }
        }
    }

    printL(INFO, INITIATOR, "Dispatcher node stopped: %s (total packets: %lu)",
           node->name, ctx->packets_sent);
    return NULL;
}
