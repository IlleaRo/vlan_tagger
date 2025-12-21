#include "vlan_tagger.h"
#include "logger/logger.h"
#include "errno.h"

#include <string.h>

void* packet_sender(void *thread_data)
{
    if (!thread_data)
    {
        pthread_exit(NULL);
    }

    long bytes_read;
    unsigned char buffer[ETHERNET_FRAME_LENGTH] = { 0 };
    struct thread_data* params = (struct thread_data *) thread_data;

    int err_counter = 0;

    while(!params->should_exit)
    {
        bytes_read = pop(params->sender_queue, buffer);

        if (bytes_read >= 1518)
        {
            memset(buffer, 0, ETHERNET_FRAME_LENGTH);
            continue;
        }

        printf("Получено из очереди %ld байт\n", bytes_read);

        ssize_t bytes_sent = sendto(*params->socket, buffer, bytes_read, 0,
                                 (struct sockaddr *)params->saddr, *params->saddr_len);

        if (bytes_sent == -1)
        {
            if (err_counter == 3)
            {
                printL(ERROR, SENDER, "Error sending packets (error code: %d)!", errno);
                params->should_exit = 1;
                send_signal_queue(params->sniffer_queue);
                pthread_exit(NULL);
            }

            err_counter++;

            printL(WARNING, SENDER, "Error sending packets (error code: %d)!", errno);
        }
        else
        {
            err_counter = 0;
            printf("Отправлено %ld байт\n", bytes_sent);
        }

        memset(buffer, 0, ETHERNET_FRAME_LENGTH);

    }
    pthread_exit(NULL);
}