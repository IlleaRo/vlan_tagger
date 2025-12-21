#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "queue.h"

static void assert_true(const int condition, const char *message) {
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(EXIT_FAILURE);
    }
}

static void test_init_state(void) {
    Queue_t queue = {};

    assert_true(init(&queue) == 0, "init should succeed");
    assert_true(is_empty(&queue) == 1, "new queue is empty");
    assert_true(is_full(&queue) == 0, "new queue is not full");

    assert_true(queue_destroy(&queue) == 0, "queue_destroy should succeed");
}

static void test_push_pop_fifo(void) {
    Queue_t queue;
    init(&queue);

    uint8_t first[] = "first";
    uint8_t second[] = "second";
    uint8_t buffer[MAX_PKG_SIZE] = {0};

    assert_true(push(&queue, first, sizeof(first)) == (ssize_t)sizeof(first), "push first element");
    assert_true(push(&queue, second, sizeof(second)) == (ssize_t)sizeof(second), "push second element");

    assert_true(front(&queue, buffer) == (ssize_t)sizeof(first), "front returns first element");
    assert_true(memcmp(buffer, first, sizeof(first)) == 0, "front content matches first element");

    assert_true(back(&queue, buffer) == (ssize_t)sizeof(second), "back returns last element");
    assert_true(memcmp(buffer, second, sizeof(second)) == 0, "back content matches last element");

    memset(buffer, 0, sizeof(buffer));
    assert_true(pop(&queue, buffer) == (ssize_t)sizeof(first), "pop returns first element size");
    assert_true(memcmp(buffer, first, sizeof(first)) == 0, "pop preserves FIFO order");

    memset(buffer, 0, sizeof(buffer));
    assert_true(pop(&queue, buffer) == (ssize_t)sizeof(second), "pop returns second element size");
    assert_true(memcmp(buffer, second, sizeof(second)) == 0, "second element content matches");

    queue_destroy(&queue);
}

static void test_full_queue_guard(void) {
    Queue_t queue;
    init(&queue);

    uint8_t value = 0;
    for (int i = 0; i < Q_SIZE; i++) {
        value = (uint8_t)i;
        assert_true(push(&queue, &value, sizeof(value)) == 1, "push fills queue");
    }

    assert_true(is_full(&queue) == 1, "queue reports full after Q_SIZE pushes");

    value = 123;
    assert_true(push(&queue, &value, sizeof(value)) == 0, "push returns 0 when queue is full");

    uint8_t buffer = 0;
    assert_true(pop(&queue, &buffer) == 1, "pop succeeds after full queue");
    assert_true(buffer == 0, "FIFO preserved after full queue cycle");

    queue_destroy(&queue);
}

int main(void) {
    test_init_state();
    test_push_pop_fifo();
    test_full_queue_guard();

    printf("All queue tests passed.\n");
    return EXIT_SUCCESS;
}
