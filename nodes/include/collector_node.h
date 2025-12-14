#ifndef COLLECTOR_NODE_H
#define COLLECTOR_NODE_H

#include "../../pipeline/node.h"

// Collector node - простая passthrough нода для объединения данных от нескольких источников
// Не выполняет никакой обработки, просто пробрасывает данные дальше
// Полезна для устранения дублирования в pipeline паттернах

Node* collector_node_create(const char* name);

void* collector_node_process(void* node_ptr);

#endif
