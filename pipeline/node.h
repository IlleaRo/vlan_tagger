#ifndef PIPELINE_NODE_H
#define PIPELINE_NODE_H

#include <pthread.h>
#include "../queue/queue.h"

#define MAX_NODE_OUTPUTS 16
#define MAX_NODE_NAME 64

typedef struct Node Node;

typedef void* (*node_process_func)(void* node);

typedef enum {
    NODE_TYPE_SNIFFER,
    NODE_TYPE_TAGGER,
    NODE_TYPE_SENDER,
    NODE_TYPE_IP_MODIFIER,
    NODE_TYPE_CUSTOM
} NodeType;

struct Node {
    char name[MAX_NODE_NAME];
    NodeType type;

    Queue_t* input_queue;
    Queue_t** output_queues;
    int output_count;

    node_process_func process;
    void* context;

    pthread_t thread;
    int* should_exit;
};

Node* node_create(const char* name, NodeType type, node_process_func process, void* context);

int node_add_output(Node* node, Queue_t* output_queue);

int node_set_input(Node* node, Queue_t* input_queue);

int node_start(Node* node);

void node_stop(Node* node);

void node_destroy(Node* node);

void node_send_to_outputs(Node* node, unsigned char* buffer, unsigned short size);

#endif
