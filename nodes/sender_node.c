#include "sender_node.h"
#include "../logger/logger.h"
#include "../common.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>

Node* sender_node_create(const char* name, int* socket, struct sockaddr_ll* saddr, int* saddr_len)
{
    if (!name || !socket || !saddr || !saddr_len)
    {
        return NULL;
    }

    SenderContext* ctx = (SenderContext*)calloc(1, sizeof(SenderContext));
    if (!ctx)
    {
        return NULL;
    }

    ctx->socket = socket;
    ctx->saddr = saddr;
    ctx->saddr_len = saddr_len;

    Node* node = node_create(name, NODE_TYPE_SENDER, sender_node_process, ctx);
    if (!node)
    {
        free(ctx);
        return NULL;
    }

    return node;
}

void* sender_node_process(void* node_ptr)
{
    if (!node_ptr)
    {
        return NULL;
    }

    Node* node = (Node*)node_ptr;
    SenderContext* ctx = (SenderContext*)node->context;

    if (!ctx || !ctx->socket || !ctx->saddr || !ctx->saddr_len)
    {
        printL(ERROR, SENDER, "%s: Invalid context", node->name);
        return NULL;
    }

    long bytes_read;
    unsigned char buffer[ETHERNET_FRAME_LENGTH] = {0};
    int err_counter = 0;

    printL(INFO, SENDER, "Sender node started: %s", node->name);

    while (!node->should_exit || !*node->should_exit)
    {
        if (!node->input_queue)
        {
            printL(ERROR, SENDER, "%s: No input queue", node->name);
            break;
        }

        bytes_read = pop(node->input_queue, buffer);

        if (bytes_read >= 1518)
        {
            memset(buffer, 0, ETHERNET_FRAME_LENGTH);
            continue;
        }

        if (bytes_read < 0)
        {
            if (node->should_exit && *node->should_exit)
            {
                break;
            }
            continue;
        }

        ssize_t bytes_sent = sendto(*ctx->socket, buffer, bytes_read, 0,
                                     (struct sockaddr *)ctx->saddr, *ctx->saddr_len);

        if (bytes_sent == -1)
        {
            if (err_counter == 3)
            {
                printL(ERROR, SENDER, "%s: Error sending packets (error code: %d)!", node->name, errno);
                if (node->should_exit)
                {
                    *node->should_exit = 1;
                }
                return NULL;
            }

            err_counter++;
            printL(WARNING, SENDER, "%s: Error sending packets (error code: %d)!", node->name, errno);
        }
        else
        {
            err_counter = 0;
        }

        memset(buffer, 0, ETHERNET_FRAME_LENGTH);
    }

    printL(INFO, SENDER, "Sender node stopped: %s", node->name);
    return NULL;
}
