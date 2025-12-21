#include "stdlib.h"
#include "queue/queue.h"
#include "logger/logger.h"
#include "vlan_tagger.h"
#include "pthread_init.h"

#include <sys/socket.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <string.h>
#include <errno.h>
#include <sys/resource.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <net/if.h>

static thread_data func_params;

void logging_programm_completion(struct thread_data *params)
{
    if (!params)
    {
        printL(ERROR, INITIATOR, "Incorrect shutdown!");
        stop_log();
        exit(EXIT_FAILURE);
    }

    params->should_exit = 1;

    send_signal_queue(params->sender_queue);
    send_signal_queue(params->sniffer_queue);

    close(*params->socket);
    printL(INFO, INITIATOR, "Socket closed.");

    queue_destroy(params->sniffer_queue);
    printL(INFO, INITIATOR, "Sniffer queue destroyed.");

    queue_destroy(params->sender_queue);
    printL(INFO, INITIATOR, "Sender queue destroyed.");

    tag_rules_t *pTagRules = params->tag_rules_obj;

    tag_rules_clear(&pTagRules);
    printL(INFO, INITIATOR, "The memory allocated for the file config structure is cleared.");

    stop_log();
}

void signal_handler(int signal)
{
    logging_programm_completion(&func_params);
    exit(EXIT_SUCCESS);
}

void pthread_init(const char *interface_name)
{
    int sock_r;
    int saddr_len;
    int res;
    int size;
    Queue_t sniffer_queue;
    Queue_t sender_queue;
    pthread_t tid[THREADS_COUNT];
    tag_rules_t *rules = NULL;
    struct sockaddr_ll saddr = { 0 };
    struct rlimit rlim;

    rlim.rlim_cur = 8 * 1024 * 1024; // 8 MB in bytes
    rlim.rlim_max = 8 * 1024 * 1024; // 8 MB in bytes

    start_log();

    if (setrlimit(RLIMIT_STACK, &rlim) != 0)
    {
        printL(ERROR, INITIATOR, "Error setting stack size limit");
        stop_log();
        exit(EXIT_FAILURE);
    }

    if (signal(SIGINT, signal_handler) == SIG_ERR)
    {
        printL(ERROR, INITIATOR, "Signal %s processing error", "SIGINT");
        stop_log();
        exit(EXIT_FAILURE);
    }

    if (signal(SIGSEGV, signal_handler) == SIG_ERR)
    {
        printL(ERROR, INITIATOR, "Signal %s processing error", "SIGSEGV");
        stop_log();
        exit(EXIT_FAILURE);
    }

    if (signal(SIGTERM, signal_handler) == SIG_ERR)
    {
        printL(ERROR, INITIATOR, "Signal %s processing error", "SIGTERM");
        stop_log();
        exit(EXIT_FAILURE);
    }

    sock_r = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sock_r < 0)
    {
        printL(ERROR, INITIATOR, "Socket opening error (error code: %d)!", errno);
        stop_log();
        exit(EXIT_FAILURE);
    }

    // Получаем индекс сетевого интерфейса по его имени
    struct ifreq ifr;

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, interface_name, sizeof(ifr.ifr_name));

    if (ioctl(sock_r, SIOCGIFINDEX, &ifr) == -1)
    {
        printL(ERROR, INITIATOR, "Error with interface id");
        close(sock_r);
        exit(EXIT_FAILURE);
    }

    // Привязываем соксет к конкретному интерфейсу
    struct sockaddr_ll sa;
    
    memset(&sa, 0, sizeof(sa));

    sa.sll_family = AF_PACKET;
    sa.sll_protocol = htons(ETH_P_ALL);
    sa.sll_ifindex = ifr.ifr_ifindex;

    if (bind(sock_r, (struct sockaddr *)&sa, sizeof(sa)) == -1)
    {
        printL(ERROR, INITIATOR, "Error setting up network interface for socket (error code: %d)!", errno);
        close(sock_r);
        exit(EXIT_FAILURE);
    }

    saddr.sll_family = AF_PACKET;
    saddr.sll_protocol = htons(ETH_P_ALL);

    if (init(&sniffer_queue) == -1)
    {
        printL(ERROR, INITIATOR, "Queue initialization error (error code: %d)", errno);
        close(sock_r);
        exit(EXIT_FAILURE);
    }

    if (init(&sender_queue) == -1)
    {
        printL(ERROR, INITIATOR, "Queue initialization error (error code: %d)", errno);
        close(sock_r);
        exit(EXIT_FAILURE);
    }

    res = config_file_check();
    if (res != 0)
    {
        printL(ERROR, PARSER, "Error opening/closing config file (error code: %d)!", res);
        close(sock_r);
        queue_destroy(&sniffer_queue);
        queue_destroy(&sender_queue);
        exit(EXIT_FAILURE);
    }

    res = tag_rules_init(&rules, MAX_NUM_OF_RUL);
    if (res != 0)
    {
        printL(ERROR, PARSER, "Error in allocating memory for the configuration file structure (error code: %d)!", res);
        close(sock_r);
        queue_destroy(&sniffer_queue);
        queue_destroy(&sender_queue);
        exit(EXIT_FAILURE);
    }

    size = config_file_read(rules, MAX_NUM_OF_RUL);
    if (size < 0)
    {
        printL(ERROR, PARSER, "Error reading file/writing data to structure (error code: %d)!", size);
        close(sock_r);
        queue_destroy(&sniffer_queue);
        queue_destroy(&sender_queue);
        tag_rules_clear(&rules);
        exit(EXIT_FAILURE);
    }

    res = tag_rules_check_collisions((const tag_rules_t *)rules, size);
    if (res != 0)
    {
        printL(ERROR, PARSER, "Error when checking the configuration file for collisions (error code: %d)!", res);
        close(sock_r);
        queue_destroy(&sniffer_queue);
        queue_destroy(&sender_queue);
        tag_rules_clear(&rules);
        exit(EXIT_FAILURE);
    }

    func_params.socket = &sock_r;
    func_params.sniffer_queue = &sniffer_queue;
    func_params.sender_queue = &sender_queue;
    func_params.saddr = &saddr;
    saddr_len = sizeof(saddr);
    func_params.saddr_len = &saddr_len;
    func_params.tag_rules_obj = rules;
    func_params.tag_rules_size = size;
    func_params.should_exit = 0;

    pthread_create(&tid[0], NULL, packet_sniffer, &func_params);
    printL(INFO, INITIATOR, "Thread №%d started", 0);

    pthread_create(&tid[1], NULL, tagger, &func_params);
    printL(INFO, INITIATOR, "Thread №%d started", 1);

    pthread_create(&tid[2], NULL, packet_sender, &func_params);
    printL(INFO, INITIATOR, "Thread №%d started", 2);

    for (int j = 0; j < THREADS_COUNT; j++)
    {
        pthread_join(tid[j], NULL);
        printL(INFO, INITIATOR, "Thread №%d completed", j);
    }

    logging_programm_completion(&func_params);
}
