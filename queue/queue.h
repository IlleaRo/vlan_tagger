#ifndef QUEUE_H
#define QUEUE_H

#include <stdint.h>
#include <sys/types.h>


#define Q_SIZE 1000 // Длина очереди (увеличено для параллельных узлов)
#define MAX_PKG_SIZE 1522 // Максимальная длина загружаемого пакета


// Структура очереди
typedef struct Queue {
    uint8_t queue[Q_SIZE][MAX_PKG_SIZE]; // Массив, который содержит элементы очереди
    uint16_t sizes[Q_SIZE]; // Массив, который содержит размеры пакетов каждого элемента очереди
    uint16_t front; // позиция первого элемента
    uint16_t rear; // позиция для вставки нового элемента
    pthread_rwlock_t rw_lock; // Блокировка читателей/писателей
    pthread_mutex_t cond_mutex; // Защитный мьютекс для pthread_cond_signal
    pthread_cond_t condition; // Переменная состояния
} Queue_t;

// Инициализация очереди
int init(Queue_t *);

// Добавление элемента в очередь в конец
ssize_t push(
    Queue_t *, // Очередь
    uint8_t *, // Пакет
    uint16_t); // Размер пакета

// Удаление первого элемента очереди
void remove_front(Queue_t *);

// Получить первый элемент из очереди и удалить его
ssize_t pop(
    Queue_t *, // Очередь
    uint8_t *); // Буфер, в который будет занесен пакет

//Получить первый элемент из очереди и записать его в буфер, функция возвращает кол-во записанных байт
ssize_t front(
    Queue_t *,
    uint8_t *);

//Записывает последний элемент в буфер и возвращает его размер
ssize_t back(
    Queue_t *, // Очередь
    uint8_t *); // Буфер, в который будет занесен пакет

// Завершение работы с очередью (уничтожение файлов мьютексов)
int queue_destroy(Queue_t *);

void send_signal_queue(Queue_t *);

#endif //QUEUE_H
