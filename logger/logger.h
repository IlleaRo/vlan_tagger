#ifndef logger_h
#define logger_h

typedef enum log_type
{
    INFO,
    ERROR,
    WARNING
} Type_e;

typedef enum msg_sdr
{
    INITIATOR,
    PARSER,
    SNIFFER,
    TAGGER,
    SENDER
} Sender_e;

int start_log(void); // Инициализация, открытие log-файла

int stop_log(void); // Завершение работы с log-файлом

// Форматированный ввод в log-файл
int printL(
        Type_e,
        Sender_e,
        const char *,
        ...);

#endif //logger_h
