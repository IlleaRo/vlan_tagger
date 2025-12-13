#define _GNU_SOURCE

#include "config_parser.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <errno.h>
#include <limits.h>

#define FILE_DIR "vlan-tagger.cfg"
#define MAX_LINE_LENGTH 100
#define MAX_RULE_CONTENT_LENGTH 37
#define MIN_VLAN_ID 0
#define MAX_VLAN_ID 4096
#define DEFAULT_INITIAL_CAPACITY 16
#define CAPACITY_GROWTH_FACTOR 2

static int tag_rules_ensure_capacity(tag_rules_t *tag_rules, int required_size);

int ip_converting(struct in_addr *, const char *);
char* line_editor(char *);
int tag_converting(int *, const char *);
int ip_comparison(const struct in_addr, const struct in_addr);
int char_count(const char *, char);

int ip_converting(struct in_addr *in_addr_obj, const char *ip_str)
{
    if (in_addr_obj == NULL || ip_str == NULL)
    {
        return -1;
    }

    if (inet_aton(ip_str, in_addr_obj) == 0)
    {
        return -2;
    }

    return 0;
}

int tag_converting(int *tag_int, const char *tag_str)
{
    if (tag_int == NULL || tag_str == NULL)
    {
        return -1;
    }

    char *endptr;
    errno = 0;
    long tag_long = strtol(tag_str, &endptr, 10);

    if (errno == ERANGE || tag_long < INT_MIN || tag_long > INT_MAX)
    {
        return -2;
    }

    if (endptr == tag_str || *endptr != '\0')
    {
        return -3;
    }

    *tag_int = (int)tag_long;

    return 0;
}

int ip_comparison(const struct in_addr ip_1, const struct in_addr ip_2)
{
    uint32_t addr1_host = ntohl(ip_1.s_addr);
    uint32_t addr2_host = ntohl(ip_2.s_addr);

    if (addr1_host < addr2_host)
    {
        return 0;
    }
    else if (addr1_host == addr2_host)
    {
        return 1;
    }
    else
    {
        return 2;
    }
}

int char_count(const char *str, char symbol)
{
    if (str == NULL)
    {
        return -1;
    }

    int count = 0;
    for (int i = 0; i < strlen(str); i++)
    {
        if (str[i] == symbol)
        {
            count++;
        }
    }

    return count;
}

char* line_editor(char *string)
{
    if (!string)
    {
        return NULL;
    }

    //Поиск начала строки
    unsigned long i;
    for (i = 0; i < strlen(string); ++i)
    {
        if (string[i] != ' ' && string[i] != '\t')
        {
            string = &string[i];
            break;
        }
    }

    // Ситуация, когда вся строка - комментарий
    if (*string == '#')
    {
        return string;
    }

    //Поиск пробелов или табуляций до комментариев
    for (char j = 0; j < MAX_RULE_CONTENT_LENGTH; j++)
    {
        if (string[j] == ' ' || string[j] == '\t' || string[j] == '#' || string[j] == '\n')
        {
            string[j] = '\0';
            break;
        }
    }

    return string;
}

int tag_rules_init(tag_rules_t *tag_rules, int initial_capacity)
{
    if (tag_rules == NULL)
    {
        return -1;
    }

    if (tag_rules->rules != NULL)
    {
        return -1;
    }

    if (initial_capacity <= 0)
    {
        initial_capacity = DEFAULT_INITIAL_CAPACITY;
    }

    tag_rules->rules = (tag_rule_t*)calloc(initial_capacity, sizeof(tag_rule_t));
    if (tag_rules->rules == NULL)
    {
        return -2;
    }

    tag_rules->size = 0;
    tag_rules->capacity = initial_capacity;

    return 0;
}

void tag_rules_destroy(tag_rules_t *tag_rules)
{
    if (tag_rules == NULL)
    {
        return;
    }

    if (tag_rules->rules != NULL)
    {
        free(tag_rules->rules);
        tag_rules->rules = NULL;
    }

    tag_rules->size = 0;
    tag_rules->capacity = 0;
}

const tag_rule_t* tag_rules_get_array(const tag_rules_t *tag_rules)
{
    if (tag_rules == NULL)
    {
        return NULL;
    }

    return tag_rules->rules;
}

int tag_rules_get_size(const tag_rules_t *tag_rules)
{
    if (tag_rules == NULL)
    {
        return -1;
    }

    return tag_rules->size;
}

static int tag_rules_ensure_capacity(tag_rules_t *tag_rules, int required_size)
{
    if (tag_rules == NULL || tag_rules->rules == NULL)
    {
        return -1;
    }

    if (required_size <= tag_rules->capacity)
    {
        return 0;
    }

    int new_capacity = tag_rules->capacity;
    while (new_capacity < required_size)
    {
        new_capacity *= CAPACITY_GROWTH_FACTOR;
    }

    tag_rule_t *new_rules = (tag_rule_t*)realloc(tag_rules->rules,
                                                   new_capacity * sizeof(tag_rule_t));
    if (new_rules == NULL)
    {
        return -2;
    }

    memset(new_rules + tag_rules->capacity, 0,
           (new_capacity - tag_rules->capacity) * sizeof(tag_rule_t));

    tag_rules->rules = new_rules;
    tag_rules->capacity = new_capacity;

    return 0;
}

