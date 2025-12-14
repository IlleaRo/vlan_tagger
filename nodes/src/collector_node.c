#include "../include/collector_node.h"
#include "../../logger/logger.h"
#include "../../common.h"
#include <stdlib.h>

Node* collector_node_create(const char* name)
{
    if (!name)
    {
        return NULL;
    }

    Node* node = node_create(name, NODE_TYPE_CUSTOM, collector_node_process, NULL);
    if (!node)
    {
        return NULL;
    }

    return node;
}

void* collector_node_process(void* node_ptr)
{
    if (!node_ptr)
    {
        return NULL;
    }

    Node* node = (Node*)node_ptr;
    unsigned char buffer[ETHERNET_FRAME_LENGTH] = {0};
    ssize_t packet_size = 0;

    printL(INFO, INITIATOR, "Collector node started: %s", node->name);

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

        // Просто пробрасываем данные дальше без обработки
        node_send_to_outputs(node, buffer, packet_size);
    }

    printL(INFO, INITIATOR, "Collector node stopped: %s", node->name);
    return NULL;
}
