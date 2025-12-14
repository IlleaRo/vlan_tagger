#ifndef LOGGER_NODE_H
#define LOGGER_NODE_H

#include "../../pipeline/node.h"

typedef struct LoggerContext {
    int log_interval;  // Логировать каждый N-й пакет (<=1 - все)
    unsigned long packet_count;
} LoggerContext;

Node* logger_node_create(const char* name, int log_interval);

void* logger_node_process(void* node_ptr);

#endif
