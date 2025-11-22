#include "tagger_node.h"
#include "../logger/logger.h"
#include "../common.h"
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

Node* tagger_node_create(const char* name, tag_rules_t* tag_rules, int tag_rules_size)
{
    if (!name || !tag_rules || tag_rules_size <= 0)
    {
        return NULL;
    }

    TaggerContext* ctx = (TaggerContext*)calloc(1, sizeof(TaggerContext));
    if (!ctx)
    {
        return NULL;
    }

    ctx->tag_rules = tag_rules;
    ctx->tag_rules_size = tag_rules_size;

    Node* node = node_create(name, NODE_TYPE_TAGGER, tagger_node_process, ctx);
    if (!node)
    {
        free(ctx);
        return NULL;
    }

    return node;
}

static short tagger_node_define_tag(uint32_t addr, const tag_rules_t* tag_rules_obj, int size)
{
    if (tag_rules_obj == NULL)
    {
        return -1;
    }

    if (size == 0)
    {
        return -2;
    }

    for (int i = 0; i < size; ++i)
    {
        if (ntohl(tag_rules_obj[i].ip_left.s_addr) <= addr &&
            ntohl(tag_rules_obj[i].ip_right.s_addr) >= addr)
        {
            return (tag_rules_obj)[i].tag;
        }
    }

    return -3;
}

static int tagger_node_analyze(unsigned char* buffer)
{
    short type_field = 0;
    type_field = ((0x0000 | (0xff & buffer[12])) << 8) | (0xff & buffer[13]);

    if (type_field <= 1500)
    {
        return -1;
    }

    if (type_field == 0x8100)
    {
        return -1;
    }

    if (type_field != 0x0800)
    {
        return -2;
    }

    return 0;
}

static uint32_t tagger_node_get_ip(unsigned char* buffer)
{
    uint32_t addr = 0;

    for (int i = 30; i < 34; i++)
    {
        if (i != 33)
        {
            addr = (addr | buffer[i]) << 8;
        }
        else
        {
            addr = (addr | buffer[i]);
        }
    }

    return addr;
}

static unsigned short tagger_node_edit_packet(unsigned char* input_buffer, unsigned char* output_buffer, short tag, unsigned short packet_size)
{
    for (int i = 0; i < 12; i++)
    {
        output_buffer[i] = input_buffer[i];
    }

    output_buffer[12] = 0x81;
    output_buffer[13] = 0x00;
    output_buffer[14] = 0 | 0;
    output_buffer[14] <<= 1;
    output_buffer[14] = output_buffer[14] | 0;
    output_buffer[14] <<= 4;
    output_buffer[14] = output_buffer[14] | ((0x0f00 & tag) >> 8);
    output_buffer[15] = 0x00ff & tag;

    for (int i = 12; i < packet_size; i++)
    {
        output_buffer[i + 4] = input_buffer[i];
    }

    packet_size += 4;

    return packet_size;
}

void* tagger_node_process(void* node_ptr)
{
    if (!node_ptr)
    {
        return NULL;
    }

    Node* node = (Node*)node_ptr;
    TaggerContext* ctx = (TaggerContext*)node->context;

    if (!ctx || !ctx->tag_rules)
    {
        printL(ERROR, TAGGER, "%s: Invalid context", node->name);
        return NULL;
    }

    unsigned char buffer[ETHERNET_FRAME_LENGTH] = {0};
    unsigned char output_buffer[ETHERNET_FRAME_LENGTH] = {0};
    short tag = 0;
    uint32_t addr = 0;
    ssize_t packet_size = 0;

    printL(INFO, TAGGER, "Tagger node started: %s", node->name);

    while (!node->should_exit || !*node->should_exit)
    {
        if (!node->input_queue)
        {
            printL(ERROR, TAGGER, "%s: No input queue", node->name);
            break;
        }

        packet_size = pop(node->input_queue, buffer);

        if (packet_size < 1)
        {
            if (packet_size == 0)
            {
                continue;
            }

            printL(ERROR, TAGGER, "%s: pop error", node->name);
            break;
        }

        if (packet_size < 46)
        {
            continue;
        }

        if (tagger_node_analyze(buffer) != 0)
        {
            continue;
        }

        addr = tagger_node_get_ip(buffer);

        tag = tagger_node_define_tag(addr, (const tag_rules_t*)ctx->tag_rules, ctx->tag_rules_size);

        switch (tag)
        {
            case -1:
            case -2:
                printL(ERROR, TAGGER, "%s: No tagging rules", node->name);
                if (node->should_exit)
                {
                    *node->should_exit = 1;
                }
                return NULL;
            case -3:
                continue;
        }

        packet_size = tagger_node_edit_packet(buffer, output_buffer, tag, packet_size);
        node_send_to_outputs(node, output_buffer, packet_size);
    }

    printL(INFO, TAGGER, "Tagger node stopped: %s", node->name);
    return NULL;
}
