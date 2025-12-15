#include "pipeline.h"
#include "../logger/logger.h"
#include "../nodes/include/dispatcher_node.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int pipeline_init(Pipeline *pipeline_ptr)
{
    pipeline_ptr->nodes = (Node**)calloc(MAX_PIPELINE_NODES, sizeof(Node*));
    pipeline_ptr->queues = (Queue_t**)calloc(MAX_PIPELINE_QUEUES, sizeof(Queue_t*));

    if (!pipeline_ptr->nodes || !pipeline_ptr->queues)
    {
        free(pipeline_ptr->nodes);
        free(pipeline_ptr->queues);

        return 1;
    }

    pipeline_ptr->node_count = 0;
    pipeline_ptr->queue_count = 0;
    pipeline_ptr->should_exit = 0;

    return 0;
}

int pipeline_build(Pipeline* pipeline, const pipeline_pattern_builder_cb cb, const void* ctx)
{
    if (!cb) {
        return 1;
    }

    if (cb(pipeline, ctx)) {
        return 1;
    }

    return 0;
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

    Queue_t* queue;

    // Если у target ноды уже есть input_queue, переиспользуем её (чтобы не плодить новых очередей)
    if (to->input_queue != NULL)
    {
        queue = to->input_queue;
        printL(INFO, INITIATOR, "Reusing existing queue for: %s -> %s", from->name, to->name);
    }
    else
    {
        // Создаем новую очередь
        queue = pipeline_create_queue(pipeline);
        if (!queue)
        {
            printL(ERROR, INITIATOR, "Failed to create queue for connection");
            return -1;
        }

        if (node_set_input(to, queue) != 0)
        {
            printL(ERROR, INITIATOR, "Failed to set input for node: %s", to->name);
            return -1;
        }
    }

    if (node_add_output(from, queue) != 0)
    {
        printL(ERROR, INITIATOR, "Failed to add output to node: %s", from->name);
        return -1;
    }

    printL(INFO, INITIATOR, "Connected: %s -> %s", from->name, to->name);
    return 0;
}

int pipeline_connect_shared(Pipeline* pipeline, Node* from, Node** to_nodes, int to_count)
{
    if (!pipeline || !from || !to_nodes || to_count <= 0)
    {
        return -1;
    }

    // Создаем автоматическое имя для dispatcher'а
    char dispatcher_name[MAX_NODE_NAME];
    snprintf(dispatcher_name, sizeof(dispatcher_name), "Disp_%.58s", from->name);

    // Создаем dispatcher node с round-robin стратегией
    Node* dispatcher = dispatcher_node_create(dispatcher_name, DISPATCH_ROUND_ROBIN);
    if (!dispatcher)
    {
        printL(ERROR, INITIATOR, "Failed to create dispatcher node");
        return -1;
    }

    // Добавляем dispatcher в pipeline
    if (pipeline_add_node(pipeline, dispatcher) != 0)
    {
        node_destroy(dispatcher);
        printL(ERROR, INITIATOR, "Failed to add dispatcher to pipeline");
        return -1;
    }

    // Подключаем источник к dispatcher'у
    if (pipeline_connect(pipeline, from, dispatcher) != 0)
    {
        printL(ERROR, INITIATOR, "Failed to connect %s to dispatcher", from->name);
        return -1;
    }

    // Подключаем dispatcher к каждому целевому узлу
    for (int i = 0; i < to_count; i++)
    {
        if (!to_nodes[i])
        {
            continue;
        }

        if (pipeline_connect(pipeline, dispatcher, to_nodes[i]) != 0)
        {
            printL(ERROR, INITIATOR, "Failed to connect dispatcher to %s", to_nodes[i]->name);
            return -1;
        }

        printL(INFO, INITIATOR, "Connected (shared via dispatcher): %s -> %s -> %s",
               from->name, dispatcher_name, to_nodes[i]->name);
    }

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
