#ifndef SENDER_NODE_H
#define SENDER_NODE_H

#include "../pipeline/node.h"
#include <sys/socket.h>
#include <linux/if_packet.h>

typedef struct SenderContext {
    int* socket;
    struct sockaddr_ll* saddr;
    int* saddr_len;
} SenderContext;

Node* sender_node_create(const char* name, int* socket, struct sockaddr_ll* saddr, int* saddr_len);

void* sender_node_process(void* node_ptr);

#endif
