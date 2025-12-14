//
// Created by illearo on 13.12.2025.
//

#include "patterns.h"

#include <stdlib.h>

#include "../nodes/include/sniffer_node.h"
#include "../nodes/include/tagger_node.h"
#include "../nodes/include/packet_counter_node.h"
#include "../nodes/include/sender_node.h"
#include "../nodes/include/logger_node.h"
#include "../nodes/include/collector_node.h"

int vlan_tagger_pattern(Pipeline *pipeline, const void *ctx) {
    const vlan_tagger_context_t *vlan_tagger_context = ctx;

    // Создаем узлы для сложного pipeline
    Node *sniffer = NULL;
    Node *tagger1 = NULL;
    Node *tagger2 = NULL;
    Node *counter = NULL;
    Node *sender = NULL;

    if (!((sniffer = sniffer_node_create("Sniffer", vlan_tagger_context->src_dev_name)))
        || !((tagger1 = tagger_node_create("Tagger1", vlan_tagger_context->tag_rules->rules,
                                           vlan_tagger_context->tag_rules->size)))
        || !((tagger2 = tagger_node_create("Tagger2", vlan_tagger_context->tag_rules->rules,
                                           vlan_tagger_context->tag_rules->size)))
        || !((counter = packet_counter_node_create("PacketCounter", 10)))
        || !((sender = sender_node_create("Sender", vlan_tagger_context->dst_dev_name)))) {
        goto __clear_and_exit;
    }

    // Добавляем все узлы в pipeline
    pipeline_add_node(pipeline, sniffer);
    pipeline_add_node(pipeline, tagger1);
    pipeline_add_node(pipeline, tagger2);
    pipeline_add_node(pipeline, counter);
    pipeline_add_node(pipeline, sender);

    // Sniffer -> [Tagger1, Tagger2] (shared queue)
    Node* tagger_pool[] = {tagger1, tagger2};
    pipeline_connect_shared(pipeline, sniffer, tagger_pool, 2);

    // [Tagger1, Tagger2] -> PacketCounter
    pipeline_connect(pipeline, tagger1, counter);
    pipeline_connect(pipeline, tagger2, counter);

    // PacketCounter -> Sender
    pipeline_connect(pipeline, counter, sender);

    return 0;

__clear_and_exit:
    node_destroy(sniffer);
    node_destroy(tagger1);
    node_destroy(tagger2);
    node_destroy(counter);
    node_destroy(sender);

    return 1;
}

int detailed_logging_pattern(Pipeline *pipeline, const void *ctx) {
    const vlan_tagger_context_t *vlan_tagger_context = ctx;

    // Создаем узлы для detailed logging pipeline
    Node *sniffer = NULL;
    Node *tagger1 = NULL;
    Node *tagger2 = NULL;
    Node *collector = NULL;
    Node *logger1 = NULL;
    Node *logger2 = NULL;
    Node *counter = NULL;
    Node *sender = NULL;

    if (!((sniffer = sniffer_node_create("Sniffer", vlan_tagger_context->src_dev_name)))
        || !((tagger1 = tagger_node_create("Tagger1", vlan_tagger_context->tag_rules->rules,
                                           vlan_tagger_context->tag_rules->size)))
        || !((tagger2 = tagger_node_create("Tagger2", vlan_tagger_context->tag_rules->rules,
                                           vlan_tagger_context->tag_rules->size)))
        || !((collector = collector_node_create("Collector")))
        || !((logger1 = logger_node_create("Logger1", 1)))
        || !((logger2 = logger_node_create("Logger2", 1)))
        || !((counter = packet_counter_node_create("PacketCounter", 10)))
        || !((sender = sender_node_create("Sender", vlan_tagger_context->dst_dev_name)))) {
        goto __clear_and_exit_logging;
    }

    // Добавляем все узлы в pipeline
    pipeline_add_node(pipeline, sniffer);
    pipeline_add_node(pipeline, tagger1);
    pipeline_add_node(pipeline, tagger2);
    pipeline_add_node(pipeline, collector);
    pipeline_add_node(pipeline, logger1);
    pipeline_add_node(pipeline, logger2);
    pipeline_add_node(pipeline, counter);
    pipeline_add_node(pipeline, sender);

    // Sniffer -> [Tagger1, Tagger2], Logger1
    Node* tagger_pool[] = {tagger1, tagger2};
    pipeline_connect_shared(pipeline, sniffer, tagger_pool, 2);
    pipeline_connect(pipeline, sniffer, logger1);

    // [Tagger1, Tagger2] -> Collector
    pipeline_connect(pipeline, tagger1, collector);
    pipeline_connect(pipeline, tagger2, collector);

    // Collector -> Sender, Logger2, Counter
    pipeline_connect(pipeline, collector, sender);
    pipeline_connect(pipeline, collector, logger2);
    pipeline_connect(pipeline, collector, counter);

    return 0;

__clear_and_exit_logging:
    node_destroy(sniffer);
    node_destroy(tagger1);
    node_destroy(tagger2);
    node_destroy(collector);
    node_destroy(logger1);
    node_destroy(logger2);
    node_destroy(counter);
    node_destroy(sender);

    return 1;
}
