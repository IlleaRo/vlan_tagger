#include "node.h"
#include "../logger/logger.h"
#include <stdlib.h>
#include <string.h>

Node* node_create(const char* name, NodeType type, node_process_func process, void* context)
{
    if (!name || !process)
    {
        return NULL;
    }

    Node* node = (Node*)calloc(1, sizeof(Node));
    if (!node)
    {
        return NULL;
    }

    strncpy(node->name, name, MAX_NODE_NAME - 1);
    node->name[MAX_NODE_NAME - 1] = '\0';
    node->type = type;
    node->output_queues = (Queue_t**)calloc(MAX_NODE_OUTPUTS, sizeof(Queue_t*));
    node->output_count = 0;
    node->process = process;
    node->context = context;

    if (!node->output_queues)
    {
        free(node);
        return NULL;
    }

    return node;
}

int node_add_output(Node* node, Queue_t* output_queue)
{
    if (!node || !output_queue || node->output_count >= MAX_NODE_OUTPUTS)
    {
        return -1;
    }

    node->output_queues[node->output_count++] = output_queue;
    return 0;
}

int node_set_input(Node* node, Queue_t* input_queue)
{
    if (!node || !input_queue)
    {
        return -1;
    }

    node->input_queue = input_queue;
    return 0;
}

int node_start(Node* node)
{
    if (!node || !node->process)
    {
        return -1;
    }

    if (pthread_create(&node->thread, NULL, node->process, node) != 0)
    {
        printL(ERROR, INITIATOR, "Failed to start node: %s", node->name);
        return -1;
    }

    printL(INFO, INITIATOR, "Node started: %s", node->name);
    return 0;
}

void node_stop(Node* node)
{
    if (!node)
    {
        return;
    }

    if (node->should_exit)
    {
        *node->should_exit = 1;
    }

    pthread_join(node->thread, NULL);
    printL(INFO, INITIATOR, "Node stopped: %s", node->name);
}

void node_destroy(Node* node)
{
    if (!node)
    {
        return;
    }

    if (node->output_queues)
    {
        free(node->output_queues);
    }

    free(node);
}

void node_send_to_outputs(Node* node, unsigned char* buffer, unsigned short size)
{
    if (!node || !buffer)
    {
        return;
    }

    for (int i = 0; i < node->output_count; i++)
    {
        if (node->output_queues[i])
        {
            if (push(node->output_queues[i], buffer, size) != size)
            {
                printL(WARNING, INITIATOR, "Node %s: failed to push to output %d", node->name, i);
            }
        }
    }
}
