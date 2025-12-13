#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <net/if.h>
#include <sys/resource.h>

#include "logger/logger.h"
#include "pipeline/pipeline.h"
#include "nodes/include/tagger_node.h"
#include "config_parser/config_parser.h"
#include "pipeline_patterns/patterns.h"

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))

Pipeline global_pipeline = {};
tag_rules_t global_tag_rules = {};

const int handled_signals[] = {SIGINT, SIGSEGV, SIGTERM};

static void signal_handler(const int signal)
{
    printL(INFO, INITIATOR, "Received signal %d, stopping pipeline", signal);

    pipeline_stop(&global_pipeline);
}

static void cleanup(void)
{
    pipeline_destroy(&global_pipeline);

    tag_rules_destroy(&global_tag_rules);
    printL(INFO, INITIATOR, "Tag rules destroyed.");

    stop_log();
}

static int fill_tag_rules(tag_rules_t *tag_rules_ptr, const char *config_path) {
    // Инициализация
    if (tag_rules_init(tag_rules_ptr, 0) != 0)
    {
        printL(ERROR, PARSER, "Error initializing tag rules structure!");
        return 1;
    }

    // Загрузка
    const int loaded_count = tag_rules_load_from_file(tag_rules_ptr, config_path);
    if (loaded_count < 0)
    {
        printL(ERROR, PARSER, "Error loading config file (code: %d)!", loaded_count);
        tag_rules_destroy(tag_rules_ptr);
        return 1;
    }

    if (loaded_count == 0)
    {
        printL(WARNING, PARSER, "No rules loaded from config file!");
    }

    // Валидация
    const int validation_result = tag_rules_validate(tag_rules_ptr);
    if (validation_result != 0)
    {
        printL(ERROR, PARSER, "Config validation failed (errors: %d)!", validation_result);
        tag_rules_destroy(tag_rules_ptr);
        return 1;
    }

    printL(INFO, PARSER, "Loaded %d tag rules from %s", loaded_count, config_path);

    return 0;
}

int main(const int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "Usage: <src interface> <dst interface>\n");
        exit(EXIT_FAILURE);
    }

    const pid_t pid = fork();
    if (pid < 0)
    {
        perror("FORK");
        exit(EXIT_FAILURE);
    }

    if (pid > 0)
    {
        printf("Daemon started with PID: %d\n", pid);
        exit(EXIT_SUCCESS);
    }

    if (setsid() < 0)
    {
        perror("SETSID");
        exit(EXIT_FAILURE);
    }

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    struct rlimit rlim;
    rlim.rlim_cur = 8 * 1024 * 1024;
    rlim.rlim_max = 8 * 1024 * 1024;

    start_log();

    if (setrlimit(RLIMIT_STACK, &rlim) != 0)
    {
        printL(ERROR, INITIATOR, "Error setting stack size limit");

        cleanup();
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < ARRAY_SIZE(handled_signals); i++) {
        if (signal(handled_signals[i], signal_handler) == SIG_ERR) {
            printL(ERROR, INITIATOR, "Signal processing error");

            cleanup();
            exit(EXIT_FAILURE);
        }
    }

    if (fill_tag_rules(&global_tag_rules, "vlan-tagger.cfg")) {
        printL(ERROR, PARSER, "Error reading tag rules!");

        cleanup();
        exit(EXIT_FAILURE);
    }

    if (pipeline_init(&global_pipeline))
    {
        printL(ERROR, INITIATOR, "Failed to init pipeline");

        cleanup();
        exit(EXIT_FAILURE);
    }

    const vlan_tagger_context_t vlan_tagger_context = {
        .tag_rules = &global_tag_rules,
        .src_dev_name = argv[1],
        .dst_dev_name = argv[2]
    };

    if (pipeline_build(&global_pipeline, vlan_tagger_pattern, &vlan_tagger_context)) {
        printL(ERROR, INITIATOR, "Failed to build pipeline");

        cleanup();
        exit(EXIT_FAILURE);
    }

    printL(INFO, INITIATOR, "Pipeline configured:");
    printL(INFO, INITIATOR, "  Sniffer -> [Tagger1, Tagger2] -> PacketCounter -> Sender");

    if (pipeline_start(&global_pipeline) != 0)
    {
        printL(ERROR, INITIATOR, "Failed to start pipeline");

        cleanup();
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < global_pipeline.node_count; i++)
    {
        pthread_join(global_pipeline.nodes[i]->thread, NULL);
    }

    cleanup();
    exit(EXIT_SUCCESS);
}
