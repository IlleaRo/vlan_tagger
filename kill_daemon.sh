#!/bin/bash

daemon_name=vlan_tagger

pids=$(pgrep -f "$daemon_name")

if [ -z "$pids" ]; then
    echo "Нет активных процессов для демона: $daemon_name"
    exit 1
fi

for pid in $pids; do
    echo "Остановка процесса с PID: $pid"
    kill $pid
done



echo "Все процессы демона $daemon_name были остановлены."
