#include "vlan_tagger.h"
#include "queue/queue.h"
#include "logger/logger.h"
#include "errno.h"

void *packet_sniffer(void *thread_data)
{
    if (!thread_data)
    {
        pthread_exit(NULL);
    }

    long buffer_len;
    unsigned char buffer[ETHERNET_FRAME_LENGTH] = {0};
    struct thread_data *params = (struct thread_data *)thread_data;

    int err_counter = 0;
    long sent_bytes = 0;

    while (!params->should_exit)
    {
        buffer_len = recvfrom(*params->socket, buffer, sizeof(buffer), 0, 
                         (struct sockaddr *)params->saddr,
                         (socklen_t *)params->saddr_len);
        
        if (buffer_len < 0)
        {
            if (err_counter == 3)
            {
                printL(ERROR, SNIFFER, "Error receiving packets (error code: %d)!", errno);
                params->should_exit = 1;
                send_signal_queue(params->sniffer_queue);
                send_signal_queue(params->sender_queue);
                pthread_exit(NULL);
            }

            err_counter++;

            printL(WARNING, SNIFFER, "Error receiving packets (error code: %d)!", errno);
        }
        else
        {
            err_counter = 0;
            sent_bytes = push(params->sniffer_queue, buffer, buffer_len);
            printf("Отправлено в очередь %ld байт\n", sent_bytes);
        }
    }

    pthread_exit(NULL);
}
