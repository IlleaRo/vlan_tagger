#ifndef PIPELINE_H
#define PIPELINE_H

#include "node.h"
#include "../queue/queue.h"

#define MAX_PIPELINE_NODES 32
#define MAX_PIPELINE_QUEUES 64

typedef struct Pipeline {
    Node** nodes;
    int node_count;

    Queue_t** queues;
    int queue_count;

    int should_exit;
} Pipeline;

int pipeline_init(Pipeline *pipeline_ptr);

typedef int (*pipeline_pattern_builder_cb)(Pipeline* pipeline, const void* ctx);

int pipeline_build(Pipeline* pipeline, const pipeline_pattern_builder_cb cb, const void* ctx);

int pipeline_add_node(Pipeline* pipeline, Node* node);

Queue_t* pipeline_create_queue(Pipeline* pipeline);

// Создает отдельную очередь для каждого соединения (дублирование данных)
int pipeline_connect(Pipeline* pipeline, Node* from, Node* to);

// Создает автоматический Dispatcher node для распределения данных между узлами
// Dispatcher читает из одной входной очереди и распределяет по отдельным очередям
// для каждого целевого узла (load balancing через round-robin)
int pipeline_connect_shared(Pipeline* pipeline, Node* from, Node** to_nodes, int to_count);

int pipeline_start(Pipeline* pipeline);

void pipeline_stop(Pipeline* pipeline);

void pipeline_destroy(Pipeline* pipeline);

#endif
