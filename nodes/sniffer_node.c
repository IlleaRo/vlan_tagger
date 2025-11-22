#include "sniffer_node.h"
#include "../logger/logger.h"
#include "../common.h"
#include <stdlib.h>
#include <errno.h>
#include <string.h>

Node* sniffer_node_create(const char* name, int* socket, struct sockaddr_ll* saddr, int* saddr_len)
{
    if (!name || !socket || !saddr || !saddr_len)
    {
        return NULL;
    }

    SnifferContext* ctx = (SnifferContext*)calloc(1, sizeof(SnifferContext));
    if (!ctx)
    {
        return NULL;
    }

    ctx->socket = socket;
    ctx->saddr = saddr;
    ctx->saddr_len = saddr_len;

    Node* node = node_create(name, NODE_TYPE_SNIFFER, sniffer_node_process, ctx);
    if (!node)
    {
        free(ctx);
        return NULL;
    }

    return node;
}

void* sniffer_node_process(void* node_ptr)
{
    if (!node_ptr)
    {
        return NULL;
    }

    Node* node = (Node*)node_ptr;
    SnifferContext* ctx = (SnifferContext*)node->context;

    if (!ctx || !ctx->socket || !ctx->saddr || !ctx->saddr_len)
    {
        printL(ERROR, SNIFFER, "Invalid context in sniffer node");
        return NULL;
    }

    long buffer_len;
    unsigned char buffer[ETHERNET_FRAME_LENGTH] = {0};
    int err_counter = 0;

    printL(INFO, SNIFFER, "Sniffer node started: %s (socket fd=%d)", node->name, *ctx->socket);

    while (!node->should_exit || !*node->should_exit)
    {
        buffer_len = recvfrom(*ctx->socket, buffer, sizeof(buffer), 0,
                             (struct sockaddr *)ctx->saddr,
                             (socklen_t *)ctx->saddr_len);

        if (buffer_len < 0)
        {
            if (err_counter == 3)
            {
                printL(ERROR, SNIFFER, "%s: Error receiving packets (error code: %d)!", node->name, errno);
                if (node->should_exit)
                {
                    *node->should_exit = 1;
                }
                return NULL;
            }

            err_counter++;
            printL(WARNING, SNIFFER, "%s: Error receiving packets (error code: %d)!", node->name, errno);
        }
        else
        {
            err_counter = 0;
            node_send_to_outputs(node, buffer, buffer_len);
        }
    }

    printL(INFO, SNIFFER, "Sniffer node stopped: %s", node->name);
}
