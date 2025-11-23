#include "ip_modifier_node.h"
#include "../logger/logger.h"
#include "../common.h"
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

Node* ip_modifier_node_create(const char* name, const char* new_ip_str)
{
    if (!name || !new_ip_str)
    {
        return NULL;
    }

    IpModifierContext* ctx = (IpModifierContext*)calloc(1, sizeof(IpModifierContext));
    if (!ctx)
    {
        return NULL;
    }

    // Преобразуем строку в 32-битное значение в сетевом порядке
    struct in_addr addr;
    if (inet_aton(new_ip_str, &addr) == 0)
    {
        free(ctx);
        return NULL;
    }

    ctx->new_ip = addr.s_addr;  // Уже в сетевом порядке байт

    Node* node = node_create(name, NODE_TYPE_IP_MODIFIER, ip_modifier_node_process, ctx);
    if (!node)
    {
        free(ctx);
        return NULL;
    }

    return node;
}

static int is_ip_packet(unsigned char* buffer)
{
    short type_field = ((0x0000 | (0xff & buffer[12])) << 8) | (0xff & buffer[13]);

    if (type_field == 0x8100)
    {
        type_field = ((0x0000 | (0xff & buffer[16])) << 8) | (0xff & buffer[17]);
    }

    return (type_field == 0x0800);
}

static void modify_destination_ip(unsigned char* buffer, uint32_t new_ip)
{
    short type_field = ((0x0000 | (0xff & buffer[12])) << 8) | (0xff & buffer[13]);
    int ip_offset = 14;

    if (type_field == 0x8100)
    {
        ip_offset = 18;
    }

    int dest_ip_offset = ip_offset + 16;

    // Копируем IP-адрес (уже в сетевом порядке байт)
    memcpy(&buffer[dest_ip_offset], &new_ip, 4);

    int ihl = (buffer[ip_offset] & 0x0F) * 4;

    buffer[ip_offset + 10] = 0;
    buffer[ip_offset + 11] = 0;

    uint32_t checksum = 0;
    for (int i = 0; i < ihl; i += 2)
    {
        uint16_t word = (buffer[ip_offset + i] << 8) | buffer[ip_offset + i + 1];
        checksum += word;
    }

    while (checksum >> 16)
    {
        checksum = (checksum & 0xFFFF) + (checksum >> 16);
    }

    checksum = ~checksum;

    buffer[ip_offset + 10] = (checksum >> 8) & 0xFF;
    buffer[ip_offset + 11] = checksum & 0xFF;
}

void* ip_modifier_node_process(void* node_ptr)
{
    if (!node_ptr)
    {
        return NULL;
    }

    Node* node = (Node*)node_ptr;
    IpModifierContext* ctx = (IpModifierContext*)node->context;

    if (!ctx)
    {
        printL(ERROR, INITIATOR, "%s: Invalid context", node->name);
        return NULL;
    }

    unsigned char buffer[ETHERNET_FRAME_LENGTH] = {0};
    ssize_t packet_size = 0;

    // Преобразуем IP обратно в строку для логирования
    char ip_str[INET_ADDRSTRLEN];
    struct in_addr addr;
    addr.s_addr = ctx->new_ip;
    inet_ntop(AF_INET, &addr, ip_str, sizeof(ip_str));

    printL(INFO, INITIATOR, "IP Modifier node started: %s (new IP: %s)", node->name, ip_str);

    while (!node->should_exit || !*node->should_exit)
    {
        if (!node->input_queue)
        {
            printL(ERROR, INITIATOR, "%s: No input queue", node->name);
            break;
        }

        packet_size = pop(node->input_queue, buffer);

        if (packet_size < 1)
        {
            if (packet_size == 0)
            {
                continue;
            }

            printL(ERROR, INITIATOR, "%s: pop error", node->name);
            break;
        }

        if (packet_size < 46)
        {
            continue;
        }

        if (is_ip_packet(buffer))
        {
            modify_destination_ip(buffer, ctx->new_ip);
        }

        node_send_to_outputs(node, buffer, packet_size);
    }

    printL(INFO, INITIATOR, "IP Modifier node stopped: %s", node->name);
    return NULL;
}
