//
// Created by illearo on 13.12.2025.
//

#ifndef VLAN_TAGGER_PIPELINE_PATTERNS_H
#define VLAN_TAGGER_PIPELINE_PATTERNS_H
#include "../pipeline/pipeline.h"
#include "../config_parser/config_parser.h"

typedef struct {
    tag_rules_t *tag_rules;
    const char *src_dev_name;
    const char *dst_dev_name;
} vlan_tagger_context_t;

int vlan_tagger_pattern(Pipeline *pipeline, const void *ctx);

int detailed_logging_pattern(Pipeline *pipeline, const void *ctx);

#endif //VLAN_TAGGER_PIPELINE_PATTERNS_H