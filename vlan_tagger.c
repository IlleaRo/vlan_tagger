#include "vlan_tagger.h"
#include "logger/logger.h"

static unsigned char buffer[1522] = { 0 };
static unsigned char second_buffer[1522] = { 0 };

short define_tag_for_ip(uint32_t addr, const tag_rules_t *tag_rules_obj, int size)
{
    if (tag_rules_obj == NULL)
    {
        return -1;
    }

    if (size == 0)
    {
        return -2;
    }

    for (int i = 0; i < size; ++i)
    {

        if (ntohl(tag_rules_obj[i].ip_left.s_addr) <= addr &&
            ntohl(tag_rules_obj[i].ip_right.s_addr) >= addr)
        {
            return (tag_rules_obj)[i].tag;
        }
    }

    return -3;
}

ssize_t read_packet(struct thread_data *params)
{
    ssize_t packet_size = 0;
    packet_size = pop(params->sniffer_queue, buffer);

    if (packet_size == -1)
    {
        return -1;
    }

    if (packet_size < 46)
    {
        printL(WARNING, TAGGER, "The packet is the wrong size. Discarded");
        return 0;
    }

    return packet_size;
}

int analyze_packet(void)
{
    short type_field = 0;
    type_field = ((0x0000 | (0xff & buffer[12])) << 8) | (0xff & buffer[13]);

    if (type_field <= 1500)
    {
        printL(INFO, TAGGER, "The packet format is not Etnernet II");
        return -1;
    }

    if (type_field == 0x8100)
    {
        printL(INFO, TAGGER, "The packet is already tagged");
        return -1;
    }

    if (type_field != 0x0800)
    {
        printL(INFO, TAGGER, "The protocol of the packet is not ip");
        return -2;
    }

    return 0;
}

uint32_t get_packet_ip(void)
{
    uint32_t addr = 0;

    for (int i = 30; i < 34; i++)
    {
        if (i != 33)
        {
            addr = (addr | buffer[i]) << 8;
        }
        else
        {
            addr = (addr | buffer[i]);
        }
    }

    return addr;
}

unsigned short packet_editor(short tag, unsigned short packet_size)
{
    for (int i = 0; i < 12; i++)
    {
        second_buffer[i] = buffer[i];
    }

    second_buffer[12] = 0x81;  // first byte of TPID
    second_buffer[13] = 0x00;  // second byte of TPID
    second_buffer[14] = 0 | 0; // PCP
    second_buffer[14] <<= 1;
    second_buffer[14] = second_buffer[14] | 0; // DEI
    second_buffer[14] <<= 4;
    second_buffer[14] = second_buffer[14] | ((0x0f00 & tag) >> 8); // first 3 bits of VID
    second_buffer[15] = 0x00ff & tag;                              // VID

    for (int i = 12; i < packet_size; i++)
    {
        second_buffer[i + 4] = buffer[i];
    }

    packet_size += 4;

    return packet_size;
}

void *tagger(void *thread_data)
{
    if (!thread_data)
    {
        pthread_exit(NULL);
    }

    short tag = 0;
    uint32_t addr = 0;
    ssize_t packet_size = 0;
    struct thread_data *params = (struct thread_data *)thread_data;

    if (params->tag_rules_obj == NULL)
    {
        printL(ERROR, TAGGER, "Configuration file not specified (error code: %d)!", -1);
        params->should_exit = 1;
        send_signal_queue(params->sender_queue);
        pthread_exit(NULL);
    }

    if (!params->tag_rules_obj)
    {
        printL(ERROR, TAGGER, "tag_rules_obj is not exist");
        params->should_exit = 1;
        send_signal_queue(params->sender_queue);
        pthread_exit(NULL);
    }

    while (!params->should_exit)
    {
        if ((packet_size = read_packet(params)) < 1)
        {
            if (packet_size == 0)
            {
                continue;
            }

            printL(ERROR, TAGGER, "pop error");
            params->should_exit = 1;
            send_signal_queue(params->sender_queue);
            pthread_exit(NULL);
        }

        if (analyze_packet() != 0)
        {
            continue;
        }

        addr = get_packet_ip();

        switch (tag = define_tag_for_ip(addr, 
                          (const tag_rules_t *)params->tag_rules_obj, 
                          params->tag_rules_size))
        {
            case -1:
            {
                printL(ERROR, TAGGER, "No list of tagging rules");
                params->should_exit = 1;
                send_signal_queue(params->sender_queue);
                pthread_exit(NULL);
            }
            case -2:
            {
                printL(ERROR, TAGGER, "The list of tagging rules does not contain any rules");
                params->should_exit = 1;
                send_signal_queue(params->sender_queue);
                pthread_exit(NULL);
            }
            case -3:
            {
                continue;
            }
        }

        packet_size = packet_editor(tag, packet_size);

        if (push(params->sender_queue, second_buffer, packet_size) != packet_size)
        {
            printL(WARNING, TAGGER, "Queue entry failed");
        }
    }

    pthread_exit(NULL);
}
