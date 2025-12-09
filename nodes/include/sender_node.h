#ifndef SENDER_NODE_H
#define SENDER_NODE_H

#include "../../pipeline/node.h"
#include <linux/if_packet.h>

typedef struct sender_node_context {
    int socket;
    struct sockaddr_ll saddr_ll;
} sender_context_t;

Node* sender_node_create(const char* name, const char* dev_name);

void* sender_node_process(void* node_ptr);

#endif
