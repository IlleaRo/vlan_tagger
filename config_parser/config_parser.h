#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

typedef struct tag_rules
{
    struct in_addr ip_left;
    struct in_addr ip_right;
    int tag;
} tag_rules_t;

int tag_rules_init(tag_rules_t **, int);
int tag_rules_clear(tag_rules_t **);
int tag_rules_check_collisions(const tag_rules_t *, int);

int config_file_check(void);
int config_file_read(tag_rules_t *, int);
