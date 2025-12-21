#!/bin/bash
set -euo pipefail

# Usage:
#   SRC_IF=veth0 DST_IF=veth1 ./vlan_tagger_test.sh
# If DST_IF is not set, SRC_IF will be used for both sniffer and sender.

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
BIN_PATH="$BUILD_DIR/vlan_tagger"
CFG_PATH="$BUILD_DIR/vlan-tagger.cfg"
LOG_PATH="$BUILD_DIR/log"

SRC_IF="${SRC_IF:-veth0}"
DST_IF="${DST_IF:-$SRC_IF}"

require_cmd() {
    if ! command -v "$1" >/dev/null 2>&1; then
        echo "Required command not found: $1"
        exit 1
    fi
}

require_cmd ip
require_cmd ping

HAS_TCPDUMP=1
if ! command -v tcpdump >/dev/null 2>&1; then
    HAS_TCPDUMP=0
    echo "⚠️  tcpdump not found. Packet capture will be skipped."
fi

cleanup() {
    if [ -n "${DAEMON_PID:-}" ] && kill -0 "$DAEMON_PID" 2>/dev/null; then
        echo "Stopping daemon (PID: $DAEMON_PID)..."
        sudo kill -9 "$DAEMON_PID" >/dev/null 2>&1 || true
    fi
}

echo "╔══════════════════════════════════════════════════════════╗"
echo "║          VLAN Tagger Pipeline - Test Script               ║"
echo "╚══════════════════════════════════════════════════════════╝"
echo "  src: $SRC_IF | dst: $DST_IF"
echo ""

trap cleanup EXIT

if [ ! -x "$BIN_PATH" ]; then
    echo "Binary not found: $BIN_PATH"
    echo "Build first: mkdir -p build && cd build && cmake .. && make"
    exit 1
fi

if [ ! -f "$CFG_PATH" ]; then
    cat > "$CFG_PATH" <<'EOF'
# Формат: IP_START-IP_END-VLAN_ID
0.0.0.0-10.1.0.0-100
10.10.0.1-10.30.255.255-200
192.168.0.0-192.168.255.255-300
EOF
    echo "Created default config at $CFG_PATH"
fi

echo "[1/6] Checking interfaces..."
if ! ip link show "$SRC_IF" >/dev/null 2>&1; then
    echo "  ✗ Interface $SRC_IF not found. Run: sudo ./netns_blackhole.sh up"
    exit 1
fi
if ! ip link show "$DST_IF" >/dev/null 2>&1; then
    echo "  ✗ Interface $DST_IF not found in current namespace."
    echo "    Use DST_IF=$SRC_IF or bring interface to this namespace."
    exit 1
fi
echo "  ✓ Interfaces are available"

echo ""
echo "[2/6] Stopping old daemons..."
if pgrep -x vlan_tagger >/dev/null 2>&1; then
    sudo "$SCRIPT_DIR/kill_daemon.sh" >/dev/null 2>&1 || true
    sleep 1
fi
echo "  ✓ Cleaned up"

echo ""
echo "[3/6] Starting daemon (auto-daemonizes)..."
: > "$LOG_PATH"
START_OUTPUT=$(sudo bash -c "cd '$BUILD_DIR' && ./vlan_tagger '$SRC_IF' '$DST_IF'" 2>&1 || true)
echo "  $START_OUTPUT"

DAEMON_PID=$(echo "$START_OUTPUT" | awk '/Daemon started with PID:/ {print $5}')
sleep 2

if [ -z "${DAEMON_PID:-}" ] || ! kill -0 "$DAEMON_PID" 2>/dev/null; then
    DAEMON_PID=$(pgrep -x vlan_tagger | head -n 1 || true)
fi

if [ -z "${DAEMON_PID:-}" ]; then
    echo "  ✗ Daemon failed to start"
    [ -f "$LOG_PATH" ] && { echo "Log tail:"; tail -20 "$LOG_PATH"; }
    exit 1
fi
echo "  ✓ Daemon running (PID: $DAEMON_PID)"

echo ""
echo "[4/6] Sending test packets (20 pings)..."
ping -I "$SRC_IF" 10.10.0.2 -c 20 -i 0.1 >/dev/null 2>&1 &
PING_PID=$!

echo ""
echo "[5/6] Capturing VLAN packets..."
sleep 1
if [ "$HAS_TCPDUMP" -eq 1 ]; then
    CAPTURED=$(sudo timeout 4 tcpdump -i "$SRC_IF" -e -n -c 10 vlan 2>&1 || true)
else
    CAPTURED=""
fi
wait "$PING_PID" 2>/dev/null || true

echo ""
echo "[6/6] Results"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

if [ "$HAS_TCPDUMP" -eq 0 ]; then
    echo "⚠️  tcpdump not installed, skipped packet capture."
    echo ""
    echo "Log output (last 10 lines):"
    tail -10 "$LOG_PATH" | sed 's/^/  /'
elif echo "$CAPTURED" | grep -q "vlan"; then
    echo "✅ VLAN tags detected!"
    echo ""
    echo "Sample packets:"
    echo "$CAPTURED" | grep "vlan" | head -3 | sed 's/^/  /'
    echo ""
    echo "Log output (last 10 lines):"
    tail -10 "$LOG_PATH" | sed 's/^/  /'
    echo ""
    echo "🎉 Pipeline is working correctly!"
else
    echo "❌ VLAN tags not detected!"
    echo ""
    echo "Captured output:"
    echo "$CAPTURED" | sed 's/^/  /'
    echo ""
    echo "Log tail:"
    tail -20 "$LOG_PATH" | sed 's/^/  /'
fi

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "To stop daemon manually: sudo pkill vlan_tagger"
