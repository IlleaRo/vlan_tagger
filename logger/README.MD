# Инструмент для логирования событий в файл
В данной папке представлена статическая библиотека, позволяющая вести логирование в файл. Ниже приведен пример
использования логгера:

```c
int main()
{
    //output: Symbols was written: 79
    //        Symbols was written: 103
    //        Symbols was written: 115
    
    //logfile:
    // 12.09.2023 23:15:12	[INFO] 	UNK:  This is just info msg! Have a good day! Oops!
    // 12.09.2023 23:15:12	[WARN]	PARS: This is a warning message! Maybe something will go wrong (error 404)!
    // 12.09.2023 23:15:12	[ERR] 	INIT: So now this is a fatal error! The program cannot continue its work. Bye bye! (32)

    start_log();

    printf("Symbols was written: %d\n", 
           printL(132, -43, "This is just info msg! Have a good day! %s","Oops!")); // Incorrect msg_type and source
    printf("Symbols was written: %d\n", 
           printL(WARNING, PARSER, "This is a warning message! Maybe something will go wrong (error %d)!", 404));
    printf("Symbols was written: %d\n", 
           printL(ERROR, INITIATOR, "So now this is a fatal error! The program cannot continue its work. Bye bye! (%2X)", '2'));
    
    
    stop_log();

    return 0;
}
```

Ниже приведена таблица с предоставляемыми функциями:

| Действие                                                     | Функция                                         | Возвращаемое значение                                                                                                                                                       |
|--------------------------------------------------------------|-------------------------------------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Запуск логирования (открытие файла, инициализация мьютекса)  | int start_log(void)                             | 0 - успешная инициализация</br>-1 - ошибка                                                                                                                                  |
| Остановка логирования (закрытие файла, уничтожение мьютекса) | int stop_log(void)                              | 0 - успешное завершение<br/>-1 - ошибка                                                                                                                                     |
| Отправка сообщения в логгер                                  | int printL(Type_e, Sender_e, const char *, ...) | -1 - передан пустой указатель<br/>-2 - ошибка получения времени<br/>-3 - ошибка записи в файл<br/>>=0 - количество символов, записанных файл (считая '\n' и не считая '\0') |

> При некорректной передачи Type_e статус сообщения INFO<br/>
> При некорректной передачи Sender_e отправитель UNK (UNKNOWN)