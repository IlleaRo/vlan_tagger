#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "logger.h"

extern const char *FILE_LOG_NAME;

static void assert_true(const int condition, const char *message) {
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(EXIT_FAILURE);
    }
}

static void read_file_to_buffer(const char *path, char *buffer, size_t size) {
    FILE *f = fopen(path, "r");
    assert_true(f != NULL, "log file opened");

    const size_t read = fread(buffer, 1, size - 1, f);
    buffer[read] = '\0';
    fclose(f);
}

static void run_simple_write_test(void) {
    assert_true(start_log() == 0, "start_log succeeds");

    int written = printL(INFO, INITIATOR, "hello %s", "world");
    assert_true(written > 0, "first printL returns positive");

    written = printL(ERROR, TAGGER, "tag failure %d", 42);
    assert_true(written > 0, "second printL returns positive");

    assert_true(stop_log() == 0, "stop_log succeeds");

    char content[2048];
    read_file_to_buffer(FILE_LOG_NAME, content, sizeof(content));

    assert_true(strstr(content, "[INFO]") != NULL, "INFO entry present");
    assert_true(strstr(content, "INIT: hello world") != NULL, "INIT message content");
    assert_true(strstr(content, "[ERR]") != NULL, "ERROR entry present");
    assert_true(strstr(content, "TAGG: tag failure 42") != NULL, "TAGGER message content");
}


int main(void) {
    unlink(FILE_LOG_NAME);

    run_simple_write_test();

    printf("All logger tests passed.\n");
    return EXIT_SUCCESS;
}
