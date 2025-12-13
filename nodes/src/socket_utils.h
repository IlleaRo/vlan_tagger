#ifndef SOCKET_UTILS_H
#define SOCKET_UTILS_H

#include <linux/if_packet.h>
#include "../../logger/logger.h"

int setup_packet_socket(const char *, const Sender_e, struct sockaddr_ll *);

#endif // SOCKET_UTILS_H