int tag_rules_load_from_file(tag_rules_t *tag_rules, const char *config_path)
{
    if (tag_rules == NULL || config_path == NULL)
    {
        return -1;
    }

    if (tag_rules->rules == NULL)
    {
        return -2;
    }

    FILE *pfile = fopen(config_path, "r");
    if (pfile == NULL)
    {
        return -3;
    }

    int loaded_count = 0;

    for (;;)
    {
        char rule[MAX_LINE_LENGTH] = { 0 };
        char *prule;
        char *rule_part;
        char *saveptr;
        int dash_count;

        prule = fgets(rule, sizeof(rule), pfile);
        if (prule == NULL)
        {
            if (feof(pfile) != 0)
            {
                break;
            }
            else
            {
                fclose(pfile);
                return -4;
            }
        }

        char* real_start = line_editor(rule);
        if (real_start == NULL)
        {
            fclose(pfile);
            return -10;
        }

        if (*real_start == '\0' || *real_start == '#' || *real_start == '\n')
        {
            continue;
        }

        dash_count = char_count(real_start, '-');
        if (dash_count < 1 || dash_count > 2)
        {
            fclose(pfile);
            return -5;
        }

        if (tag_rules_ensure_capacity(tag_rules, loaded_count + 1) != 0)
        {
            fclose(pfile);
            return -11;
        }

        rule_part = strtok_r(real_start, "-", &saveptr);
        if (rule_part == NULL)
        {
            fclose(pfile);
            return -6;
        }

        if (ip_converting(&tag_rules->rules[loaded_count].ip_left, rule_part) != 0)
        {
            fclose(pfile);
            return -7;
        }

        if (dash_count == 1)
        {
            tag_rules->rules[loaded_count].ip_right = tag_rules->rules[loaded_count].ip_left;
        }
        else
        {
            rule_part = strtok_r(NULL, "-", &saveptr);
            if (rule_part == NULL)
            {
                fclose(pfile);
                return -6;
            }

            if (ip_converting(&tag_rules->rules[loaded_count].ip_right, rule_part) != 0)
            {
                fclose(pfile);
                return -7;
            }
        }

        rule_part = strtok_r(NULL, "-", &saveptr);
        if (rule_part == NULL)
        {
            fclose(pfile);
            return -6;
        }

        if (tag_converting(&tag_rules->rules[loaded_count].tag, rule_part) != 0)
        {
            fclose(pfile);
            return -8;
        }

        loaded_count++;
    }

    if (fclose(pfile) == EOF)
    {
        return -9;
    }

    tag_rules->size = loaded_count;

    return loaded_count;
}

int tag_rules_validate(const tag_rules_t *tag_rules)
{
    if (tag_rules == NULL || tag_rules->rules == NULL)
    {
        return -1;
    }

    if (tag_rules->size <= 0)
    {
        return -1;
    }

    int err_count = 0;

    // Проверка 1: ip_left <= ip_right в каждом правиле
    for (int i = 0; i < tag_rules->size; i++)
    {
        int res = ip_comparison(tag_rules->rules[i].ip_left,
                                tag_rules->rules[i].ip_right);
        if (res < 0)
        {
            return -2;
        }
        else if (res > 1)
        {
            err_count++;
        }
    }

    if (err_count > 0)
    {
        return err_count;
    }

    // Проверка 2: коллизии диапазонов
    err_count = 0;
    for (int i = 0; i < tag_rules->size; i++)
    {
        for (int j = i + 1; j < tag_rules->size; j++)
        {
            int res_iright_jleft = ip_comparison(tag_rules->rules[i].ip_right,
                                                  tag_rules->rules[j].ip_left);
            if (res_iright_jleft < 0)
            {
                return -2;
            }
            else if (res_iright_jleft == 0)
            {
                continue;
            }
            else
            {
                int res_jright_ileft = ip_comparison(tag_rules->rules[j].ip_right,
                                                      tag_rules->rules[i].ip_left);
                if (res_jright_ileft < 0)
                {
                    return -2;
                }
                else if (res_jright_ileft == 0)
                {
                    continue;
                }
                else
                {
                    err_count++;
                }
            }
        }
    }

    if (err_count > 0)
    {
        return err_count;
    }

    // Проверка 3: валидность VLAN ID
    err_count = 0;
    for (int i = 0; i < tag_rules->size; i++)
    {
        if (tag_rules->rules[i].tag < MIN_VLAN_ID ||
            tag_rules->rules[i].tag > MAX_VLAN_ID)
        {
            err_count++;
        }
    }

    if (err_count > 0)
    {
        return err_count;
    }

    return 0;
}
