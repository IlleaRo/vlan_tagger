#include "queue.h"
#include <string.h>

// Получение индекса следующего элемента
void next(unsigned short *);

// Инициализация очереди
int init(Queue_t *q)
{
    if (!q)
    {
        return -1;
    }

    if (pthread_rwlock_init(&q->rw_lock, NULL) != 0)
    {
        return -1;
    }

    if (pthread_mutex_init(&q->cond_mutex, NULL) != 0 )
    {
        return -1;
    }

    if (pthread_cond_init(&q->condition, NULL))
    {
        return -1;
    }

    q->front = 0;
    q->rear = 0;

    for (int i = 0; i < Q_SIZE; ++i)
    {
        q->pkg_sizes[i] = 0;
    }

    return 0;
}

int is_full(Queue_t *q)
{
    if (q->pkg_sizes[q->rear] == 0)
    {
        return 0;
    }

    return 1;
}

// Проверка на наличие элементов в очереди
int is_empty(Queue_t *q)
{
    if (q->pkg_sizes[q->front] == 0)
    {
        return 1;
    }

    return 0;
}

// Добавление элемента в конец очереди
ssize_t push(
        Queue_t *q, // Очередь
        unsigned char *pkg, // Пакет
        unsigned short pkg_size) // Размер пакета
{
    if (!q || !pkg || pkg_size > MAX_PKG_SIZE) // Проверка корректности аргумента
    {
        return -1;
    }

    if (pkg_size == 0)
    {
        return 0;
    }

    // Проверка на наличие свободных мест в очереди
    if (!is_full(q))
    {
        pthread_rwlock_wrlock(&q->rw_lock);

        memcpy(q->queue[q->rear], pkg, pkg_size);
        q->pkg_sizes[q->rear] = pkg_size;
        next(&q->rear);

        pthread_cond_signal(&q->condition);

        pthread_rwlock_unlock(&q->rw_lock);

        return pkg_size;
    }

    return 0;
}

// Удаляет первый элемент из начала очереди
void remove_front(Queue_t *q)
{
    if (!q || is_empty(q))
    {
        return;
    }

    pthread_rwlock_wrlock(&q->rw_lock);

    q->pkg_sizes[q->front] = 0;
    next(&q->front);

    pthread_rwlock_unlock(&q->rw_lock);
}

// Получить первый элемент из очереди и удалить его
ssize_t pop(
        Queue_t *q,
        unsigned char *buff)
{
    if (!q || !buff)
    {
        return -1;
    }

    if (is_empty(q))
    {
        if (pthread_cond_wait(&q->condition, &q->cond_mutex) != 0)
        {
            pthread_mutex_unlock(&q->cond_mutex);
            return -1;
        }
        pthread_mutex_unlock(&q->cond_mutex);
    }

    pthread_rwlock_wrlock(&q->rw_lock);

    memcpy(buff, q->queue[q->front], q->pkg_sizes[q->front]);
    unsigned short read_bytes = q->pkg_sizes[q->front];
    q->pkg_sizes[q->front] = 0;
    next(&q->front);

    pthread_rwlock_unlock(&q->rw_lock);

    return read_bytes;
}


//Записывает первый элемент в буфер и возвращает его размер
ssize_t front(
        Queue_t *q,
        unsigned char *buff)
{
    if (!q || !buff || is_empty(q))
    {
        return -1;
    }

    pthread_rwlock_rdlock(&q->rw_lock);

    unsigned short read_bytes = q->pkg_sizes[q->front];
    memcpy(buff, q->queue[q->front], read_bytes);

    pthread_rwlock_unlock(&q->rw_lock);

    return read_bytes;
}

//Записывает последний элемент в буфер и возвращает его размер
ssize_t back(
        Queue_t *q,
        unsigned char *buff)
{
    if (!q || !buff || is_empty(q))
    {
        return -1;
    }

    pthread_rwlock_rdlock(&q->rw_lock);

    unsigned short cursor;
    if (q->rear == 0)
    {
        cursor = Q_SIZE-1;
    }
    else
    {
        cursor = q->rear - 1;
    }

    unsigned short read_bytes = q->pkg_sizes[cursor];
    memcpy(buff, q->queue[cursor], read_bytes);

    pthread_rwlock_unlock(&q->rw_lock);

    return read_bytes;
}

int queue_destroy(Queue_t *q)
{
    if (!q)
    {
        return -1;
    }

    int returned_value = 0;

    if (pthread_rwlock_destroy(&q->rw_lock) != 0)
    {
        returned_value = -1;
    }

    if (pthread_mutex_destroy(&q->cond_mutex) != 0)
    {
        returned_value = -1;
    }

    if (pthread_cond_destroy(&q->condition) != 0)
    {
        returned_value = -1;
    }

    return returned_value;
}

void next(unsigned short *num)
{
    if (!num)
    {
        return;
    }

    if (*num == Q_SIZE - 1)
    {
        *num = 0;
        return;
    }

    (*num)++;
}

void send_signal_queue(Queue_t *q)
{
    pthread_cond_signal(&q->condition);
}
