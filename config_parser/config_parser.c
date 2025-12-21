#include "config_parser.h"
#include <string.h>

#define FILE_DIR "vlan-tagger.cfg"

int ip_converting(struct in_addr *, const char *);
char* line_editor(char *);
int tag_converting(int *, const char *);
int ip_comparison(const struct in_addr, const struct in_addr);
int ip_str_to_int(char *, int *);
int char_count(const char *, char);

int tag_rules_init(tag_rules_t **tag_rules_obj, int size)
{
    if (*tag_rules_obj != NULL || size <= 0)
    {
        return -1;
    }

    *tag_rules_obj = (tag_rules_t*) calloc(size, sizeof(tag_rules_t));

    if (tag_rules_obj == NULL)
    {
        return -2;
    }

    return 0;
}

int tag_rules_clear(tag_rules_t **tag_rules_obj)
{
    if (*tag_rules_obj == NULL)
    {
        return -1;
    }

    free(*tag_rules_obj);

    return 0;
}

int tag_rules_check_collisions(const tag_rules_t *tag_rules_obj, int size)
{
    if (tag_rules_obj == NULL || size <= 0)
    {
        return -1;
    }

    int err_count = 0;
    for (int i = 0; i < size; i++)
    {
        int res = ip_comparison(tag_rules_obj[i].ip_left, tag_rules_obj[i].ip_right);
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

    err_count = 0;
    for (int i = 0; i < size; i++)
    {
        for (int j = i + 1; j < size; j++)
        {
            int res_iright_jleft = ip_comparison(tag_rules_obj[i].ip_right, tag_rules_obj[j].ip_left);
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
                int res_jright_ileft = ip_comparison(tag_rules_obj[j].ip_right, tag_rules_obj[i].ip_left);
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

    err_count = 0;
    for (int i = 0; i < size; i++)
    {
        if (tag_rules_obj[i].tag < 0 || tag_rules_obj[i].tag > 4096)
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

int config_file_check(void)
{
    FILE *pFile;
    if ((pFile = fopen(FILE_DIR, "r")) == NULL)
    {
        return -1;
    }

    if (fclose(pFile) > 0)
    {
        return -2;
    }

    return 0;
}

int config_file_read(tag_rules_t *tag_rules_obj, int size)
{
    if (tag_rules_obj == NULL)
    {
        return -1;
    }

	int i;
    FILE* pfile;
    if ((pfile = fopen(FILE_DIR, "r")) == NULL)
    {
        return -2;
    }

    for (i = 0;; i++)
    {
		char rule[100] = { 0 };
        char *prule;
        char *rule_part;
        int full_stop_count;
		
        if (i == size)
        {
            break;
        }

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

        char* real_start;
        if ((real_start = line_editor(rule)) == NULL)
        {
            fclose(pfile);
            return -10;
        }

        if (*real_start == '\0' || *real_start == '#' || *real_start == '\n')
        {
            --i;
            continue;
        }

        full_stop_count = char_count(real_start, '-');
        if (full_stop_count < 1 || full_stop_count > 2)
        {
            fclose(pfile);
            return -5;
        }

        rule_part = strtok(real_start, "-");
        if (rule_part == NULL)
        {
            fclose(pfile);
            return -6;
        }

        if (ip_converting(&tag_rules_obj[i].ip_left, rule_part) != 0)
        {
            fclose(pfile);
            return -7;
        }

        if (full_stop_count == 1)
        {
            tag_rules_obj[i].ip_right = tag_rules_obj[i].ip_left;
        }
        else
        {
            rule_part = strtok(NULL, "-");
            if (rule_part == NULL)
            {
                fclose(pfile);
                return -6;
            }

            if (ip_converting(&tag_rules_obj[i].ip_right, rule_part) != 0)
            {
                fclose(pfile);
                return -7;
            }
        }

        rule_part = strtok(NULL, "-");
        if (rule_part == NULL)
        {
            fclose(pfile);
            return -6;
        }

        if (tag_converting(&tag_rules_obj[i].tag, rule_part) != 0)
        {
            fclose(pfile);
            return -8;
        }
    }

    if (fclose(pfile) == EOF)
    {
        return -9;
    }

    return i;
}

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

	int tag_int_tmp;
    tag_int_tmp = atoi(tag_str);
    if (tag_int_tmp == 0 && tag_str[0] != '0')
    {
        return -2;
    }

    *tag_int = tag_int_tmp;

    return 0;
}

int ip_comparison(const struct in_addr ip_1, const struct in_addr ip_2)
{
    char ip_1_str[16] = { 0 };
    char ip_2_str[16] = { 0 };
    char *pip_1_str = ip_1_str;
    char *pip_2_str = ip_2_str;
    char *ip_1_str_tmp;
    char *ip_2_str_tmp;
	int ip_1_int[4] = { 0 };
    int ip_2_int[4] = { 0 };

    ip_1_str_tmp = inet_ntoa(ip_1);
    if (ip_1_str_tmp == NULL)
    {
        return -1;
    }

    memcpy(pip_1_str, ip_1_str_tmp, strlen(ip_1_str_tmp) + 1);

    ip_2_str_tmp = inet_ntoa(ip_2);
    if (pip_2_str == NULL)
    {
        return -1;
    }

    memcpy(pip_2_str, ip_2_str_tmp, strlen(ip_2_str_tmp) + 1);

    if (ip_str_to_int(pip_1_str, ip_1_int) != 0)
    {
        return -2;
    }
    if (ip_str_to_int(pip_2_str, ip_2_int) != 0)
    {
        return -2;
    }

    for (int i = 0; i < 4; i++)
    {
        if (ip_1_int[i] < ip_2_int[i])
        {
            return 0;
        }
        else if (ip_1_int[i] == ip_2_int[i])
        {
            if (i == 3)
            {
                return 1;
            }
            else
            {
                continue;
            }
        }
        else if (ip_1_int[i] > ip_2_int[i])
        {
            return 2;
        }
    }
}

int ip_str_to_int(char *ip_str, int *ip_int)
{
    if (ip_str == NULL || ip_int == NULL)
    {
        return -1;
    }

    int i = 0;
    char *ip_str_part = strtok(ip_str, ".");
    while (ip_str_part != NULL)
    {
        ip_int[i] = atoi(ip_str_part);
        if (ip_int[i] == 0 && ip_str_part[0] != '0')
        {
            return -2;
        }

        i++;
        ip_str_part = strtok(NULL, ".");
    }

    return 0;
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
    for (char j = 0; j < 37; j++)
    {
        if (string[j] == '\n')
        {
            continue;
        }
        if (string[j] == ' ' || string[j] == '\t' || string[j] == '#')
        {
            string[j] = '\0';
            break;
        }
    }

    return string;
}
