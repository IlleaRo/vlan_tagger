#ifndef queue_h
#define queue_h

#include <pthread.h>
#include <sys/types.h>


#define Q_SIZE 100 // Длина очереди
#define MAX_PKG_SIZE 1522 // Максимальная длина загружаемого пакета


// Структура очереди
typedef struct Queue
{
    unsigned char queue[Q_SIZE][MAX_PKG_SIZE]; // Массив, который содержит элементы очереди
    unsigned short pkg_sizes[Q_SIZE]; // Массив, который содержит размеры пакетов каждого элемента очереди
    unsigned short front; // позиция первого элемента
    unsigned short rear; // позиция для вставки нового элемента
    pthread_rwlock_t rw_lock; // Блокировка читателей/писателей
    pthread_mutex_t cond_mutex; // Защитный мьютекс для pthread_cond_signal
    pthread_cond_t condition; // Переменная состояния
} Queue_t;

// Инициализация очереди
int init(Queue_t *);

// Проверка наличия свободных мест в очереди
int is_full(Queue_t *);

// Проверка наличие элементов в очереди
int is_empty(Queue_t *);

// Добавление элемента в очередь в конец
ssize_t push(
        Queue_t *, // Очередь
        unsigned char *, // Пакет
        unsigned short); // Размер пакета

// Удаление первого элемента очереди
void remove_front(Queue_t *);

// Получить первый элемент из очереди и удалить его
ssize_t pop(
        Queue_t *, // Очередь
        unsigned char *); // Буфер, в который будет занесен пакет

//Получить первый элемент из очереди и записать его в буфер, функция возвращает кол-во записанных байт
ssize_t front(
        Queue_t *,
        unsigned char *);

//Записывает последний элемент в буфер и возвращает его размер
ssize_t back(
        Queue_t *, // Очередь
        unsigned char *); // Буфер, в который будет занесен пакет

// Завершение работы с очередью (уничтожение файлов мьютексов)
int queue_destroy(Queue_t *); 

void send_signal_queue(Queue_t *);

#endif //queue_h
