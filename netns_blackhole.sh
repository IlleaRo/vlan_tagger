#!/bin/bash
set -e

NS=blackhole
VETH_HOST=veth0
VETH_NS=veth1
HOST_IP=10.10.0.1/24
NS_IP=10.10.0.2/24
NS_GW=10.10.0.1

up() {
    echo "[+] Создаю namespace $NS..."
    sudo ip netns add "$NS" 2>/dev/null || echo "Namespace уже существует, продолжаю..."

    echo "[+] Создаю veth-пару $VETH_HOST <-> $VETH_NS..."
    if ! ip link show "$VETH_HOST" >/dev/null 2>&1; then
        sudo ip link add "$VETH_HOST" type veth peer name "$VETH_NS"
    else
        echo "Интерфейс $VETH_HOST уже существует, пропускаю создание."
    fi

    echo "[+] Перемещаю $VETH_NS в namespace $NS..."
    sudo ip link set "$VETH_NS" netns "$NS" 2>/dev/null || echo "$VETH_NS уже в namespace?"

    echo "[+] Настраиваю host-интерфейс $VETH_HOST..."
    sudo ip addr flush dev "$VETH_HOST" || true
    sudo ip addr add "$HOST_IP" dev "$VETH_HOST" 2>/dev/null || echo "IP уже назначен?"
    sudo ip link set "$VETH_HOST" up

    echo "[+] Настраиваю интерфейс внутри namespace..."
    sudo ip netns exec "$NS" ip addr flush dev "$VETH_NS" || true
    sudo ip netns exec "$NS" ip addr add "$NS_IP" dev "$VETH_NS" 2>/dev/null || echo "IP уже назначен?"
    sudo ip netns exec "$NS" ip link set "$VETH_NS" up

    echo "[+] Добавляю default route внутри namespace..."
    sudo ip netns exec "$NS" ip route replace default via "$NS_GW"

    echo "[✓] Готово. Можно работать!"


    echo -n "[?] Запустить Wireshark на eth0? [y/N]: "
    # Проверяем, что истёк таймаут или введено пусто
    if ! read -r -t 5 answer || [ -z "$answer" ]; then
        echo "Нет ответа — выходим."
        exit 0
    fi

    # Если ответил положительно — запускаем
    case "$answer" in
        y|Y|yes|YES)
            echo "Запускаю Wireshark..."

            # 1. Wireshark в хосте
            sudo wireshark -i veth0 -k > /dev/null 2>&1 &

            # 2. Wireshark в namespace
            sudo ip netns exec "$NS" \
                env DISPLAY="$DISPLAY" XAUTHORITY="$XAUTHORITY" \
                nohup wireshark -i veth1 -k > /dev/null 2>&1 &

            ;;
        *)
            echo "Отменено пользователем."
            ;;
    esac
}

down() {
    echo "[-] Удаляю namespace $NS (если есть)..."
    if ip netns list | grep -q "$NS"; then
        sudo ip netns del "$NS"
    else
        echo "Namespace $NS не существует — пропускаю."
    fi

    echo "[-] Удаляю интерфейс $VETH_HOST (если есть)..."
    if ip link show "$VETH_HOST" >/dev/null 2>&1; then
        sudo ip link del "$VETH_HOST"
    else
        echo "Интерфейс $VETH_HOST не найден — пропускаю."
    fi

    echo "[✓] Откат завершён."
}

case "$1" in
    up)
        up
        ;;
    down)
        down
        ;;
    restart)
        down
        up
        ;;
    *)
        echo "Использование: $0 {up|down|restart}"
        exit 1
        ;;
esac
