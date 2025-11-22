#include "pipeline.h"
#include "../logger/logger.h"
#include <stdlib.h>
#include <string.h>

Pipeline* pipeline_create(void)
{
    Pipeline* pipeline = (Pipeline*)malloc(sizeof(Pipeline));
    if (!pipeline)
    {
        return NULL;
    }

    pipeline->nodes = (Node**)calloc(MAX_PIPELINE_NODES, sizeof(Node*));
    pipeline->queues = (Queue_t**)calloc(MAX_PIPELINE_QUEUES, sizeof(Queue_t*));

    if (!pipeline->nodes || !pipeline->queues)
    {
        free(pipeline->nodes);
        free(pipeline->queues);
        free(pipeline);
        return NULL;
    }

    pipeline->node_count = 0;
    pipeline->queue_count = 0;
    pipeline->should_exit = 0;

    return pipeline;
}

int pipeline_add_node(Pipeline* pipeline, Node* node)
{
    if (!pipeline || !node || pipeline->node_count >= MAX_PIPELINE_NODES)
    {
        return -1;
    }

    pipeline->nodes[pipeline->node_count++] = node;
    node->should_exit = &pipeline->should_exit;

    printL(INFO, INITIATOR, "Node added to pipeline: %s", node->name);
    return 0;
}

Queue_t* pipeline_create_queue(Pipeline* pipeline)
{
    if (!pipeline || pipeline->queue_count >= MAX_PIPELINE_QUEUES)
    {
        return NULL;
    }

    Queue_t* queue = (Queue_t*)malloc(sizeof(Queue_t));
    if (!queue)
    {
        return NULL;
    }

    if (init(queue) != 0)
    {
        free(queue);
        return NULL;
    }

    pipeline->queues[pipeline->queue_count++] = queue;
    return queue;
}

int pipeline_connect(Pipeline* pipeline, Node* from, Node* to)
{
    if (!pipeline || !from || !to)
    {
        return -1;
    }

    Queue_t* queue = pipeline_create_queue(pipeline);
    if (!queue)
    {
        printL(ERROR, INITIATOR, "Failed to create queue for connection");
        return -1;
    }

    if (node_add_output(from, queue) != 0)
    {
        printL(ERROR, INITIATOR, "Failed to add output to node: %s", from->name);
        return -1;
    }

    if (node_set_input(to, queue) != 0)
    {
        printL(ERROR, INITIATOR, "Failed to set input for node: %s", to->name);
        return -1;
    }

    printL(INFO, INITIATOR, "Connected: %s -> %s", from->name, to->name);
    return 0;
}

int pipeline_start(Pipeline* pipeline)
{
    if (!pipeline)
    {
        return -1;
    }

    printL(INFO, INITIATOR, "Starting pipeline with %d nodes", pipeline->node_count);

    for (int i = 0; i < pipeline->node_count; i++)
    {
        if (node_start(pipeline->nodes[i]) != 0)
        {
            printL(ERROR, INITIATOR, "Failed to start node: %s", pipeline->nodes[i]->name);
            return -1;
        }
    }

    printL(INFO, INITIATOR, "Pipeline started successfully");
    return 0;
}

void pipeline_stop(Pipeline* pipeline)
{
    if (!pipeline)
    {
        return;
    }

    printL(INFO, INITIATOR, "Stopping pipeline");
    pipeline->should_exit = 1;

    for (int i = 0; i < pipeline->queue_count; i++)
    {
        if (pipeline->queues[i])
        {
            send_signal_queue(pipeline->queues[i]);
        }
    }

    for (int i = 0; i < pipeline->node_count; i++)
    {
        if (pipeline->nodes[i])
        {
            node_stop(pipeline->nodes[i]);
        }
    }

    printL(INFO, INITIATOR, "Pipeline stopped");
}

void pipeline_destroy(Pipeline* pipeline)
{
    if (!pipeline)
    {
        return;
    }

    for (int i = 0; i < pipeline->queue_count; i++)
    {
        if (pipeline->queues[i])
        {
            queue_destroy(pipeline->queues[i]);
            free(pipeline->queues[i]);
        }
    }

    for (int i = 0; i < pipeline->node_count; i++)
    {
        if (pipeline->nodes[i])
        {
            node_destroy(pipeline->nodes[i]);
        }
    }

    free(pipeline->nodes);
    free(pipeline->queues);
    free(pipeline);

    printL(INFO, INITIATOR, "Pipeline destroyed");
}
