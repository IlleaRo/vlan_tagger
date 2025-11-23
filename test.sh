#!/bin/bash

echo "╔══════════════════════════════════════════════════════════╗"
echo "║          VLAN Tagger Pipeline - Test Script             ║"
echo "╚══════════════════════════════════════════════════════════╝"
echo ""

cd "$(dirname "$0")/build" || exit 1

# Stop old daemons
echo "[1/5] Stopping old daemons..."
pkill -9 vlan_tagger 2>/dev/null
sudo pkill -9 vlan_tagger 2>/dev/null
sleep 1
echo "  ✓ Stopped"

# Check veth0
echo ""
echo "[2/5] Checking veth0..."
if ! ip link show veth0 > /dev/null 2>&1; then
    echo "  ✗ veth0 not found!"
    echo "    Create it: sudo ../netns_blackhole.sh up"
    exit 1
fi
echo "  ✓ veth0 exists"

# Start daemon in background
echo ""
echo "[3/5] Starting daemon..."
sudo ./vlan_tagger veth0 &
sleep 2

if ! pgrep vlan_tagger > /dev/null; then
    echo "  ✗ Daemon failed to start!"
    echo "  Check log: cat $(pwd)/log"
    exit 1
fi
echo "  ✓ Daemon running (PID: $(pgrep vlan_tagger))"

# Send test packets
echo ""
echo "[4/5] Sending test packets (20 pings)..."
ping -I veth0 10.10.0.2 -c 20 -i 0.1 > /dev/null 2>&1 &
PING_PID=$!

# Capture VLAN packets
echo ""
echo "[5/5] Capturing VLAN packets..."
sleep 1
CAPTURED=$(sudo timeout 4 tcpdump -i veth0 -e -n -c 10 vlan 2>&1)

wait $PING_PID 2>/dev/null

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

if echo "$CAPTURED" | grep -q "vlan"; then
    echo "✅ SUCCESS! VLAN tags detected!"
    echo ""
    echo "Sample packets:"
    echo "$CAPTURED" | grep "vlan" | head -3 | sed 's/^/  /'
    echo ""
    echo "Log output (last 10 lines):"
    tail -10 log | sed 's/^/  /'
    echo ""
    echo "🎉 Pipeline is working correctly!"
else
    echo "❌ ERROR: VLAN tags not detected!"
    echo ""
    echo "Check log:"
    tail -20 log | sed 's/^/  /'
fi

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "To stop daemon: sudo pkill vlan_tagger"
