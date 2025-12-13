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

    if (global_tag_rules.rules)
    {
        tag_rules_clear(&global_tag_rules.rules);
        printL(INFO, INITIATOR, "Tag rules cleared.");
    }

    stop_log();
}

static int fill_tag_rules(tag_rules_t *tag_rules_ptr) {
    if (config_file_check() != 0)
    {
        printL(ERROR, PARSER, "Error opening/closing config file!");
        return 1;
    }

    if (tag_rules_init(&tag_rules_ptr->rules, 64) != 0)
    {
        printL(ERROR, PARSER, "Error allocating memory for tag rules!");
        return 1;
    }

    const int size = config_file_read(tag_rules_ptr->rules, 64);
    if (size < 0)
    {
        printL(ERROR, PARSER, "Error reading config file!");
        return 1;
    }

    if (tag_rules_check_collisions(tag_rules_ptr->rules, size) != 0)
    {
        printL(ERROR, PARSER, "Error checking config file for collisions!");
        return 1;
    }

    tag_rules_convert_to_host_order(tag_rules_ptr->rules, size);

    tag_rules_ptr->size = size;

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

    if (fill_tag_rules(&global_tag_rules)) {
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
