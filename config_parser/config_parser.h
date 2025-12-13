#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include <netinet/in.h>

typedef struct tag_rules
{
    struct in_addr ip_left;
    struct in_addr ip_right;
    int tag;
} tag_rule_t;

typedef struct {
    tag_rule_t* rules;
    int size;
    int capacity;
} tag_rules_t;

int tag_rules_init(tag_rules_t *tag_rules, int initial_capacity);
void tag_rules_destroy(tag_rules_t *tag_rules);
int tag_rules_load_from_file(tag_rules_t *tag_rules, const char *config_path);
int tag_rules_validate(const tag_rules_t *tag_rules);
const tag_rule_t* tag_rules_get_array(const tag_rules_t *tag_rules);
int tag_rules_get_size(const tag_rules_t *tag_rules);

#endif
