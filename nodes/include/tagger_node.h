#ifndef TAGGER_NODE_H
#define TAGGER_NODE_H

#include "../../pipeline/node.h"
#include "../../config_parser/config_parser.h"

typedef struct TaggerContext {
    tag_rule_t* tag_rules;
    int tag_rules_size;
} TaggerContext;

Node* tagger_node_create(const char* name, tag_rule_t* tag_rules, int tag_rules_size);

void* tagger_node_process(void* node_ptr);

#endif
