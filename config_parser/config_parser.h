#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include <netinet/in.h>

#define MAX_RULES 128

typedef struct {
    struct in_addr ip_left;
    struct in_addr ip_right;
    int tag;
} tag_rule_t;

typedef struct {
    tag_rule_t rules[MAX_RULES];
    int size;
} tag_rules_t;

/**
 * Загружает и валидирует правила тегирования из файла.
 *
 * @param tag_rules указатель на структуру для записи правил
 * @param config_path путь к конфигурационному файлу
 * @return количество загруженных правил (>= 0) или код ошибки (< 0)
 *
 * Коды ошибок:
 *  -1: невалидные аргументы
 *  -2: не удалось открыть файл
 *  -3: ошибка чтения файла
 *  -4: ошибка парсинга строки
 *  -5: неверный формат правила (количество дефисов)
 *  -6: превышен лимит правил (MAX_RULES)
 *  -7: ошибка парсинга токена
 *  -8: невалидный IP адрес
 *  -9: невалидный VLAN ID
 * -10: ошибка закрытия файла
 * -11: ip_left > ip_right в правиле
 * -12: пересечение диапазонов IP
 * -13: VLAN ID вне диапазона 0-4095
 */
int tag_rules_load(tag_rules_t *tag_rules, const char *config_path);

#endif
