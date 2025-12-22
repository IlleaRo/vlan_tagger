# VLAN Tagger - Pipeline Framework

**Гибкий фреймворк для создания конвейеров обработки сетевых пакетов**

## Описание

VLAN Tagger - это многопоточный демон на базе pipeline framework для обработки сетевых пакетов. Позволяет создавать гибкие конвейеры из различных узлов обработки пакетов с поддержкой параллельного выполнения.

## Доступные узлы (Nodes)

| Узел | Описание |
|------|----------|
| **Sniffer** | Захватывает пакеты с сетевого интерфейса через RAW socket |
| **Tagger** | Добавляет VLAN теги (802.1Q) согласно правилам из конфига |
| **Sender** | Отправляет обработанные пакеты обратно в сеть |
| **PacketCounter** | Собирает статистику о пакетах (количество, размер, VLAN/non-VLAN) |
| **Logger** | Выводит hex dump содержимого пакетов для отладки |
| **Collector** | Объединяет данные от нескольких источников (упрощает pipeline паттерны) |
| **Dispatcher** | Распределяет пакеты между воркерами (автоматически создается в `pipeline_connect_shared`) |
| **IpModifier** | Модифицирует IP-адреса в пакетах с пересчётом контрольных сумм |

**ВАЖНОЕ ПРАВИЛО:** Все узлы (кроме Sender) **ОБЯЗАНЫ** пробрасывать данные дальше через `node_send_to_outputs()`, даже если они выполняют только аналитику. Это гарантирует, что данные не потеряются в pipeline.

## Архитектура

```
Sniffer -> [Tagger1, Tagger2] -> PacketCounter -> Sender
```

Каждый узел:
- Работает в отдельном потоке (pthread)
- Имеет входную очередь для приёма пакетов
- Может отправлять пакеты в несколько выходных очередей
- Логирует свои действия в общий файл

### Режимы соединения узлов

**1. Дублирование данных** (`pipeline_connect`)
```
     Sniffer
        │
    ┌───┴───┐
  queue1  queue2    ← Две очереди
    │       │
 Tagger1  Tagger2   ← Оба получают копии
```
```c
pipeline_connect(pipeline, sniffer, tagger1);
pipeline_connect(pipeline, sniffer, tagger2);
```

**2. Балансировка нагрузки** (`pipeline_connect_shared`)
```
     Sniffer
        │
      queue
        │
    Dispatcher      ← Автоматически создается
        │
    ┌───┴───┐
 queue1  queue2     ← Отдельные очереди
    │       │
 Tagger1  Tagger2   ← Каждый получает свою порцию (round-robin)
```
```c
Node* workers[] = {tagger1, tagger2};
pipeline_connect_shared(pipeline, sniffer, workers, 2);
// Автоматически создаст Dispatcher_Sniffer node
```

**Архитектурный принцип:**
- Каждая нода имеет **ровно одну** входную очередь
- Dispatcher управляет распределением пакетов
- Стратегии распределения: Round-Robin (по умолчанию), Random

**Когда использовать:**
- Дублирование: логирование + обработка, мониторинг
- Балансировка: worker pool, увеличение производительности, параллельная обработка

## Быстрый старт

### 1. Сборка проекта

```bash
mkdir build
cd build
cmake ..
make
```

Будет создан бинарник `vlan_tagger`

### 1.1. Генерация документации (Doxygen)

```bash
cd build
cmake --build . --target doc
```

HTML окажется в `build/docs/html/index.html`. Конфигурация берёт исходники из дерева проекта и складывает артефакты в каталог сборки, поэтому рабочая директория для команды — `build/`.

### 2. Создание конфигурационного файла

Создайте файл `vlan-tagger.cfg` в директории `build/`:

```
# Формат: IP_START-IP_END-VLAN_ID
0.0.0.0-10.1.0.0-100
10.2.0.0-10.4.255.255-200
192.168.0.0-192.168.255.255-300
```

**Формат файла:**
- `IP_START` - начало диапазона IP-адресов (включительно)
- `IP_END` - конец диапазона (включительно)
- `VLAN_ID` - номер VLAN тега (0-4095)
- Поддерживаются комментарии (строки, начинающиеся с `#`) и пустые строки

