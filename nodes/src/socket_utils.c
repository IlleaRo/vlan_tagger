#include "socket_utils.h"

#include <errno.h>
#include <net/if.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <linux/if_ether.h>

int setup_packet_socket(const char *dev_name, const Sender_e sender, struct sockaddr_ll *saddr_ll) {
    if (!dev_name || !saddr_ll) {
        return -1;
    }

    const size_t dev_name_len = strlen(dev_name);
    if (dev_name_len >= IFNAMSIZ || dev_name_len < 1) {
        printL(ERROR, sender, "Invalid interface name");
        return -1;
    }

    const int socket_fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (socket_fd == -1) {
        printL(ERROR, sender, "socket(AF_PACKET) failed (errno=%d)", errno);
        return -1;
    }

    struct ifreq ifr = {};
    strncpy(ifr.ifr_name, dev_name, IFNAMSIZ - 1);
    ifr.ifr_name[IFNAMSIZ - 1] = '\0';

    if (ioctl(socket_fd, SIOCGIFINDEX, &ifr) == -1 || ifr.ifr_ifindex == 0) {
        printL(ERROR, sender, "%s: can't find interface (errno=%d)", dev_name, errno);
        close(socket_fd);
        return -1;
    }

    memset(saddr_ll, 0, sizeof(*saddr_ll));
    saddr_ll->sll_family = AF_PACKET;
    saddr_ll->sll_protocol = htons(ETH_P_ALL);
    saddr_ll->sll_ifindex = ifr.ifr_ifindex;

    if (bind(socket_fd, (struct sockaddr *) saddr_ll, sizeof(*saddr_ll)) == -1) {
        printL(ERROR, sender, "Error binding socket to %s (errno=%d)", dev_name, errno);
        close(socket_fd);
        return -1;
    }

    return socket_fd;
}
