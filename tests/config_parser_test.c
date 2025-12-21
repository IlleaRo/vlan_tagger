#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "config_parser.h"

static void assert_true(const int condition, const char *message) {
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(EXIT_FAILURE);
    }
}

static char *write_temp_config(const char *content) {
    char template[] = "/tmp/vlan_tagger_cfgXXXXXX";
    const int fd = mkstemp(template);
    if (fd == -1) {
        return NULL;
    }

    const size_t expected = strlen(content);
    const ssize_t written = write(fd, content, expected);
    close(fd);

    if (written < 0 || (size_t)written != expected) {
        unlink(template);
        return NULL;
    }

    return strdup(template);
}

static void cleanup_temp_config(char *path) {
    if (path != NULL) {
        unlink(path);
        free(path);
    }
}

static void test_valid_rules(void) {
    const char *config =
        "# comment line\n"
        "10.0.0.1-10.0.0.10-100\n"
        "172.16.0.5-200\n"
        "192.168.0.0-192.168.0.10-300\n";

    char *path = write_temp_config(config);
    assert_true(path != NULL, "temp config created");

    tag_rules_t rules;
    const int result = tag_rules_load(&rules, path);
    assert_true(result == 3, "three rules loaded");

    struct in_addr addr;
    inet_aton("10.0.0.1", &addr);
    assert_true(rules.rules[0].ip_left.s_addr == addr.s_addr, "first rule left bound");
    inet_aton("10.0.0.10", &addr);
    assert_true(rules.rules[0].ip_right.s_addr == addr.s_addr, "first rule right bound");
    assert_true(rules.rules[0].tag == 100, "first rule vlan tag");

    inet_aton("172.16.0.5", &addr);
    assert_true(rules.rules[1].ip_left.s_addr == addr.s_addr, "single IP rule left");
    assert_true(rules.rules[1].ip_right.s_addr == addr.s_addr, "single IP rule duplicated to right");
    assert_true(rules.rules[1].tag == 200, "single IP rule tag");

    inet_aton("192.168.0.0", &addr);
    assert_true(rules.rules[2].ip_left.s_addr == addr.s_addr, "third rule left bound");
    inet_aton("192.168.0.10", &addr);
    assert_true(rules.rules[2].ip_right.s_addr == addr.s_addr, "third rule right bound");
    assert_true(rules.rules[2].tag == 300, "third rule vlan tag");

    cleanup_temp_config(path);
}

static void test_invalid_vlan_id(void) {
    const char *config = "10.0.0.1-10.0.0.2-5000\n";
    char *path = write_temp_config(config);
    assert_true(path != NULL, "temp config created");

    tag_rules_t rules;
    const int result = tag_rules_load(&rules, path);
    assert_true(result == -13, "VLAN ID out of range triggers error -13");

    cleanup_temp_config(path);
}

static void test_overlapping_ranges(void) {
    const char *config =
        "10.0.0.0-10.0.0.5-10\n"
        "10.0.0.4-10.0.0.10-20\n";
    char *path = write_temp_config(config);
    assert_true(path != NULL, "temp config created");

    tag_rules_t rules;
    const int result = tag_rules_load(&rules, path);
    assert_true(result == -12, "overlapping ranges trigger error -12");

    cleanup_temp_config(path);
}

int main(void) {
    test_valid_rules();
    test_invalid_vlan_id();
    test_overlapping_ranges();

    printf("All config_parser tests passed.\n");
    return EXIT_SUCCESS;
}
