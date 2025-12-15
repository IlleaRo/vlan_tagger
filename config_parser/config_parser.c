#define _GNU_SOURCE

#include "config_parser.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <errno.h>
#include <limits.h>

#define MAX_LINE_LENGTH 100
#define MAX_RULE_CONTENT_LENGTH 37
#define MIN_VLAN_ID 0
#define MAX_VLAN_ID 4095

static int ip_converting(struct in_addr *in_addr_obj, const char *ip_str)
{
    if (in_addr_obj == NULL || ip_str == NULL)
    {
        return -1;
    }

    if (inet_aton(ip_str, in_addr_obj) == 0)
    {
        return -1;
    }

    return 0;
}

static int tag_converting(int *tag_int, const char *tag_str)
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
        return -1;
    }

    if (endptr == tag_str || *endptr != '\0')
    {
        return -1;
    }

    *tag_int = (int)tag_long;

    return 0;
}

static int char_count(const char *str, char symbol)
{
    if (str == NULL)
    {
        return -1;
    }

    int count = 0;
    for (size_t i = 0; i < strlen(str); i++)
    {
        if (str[i] == symbol)
        {
            count++;
        }
    }

    return count;
}

static char* line_editor(char *string)
{
    if (!string)
    {
        return NULL;
    }

    // Поиск начала строки (пропуск пробелов и табов)
    for (size_t i = 0; i < strlen(string); ++i)
    {
        if (string[i] != ' ' && string[i] != '\t')
        {
            string = &string[i];
            break;
        }
    }

    // Строка-комментарий
    if (*string == '#')
    {
        return string;
    }

    // Обрезаем строку на первом пробеле/табе/комментарии
    for (int j = 0; j < MAX_RULE_CONTENT_LENGTH; j++)
    {
        if (string[j] == ' ' || string[j] == '\t' || string[j] == '#' || string[j] == '\n')
        {
            string[j] = '\0';
            break;
        }
    }

    return string;
}

static int validate_rules(const tag_rules_t *tag_rules)
{
    // Проверка 1: ip_left <= ip_right в каждом правиле
    for (int i = 0; i < tag_rules->size; i++)
    {
        uint32_t left = ntohl(tag_rules->rules[i].ip_left.s_addr);
        uint32_t right = ntohl(tag_rules->rules[i].ip_right.s_addr);

        if (left > right)
        {
            return -11;
        }
    }

    // Проверка 2: коллизии диапазонов
    for (int i = 0; i < tag_rules->size; i++)
    {
        uint32_t i_left = ntohl(tag_rules->rules[i].ip_left.s_addr);
        uint32_t i_right = ntohl(tag_rules->rules[i].ip_right.s_addr);

        for (int j = i + 1; j < tag_rules->size; j++)
        {
            uint32_t j_left = ntohl(tag_rules->rules[j].ip_left.s_addr);
            uint32_t j_right = ntohl(tag_rules->rules[j].ip_right.s_addr);

            // Проверка пересечения: !(i_right < j_left || j_right < i_left)
            if (!(i_right < j_left || j_right < i_left))
            {
                return -12;
            }
        }
    }

    // Проверка 3: валидность VLAN ID
    for (int i = 0; i < tag_rules->size; i++)
    {
        if (tag_rules->rules[i].tag < MIN_VLAN_ID ||
            tag_rules->rules[i].tag > MAX_VLAN_ID)
        {
            return -13;
        }
    }

    return 0;
}

int tag_rules_load(tag_rules_t *tag_rules, const char *config_path)
{
    if (tag_rules == NULL || config_path == NULL)
    {
        return -1;
    }

    FILE *pfile = fopen(config_path, "r");
    if (pfile == NULL)
    {
        return -2;
    }

    // Обнуляем структуру перед загрузкой
    memset(tag_rules, 0, sizeof(tag_rules_t));

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
                return -3;
            }
        }

        char* real_start = line_editor(rule);
        if (real_start == NULL)
        {
            fclose(pfile);
            return -4;
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

        if (loaded_count >= MAX_RULES)
        {
            fclose(pfile);
            return -6;
        }

        rule_part = strtok_r(real_start, "-", &saveptr);
        if (rule_part == NULL)
        {
            fclose(pfile);
            return -7;
        }

        if (ip_converting(&tag_rules->rules[loaded_count].ip_left, rule_part) != 0)
        {
            fclose(pfile);
            return -8;
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
                return -7;
            }

            if (ip_converting(&tag_rules->rules[loaded_count].ip_right, rule_part) != 0)
            {
                fclose(pfile);
                return -8;
            }
        }

        rule_part = strtok_r(NULL, "-", &saveptr);
        if (rule_part == NULL)
        {
            fclose(pfile);
            return -7;
        }

        if (tag_converting(&tag_rules->rules[loaded_count].tag, rule_part) != 0)
        {
            fclose(pfile);
            return -9;
        }

        loaded_count++;
    }

    if (fclose(pfile) == EOF)
    {
        return -10;
    }

    tag_rules->size = loaded_count;

    // Валидация загруженных правил
    if (loaded_count > 0)
    {
        int validation_result = validate_rules(tag_rules);
        if (validation_result != 0)
        {
            return validation_result;
        }
    }

    return loaded_count;
}
