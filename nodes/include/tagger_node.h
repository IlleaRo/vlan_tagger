#ifndef TAGGER_NODE_H
#define TAGGER_NODE_H

#include "../../pipeline/node.h"
#include "../../config_parser/config_parser.h"

typedef struct {
    uint32_t ip_left;   // host byte order
    uint32_t ip_right;  // host byte order
    int tag;
} tagger_rule_t;

typedef struct TaggerContext {
    tagger_rule_t* tag_rules;  // Копия правил с IP в host byte order
    int tag_rules_size;
} TaggerContext;

Node* tagger_node_create(const char* name, const tag_rule_t* tag_rules, int tag_rules_size);

void* tagger_node_process(void* node_ptr);

#endif
