#include "sniffer_node.h"
#include "../logger/logger.h"
#include "../common.h"
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <linux/if_ether.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/socket.h>


Node *sniffer_node_create(const char *name, const char *dev_name) {
#ifdef DEBUG
    if (!name || !dev_name) {
        return NULL;
    }
#endif // DEBUG

    sniffer_context_t *ctx = calloc(1, sizeof(sniffer_context_t));
    if (!ctx) {
        goto __clear_and_exit;
    }

    ctx->socket = -1;

#ifdef DEBUG
    const size_t dev_name_len = strlen(dev_name);

    if (dev_name_len >= IFNAMSIZ || dev_name_len < 1) {
        goto __clear_and_exit;
    }
#endif // DEBUG

    ctx->socket = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (ctx->socket < 0) {
        printL(ERROR, INITIATOR, "Socket opening error (error code: %d)!", errno);

        goto __clear_and_exit;
    }

    struct ifreq ifr = {};
    strncpy(ifr.ifr_name, dev_name, IFNAMSIZ - 1);
    ifr.ifr_name[IFNAMSIZ - 1] = '\0';

    if (ioctl(ctx->socket, SIOCGIFINDEX, &ifr) == -1) {
        printL(ERROR, INITIATOR, "Error with interface id");

        goto __clear_and_exit;
    }

    ctx->saddr.sll_family = AF_PACKET;
    ctx->saddr.sll_protocol = htons(ETH_P_ALL);
    ctx->saddr.sll_ifindex = ifr.ifr_ifindex;

    if (bind(ctx->socket, (struct sockaddr *) &ctx->saddr, sizeof(ctx->saddr)) == -1) {
        printL(ERROR, INITIATOR, "Error setting up network interface for socket (error code: %d)!", errno);

        goto __clear_and_exit;
    }

    Node *node = node_create(name, NODE_TYPE_SNIFFER, sniffer_node_process, ctx);
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

void *sniffer_node_process(void *node_ptr) {
#ifdef DEBUG
    if (!node_ptr) {
        return NULL;
    }
#endif // DEBUG

    Node *node = node_ptr;
    sniffer_context_t *ctx = node->context;

#ifdef DEBUG
    if (!ctx) {
        printL(ERROR, SNIFFER, "Invalid context in sniffer node");
        return NULL;
    }
#endif // DEBUG

    uint8_t buffer[ETHERNET_FRAME_LENGTH] = {};
    uint8_t err_counter = 0;

    printL(INFO, SNIFFER, "Sniffer node started: %s (socket fd=%d)", node->name, ctx->socket);

    while (!node->should_exit || !*node->should_exit) {
        socklen_t saddr_len = sizeof(ctx->saddr); // recvfrom may change addr_len

        const ssize_t rcv_len = recvfrom(ctx->socket, buffer, sizeof(buffer), 0,
                                         (struct sockaddr *) &ctx->saddr,
                                         &saddr_len);

        if (rcv_len < 0) {
            if (err_counter >= MAX_ERR_NUM) {
                printL(ERROR, SNIFFER, "%s: Error receiving packets (error code: %d)!", node->name, errno);

                if (node->should_exit) {
                    *node->should_exit = 1;
                }

                close(ctx->socket);

                return NULL;
            }

            err_counter++;
            printL(WARNING, SNIFFER, "%s: Error receiving packets (error code: %d)!", node->name, errno);
        } else {
            err_counter = 0;
            node_send_to_outputs(node, buffer, rcv_len);
        }
    }

    printL(INFO, SNIFFER, "Sniffer node stopped: %s", node->name);

    close(ctx->socket);

    return NULL;
}
