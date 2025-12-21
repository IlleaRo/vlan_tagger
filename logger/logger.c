#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdarg.h>
#include <pthread.h>
#include "logger.h"

#define FILE_LOG_NAME "log"
#define TIME_BUF_SIZE 80
#define USR_MSG_SIZE 200


static pthread_mutex_t mutex;
static FILE *file;


int start_log(void)
{
    if ((file = fopen(FILE_LOG_NAME, "a")) == NULL)
    {
        return -1;
    }

    if (pthread_mutex_init(&mutex,NULL) != 0)
    {
        return -1;
    }

    return 0;
}

int stop_log(void)
{
    if (fclose(file) == EOF)
    {
        return -1;
    }

    if (pthread_mutex_destroy(&mutex) != 0)
    {
        return -1;
    }

    return 0;
}

int get_time(char* buffer)
{
    time_t currentTime;
    struct tm *time_info;

    // Получить текущее время
    if ((currentTime = time(NULL)) == -1)
    {
        return -1;
    }

    // Преобразовать в структуру tm
    if ((time_info = localtime(&currentTime)) == NULL)
    {
        return -1;
    }

    // Определить формат вывода
    if (strftime(buffer, TIME_BUF_SIZE, "%d.%m.%Y %H:%M:%S", time_info) == 0)
    {
        return -1;
    }

    return 0;
}




int printL(
        Type_e msg_type,
        Sender_e sender,
        const char *format,
        ...)
{
    if (!format)
    {
        return -1;
    }

    char buffer[USR_MSG_SIZE] = { 0 };
    
    // Возвращаемое значение - количество записанных символов
    int returned_val = 0;

    // Работа с переменным числом аргументов
    va_list args;
    va_start(args, format);

    returned_val = vsnprintf(buffer, USR_MSG_SIZE, format, args);

    va_end(args);

    // Получаем время
    char time_buff[TIME_BUF_SIZE] = { 0 };
    if (get_time(time_buff) == -1)
    {
        return -2;
    }

    // Входим в критический участок
    pthread_mutex_lock(&mutex);

    if (fputs(time_buff, file) == EOF)
    {
        pthread_mutex_unlock(&mutex);
        return -3;
    }
    //Увеличиваем счетчик записанных символов
    returned_val += (int) strlen(time_buff);

    // Записываем тип сообщения
    switch (msg_type)
    {
        case INFO:
        {
            if (fputs("\t[INFO]\t", file) == EOF)
            {
                pthread_mutex_unlock(&mutex);
                return -3;
            }
            break;
        }
        case WARNING:
        {
            if (fputs("\t[WARN]\t", file) == EOF)
            {
                pthread_mutex_unlock(&mutex);
                return -3;
            }
            break;
        }
        case ERROR:
        {
            if (fputs("\t[ERR] \t", file) == EOF)
            {
                pthread_mutex_unlock(&mutex);
                return -3;
            }
            break;
        }
        default:
        {
            if (fputs("\t[INFO]\t", file) == EOF)
            {
                pthread_mutex_unlock(&mutex);
                return -3;
            }
        }
    }

    returned_val += 8;

    switch (sender)
    {
        case INITIATOR:
        {
            if (fputs("INIT: ", file) == EOF)
            {
                pthread_mutex_unlock(&mutex);
                return -3;
            }
            break;
        }
        case PARSER:
        {
            if (fputs("PARS: ", file) == EOF)
            {
                pthread_mutex_unlock(&mutex);
                return -3;
            }
            break;
        }
        case SNIFFER:
        {
            if (fputs("SNIF: ", file) == EOF)
            {
                pthread_mutex_unlock(&mutex);
                return -3;
            }
            break;
        }
        case TAGGER:
        {
            if (fputs("TAGG: ", file) == EOF)
            {
                pthread_mutex_unlock(&mutex);
                return -3;
            }
            break;
        }
        case SENDER:
        {
            if (fputs("SEND: ", file) == EOF)
            {
                pthread_mutex_unlock(&mutex);
                return -3;
            }
            break;
        }
        default:
        {
            if (fputs("UNK:  ", file) == EOF)
            {
                pthread_mutex_unlock(&mutex);
                return -3;
            }
        }
    }
    returned_val += 6;

    // Записываем само сообщение
    if (fputs(buffer, file) == EOF)
    {
        pthread_mutex_unlock(&mutex);
        return -3;
    }
    if (fputc('\n', file) == EOF)
    {
        pthread_mutex_unlock(&mutex);
        return -3;
    }

    pthread_mutex_unlock(&mutex);

    returned_val++;

    return returned_val;
}
