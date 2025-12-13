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
} tag_rules_t; // TODO: нужно пересмотреть проект и переписать раздельное использование массива правил и размера на единую структуру

int tag_rules_init(tag_rule_t **, int);
int tag_rules_clear(tag_rule_t **);
int tag_rules_check_collisions(const tag_rule_t *, int);
void tag_rules_convert_to_host_order(tag_rule_t *, int);  // TODO: убрать костыль после рефакторинга правил

int config_file_check(void);
int config_file_read(tag_rule_t *, int);

#endif
