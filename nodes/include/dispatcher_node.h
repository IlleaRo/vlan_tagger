#ifndef DISPATCHER_NODE_H
#define DISPATCHER_NODE_H

#include "../../pipeline/node.h"

// Dispatcher node - распределяет пакеты между несколькими узлами-воркерами
// Читает из одной входной очереди и распределяет по выходным очередям
// Используется вместо shared queue для чистой архитектуры "одна нода = одна очередь"

typedef enum {
    DISPATCH_ROUND_ROBIN,  // По очереди
    DISPATCH_RANDOM        // Случайно
} DispatchStrategy;

typedef struct {
    DispatchStrategy strategy;
    unsigned int next_index;      // Для round-robin
    unsigned long packets_sent;   // Статистика
} DispatcherContext;

Node* dispatcher_node_create(const char* name, DispatchStrategy strategy);

void* dispatcher_node_process(void* node_ptr);

#endif
