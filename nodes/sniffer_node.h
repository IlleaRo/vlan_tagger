#ifndef SNIFFER_NODE_H
#define SNIFFER_NODE_H

#include "../pipeline/node.h"
#include <sys/socket.h>
#include <linux/if_packet.h>

typedef struct SnifferContext {
    int* socket;
    struct sockaddr_ll* saddr;
    int* saddr_len;
} SnifferContext;

Node* sniffer_node_create(const char* name, int* socket, struct sockaddr_ll* saddr, int* saddr_len);

void* sniffer_node_process(void* node_ptr);

#endif
