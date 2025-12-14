#include "queue.h"

#include <pthread.h>
#include <stdint.h>
#include <string.h>

// Получение индекса следующего элемента
static void next(uint16_t *num) {
#ifdef DEBUG
    if (!num) {
        return;
    }
#endif

    if (*num == Q_SIZE - 1) {
        *num = 0;
        return;
    }

    (*num)++;
}

// Инициализация очереди
int init(Queue_t *q) {
#ifdef DEBUG
    if (!q) {
        return -1;
    }
#endif

    if (pthread_rwlock_init(&q->rw_lock, NULL) != 0) {
        return -1;
    }

    if (pthread_mutex_init(&q->cond_mutex, NULL) != 0) {
        return -1;
    }

    if (pthread_cond_init(&q->condition, NULL)) {
        return -1;
    }

    q->front = 0;
    q->rear = 0;

    memset(q->sizes, 0, Q_SIZE);

    return 0;
}

int is_full(const Queue_t *q) {
    if (q->sizes[q->rear] == 0) {
        return 0;
    }

    return 1;
}

// Проверка на наличие элементов в очереди
int is_empty(const Queue_t *q) {
    if (q->sizes[q->front] == 0) {
        return 1;
    }

    return 0;
}

// Добавление элемента в конец очереди
ssize_t push(Queue_t *q, uint8_t *buf, const uint16_t size) {
#ifdef DEBUG
    if (!q || !buf) // Проверка корректности аргумента
    {
        return -1;
    }
#endif

    if (size > MAX_PKG_SIZE) {
        return -1;
    }

    if (size == 0) {
        return 0;
    }

    // Проверка на наличие свободных мест в очереди
    if (is_full(q)) {
        return 0;
    }

    pthread_rwlock_wrlock(&q->rw_lock);

    memcpy(q->queue[q->rear], buf, size);
    q->sizes[q->rear] = size;
    next(&q->rear);

    pthread_cond_signal(&q->condition);

    pthread_rwlock_unlock(&q->rw_lock);

    return size;
}

// Удаляет первый элемент из начала очереди
void remove_front(Queue_t *q) {
#ifdef DEBUG
    if (!q) {
        return;
    }
#endif

    if (is_empty(q)) {
        return;
    }

    pthread_rwlock_wrlock(&q->rw_lock);

    q->sizes[q->front] = 0;
    next(&q->front);

    pthread_rwlock_unlock(&q->rw_lock);
}

// Получить первый элемент из очереди и удалить его
ssize_t pop(Queue_t *q, uint8_t *buff) {
#ifdef DEBUG
    if (!q || !buff) {
        return -1;
    }
#endif

    pthread_mutex_lock(&q->cond_mutex);

    while (is_empty(q)) {
        if (pthread_cond_wait(&q->condition, &q->cond_mutex) != 0) {
            pthread_mutex_unlock(&q->cond_mutex);
            return -1;
        }
    }

    pthread_mutex_unlock(&q->cond_mutex);
    pthread_rwlock_wrlock(&q->rw_lock);

    memcpy(buff, q->queue[q->front], q->sizes[q->front]);
    const uint16_t size = q->sizes[q->front];
    q->sizes[q->front] = 0;
    next(&q->front);

    pthread_rwlock_unlock(&q->rw_lock);

    return size;
}


//Записывает первый элемент в буфер и возвращает его размер
ssize_t front(Queue_t *q, uint8_t *buff) {
#ifdef DEBUG
    if (!q || !buff) {
        return -1;
    }
#endif

    if (is_empty(q)) {
        return -1;
    }

    pthread_rwlock_rdlock(&q->rw_lock);

    const uint16_t size = q->sizes[q->front];
    memcpy(buff, q->queue[q->front], size);

    pthread_rwlock_unlock(&q->rw_lock);

    return size;
}

//Записывает последний элемент в буфер и возвращает его размер
ssize_t back(Queue_t *q, uint8_t *buff) {
#ifdef DEBUG
    if (!q || !buff) {
        return -1;
    }
#endif

    if (is_empty(q)) {
        return -1;
    }

    pthread_rwlock_rdlock(&q->rw_lock);

    uint16_t cursor;
    if (q->rear == 0) {
        cursor = Q_SIZE - 1;
    } else {
        cursor = q->rear - 1;
    }

    const uint16_t size = q->sizes[cursor];
    memcpy(buff, q->queue[cursor], size);

    pthread_rwlock_unlock(&q->rw_lock);

    return size;
}

int queue_destroy(Queue_t *q) {
#ifdef DEBUG
    if (!q) {
        return -1;
    }
#endif

    int res = 0;

    if (pthread_rwlock_destroy(&q->rw_lock) != 0) {
        res = -1;
    }

    if (pthread_mutex_destroy(&q->cond_mutex) != 0) {
        res = -1;
    }

    if (pthread_cond_destroy(&q->condition) != 0) {
        res = -1;
    }

    return res;
}

void send_signal_queue(Queue_t *q) {
    pthread_cond_signal(&q->condition);
}
