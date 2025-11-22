#ifndef PACKET_COUNTER_NODE_H
#define PACKET_COUNTER_NODE_H

#include "../pipeline/node.h"

typedef struct PacketCounterContext {
    unsigned long total_packets;
    unsigned long total_bytes;
    unsigned long vlan_packets;
    unsigned long non_vlan_packets;
    int log_interval;  // Логировать каждые N пакетов
} PacketCounterContext;

Node* packet_counter_node_create(const char* name, int log_interval);

void* packet_counter_node_process(void* node_ptr);

#endif
