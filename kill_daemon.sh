#!/bin/bash

daemon_name=vlan_tagger

# Ищем только процессы с именем vlan_tagger
pids=$(pgrep -x "$daemon_name")

if [ -z "$pids" ]; then
    echo "Нет активных процессов для демона: $daemon_name"
    exit 1
fi

for pid in $pids; do
    echo "Остановка процесса с PID: $pid"
    sudo kill -9 "$pid"
done

echo "Все процессы демона $daemon_name были остановлены."
