#include "../include/sender_node.h"
#include "../../logger/logger.h"
#include "../../common.h"
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include "socket_utils.h"
#include <linux/if_ether.h>

#ifndef VLAN_ETH_FRAME_LEN
#define VLAN_ETH_FRAME_LEN 1518
#endif // VLAN_ETH_FRAME_LEN

Node *sender_node_create(const char *name, const char *dev_name) {
    if (!name || !dev_name) {
        return NULL;
    }

    sender_context_t *ctx = calloc(1, sizeof(sender_context_t));
    if (!ctx) {
        goto __clear_and_exit;
    }

    ctx->socket = setup_packet_socket(dev_name, SENDER, &ctx->saddr_ll);
    if (ctx->socket == -1) {
        goto __clear_and_exit;
    }

    Node *node = node_create(name, NODE_TYPE_SENDER, sender_node_process, ctx);
    if (!node) {
        goto __clear_and_exit;
    }

    return node;

__clear_and_exit:
    if (ctx) {
        if (ctx->socket >= 0) {
            close(ctx->socket);
        }

        free(ctx);
    }

    return NULL;
}

void *sender_node_process(void *node_ptr) {
#ifdef DEBUG
    if (!node_ptr) {
        return NULL;
    }
#endif // DEBUG

    Node *node = node_ptr;
    sender_context_t *ctx = node->context;

#ifdef DEBUG
    if (!ctx) {
        printL(ERROR, SENDER, "%s: Invalid context", node->name);
        return NULL;
    }
#endif

    uint8_t buffer[ETHERNET_FRAME_LENGTH] = {0};
    int err_counter = 0;

    printL(INFO, SENDER, "Sender node started: %s", node->name);

    while (!node->should_exit || !*node->should_exit) {
        if (!node->input_queue) {
            printL(ERROR, SENDER, "%s: No input queue", node->name);
            break;
        }

        const ssize_t rcv_len = pop(node->input_queue, buffer);

        if (rcv_len >= VLAN_ETH_FRAME_LEN) {
            continue;
        }

        if (rcv_len < 0) {
            if (node->should_exit && *node->should_exit) {
                break;
            }

            continue;
        }

        const ssize_t bytes_sent = sendto(ctx->socket, buffer, rcv_len, 0,
                                          (struct sockaddr *) &ctx->saddr_ll, sizeof(ctx->saddr_ll));

        if (bytes_sent == -1) {
            if (err_counter == 3) {
                printL(ERROR, SENDER, "%s: Error sending packets (error code: %d)!", node->name, errno);
                if (node->should_exit) {
                    *node->should_exit = 1;
                }
                return NULL;
            }

            err_counter++;
            printL(WARNING, SENDER, "%s: Error sending packets (error code: %d)!", node->name, errno);
        } else {
            err_counter = 0;
        }
    }

    printL(INFO, SENDER, "Sender node stopped: %s", node->name);

    close(ctx->socket);

    return NULL;
}
