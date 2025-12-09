#ifndef SNIFFER_NODE_H
#define SNIFFER_NODE_H

#include "../../pipeline/node.h"
#include <linux/if_packet.h>

#define MAX_ERR_NUM 3

typedef struct sniffer_context {
    int socket;
    struct sockaddr_ll saddr_ll;
} sniffer_context_t;

Node* sniffer_node_create(const char *, const char *);

void* sniffer_node_process(void* node_ptr);

#endif
