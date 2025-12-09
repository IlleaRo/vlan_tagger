#ifndef IP_MODIFIER_NODE_H
#define IP_MODIFIER_NODE_H

#include "../../pipeline/node.h"
#include <stdint.h>

typedef struct IpModifierContext {
    uint32_t new_ip;
} IpModifierContext;

Node* ip_modifier_node_create(const char* name, const char* new_ip_str);

void* ip_modifier_node_process(void* node_ptr);

#endif