### 3. Создание тестового окружения

Для тестирования нужен сетевой интерфейс. Создадим пару veth:

```bash
sudo ./netns_blackhole.sh up
```

Это создаст:
- Интерфейс `veth0` (10.10.0.1) в основном namespace
- Интерфейс `veth1` (10.10.0.2) в изолированном namespace `blackhole`

### 4. Запуск демона

```bash
cd build
sudo ./vlan_tagger veth0 veth1
```

Аргументы:
- `veth0` (первый) - source interface для Sniffer
- `veth1` (второй) - destination interface для Sender

## Ручное тестирование

### Отправка тестовых пакетов

```bash
# Из основного namespace
ping -I veth0 10.10.0.2 -c 100
```

### Мониторинг логов

```bash
tail -f build/log
```

Пример вывода:
```
22.11.2025 14:30:15 [INFO] INIT: === DEBUG MODE STARTED ===
22.11.2025 14:30:15 [INFO] SNIF: Sniffer node started: Sniffer (socket fd=3)
22.11.2025 14:30:15 [INFO] TAGG: Tagger1 started with 3 rules
22.11.2025 14:30:15 [INFO] SEND: Sender node started: Sender (socket fd=3)
22.11.2025 14:30:20 [INFO] UNK:  PacketCounter: 100 packets | 9800 bytes | VLAN: 50, non-VLAN: 50
```

### Захват пакетов с VLAN тегами

```bash
sudo tcpdump -i veth0 -e -n vlan
```

Пример вывода с VLAN тегом:
```
14:30:20.123456 aa:bb:cc:dd:ee:ff > ff:ff:ff:ff:ff:ff, ethertype 802.1Q (0x8100), length 102: vlan 100, p 0, ethertype IPv4, 10.10.0.1 > 10.10.0.2: ICMP echo request
```

## Пример конфигурации pipeline

В файле `patterns.c`:

```c
// Создание узлов
Node* sniffer = sniffer_node_create("Sniffer", &sock_r, &saddr, &saddr_len);
Node* tagger1 = tagger_node_create("Tagger1", global_tag_rules, size);
Node* tagger2 = tagger_node_create("Tagger2", global_tag_rules, size);
Node* counter = packet_counter_node_create("PacketCounter", 100);
Node* sender = sender_node_create("Sender", &sock_r, &saddr, &saddr_len);

// Добавление в pipeline
```

**Вариант 1: Дублирование (каждый tagger обрабатывает все пакеты)**
```c
pipeline_connect(pipeline, sniffer, tagger1);
pipeline_connect(pipeline, sniffer, tagger2);
pipeline_connect(pipeline, tagger1, sender);
pipeline_connect(pipeline, tagger2, sender);
```

**Вариант 2: Worker pool (пакеты распределяются между tagger'ами)**
```c
Node* workers[] = {tagger1, tagger2};
pipeline_connect_shared(pipeline, sniffer, workers, 2);  // Балансировка
pipeline_connect(pipeline, tagger1, sender);
pipeline_connect(pipeline, tagger2, sender);
```

## Остановка демона

```bash
./kill_daemon.sh
```

Скрипт `kill_daemon.sh` корректно останавливает все процессы демона и показывает их PID.

## Очистка тестового окружения

```bash
sudo ./netns_blackhole.sh down
```

## Требования

- Linux (протестировано на WSL2)
- GCC
- CMake = 3.25
- pthread
- Права root для работы с RAW сockets

## Пример работы

До обработки демоном:
```
0000   a8 a1 59 48 07 8b 54 c2 50 76 20 b8 08 00 45 a0
       [MAC адреса................] [Type] [IP...]
```

После обработки (добавлен VLAN тег 100 = 0x0064):
```
0000   a8 a1 59 48 07 8b 54 c2 50 76 20 b8 81 00 00 64
       [MAC адреса................] [VLAN Tag ] [Type]

0010   08 00 45 a0 ...
       [IP...]
```

Добавились 4 байта: `81 00 00 64` где:
- `81 00` - TPID (Tag Protocol Identifier, 802.1Q)
- `00 64` - TCI (VLAN ID = 100)

## Лицензия

Проект разработан в образовательных целях.
