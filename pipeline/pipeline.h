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

Pipeline* pipeline_create(void);

int pipeline_add_node(Pipeline* pipeline, Node* node);

Queue_t* pipeline_create_queue(Pipeline* pipeline);

int pipeline_connect(Pipeline* pipeline, Node* from, Node* to);

int pipeline_start(Pipeline* pipeline);

void pipeline_stop(Pipeline* pipeline);

void pipeline_destroy(Pipeline* pipeline);

#endif
