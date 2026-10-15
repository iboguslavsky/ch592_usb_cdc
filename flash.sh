#!/usr/bin/env bash
set -e
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
BIN="$DIR/build/ch592_usb_cdc.bin"
[ -f "$BIN" ] || { echo "Building first..."; make -C "$DIR"; }
# Check if WCH ISP device is already present in USB topology (WITHOUT claiming the USB interface)
if ! lsusb 2>/dev/null | grep -q -E "4348:55e0|1a86:55e0"; then
    # Look for active CH592 USB CDC device
    CDC_PORT=$(ls /dev/serial/by-id/*CH592* 2>/dev/null | head -n 1 || true)
    if [ -n "$CDC_PORT" ] && [ -e "$CDC_PORT" ]; then
        echo "Found active CH592 CDC console at $CDC_PORT. Requesting buttonless ISP reboot..."
        python3 -c "
import serial, time
# Try 1200 bps touch first (universal for all usb_cdc projects)
try:
    s = serial.Serial('$CDC_PORT', 1200, timeout=0.2)
    s.close()
    time.sleep(0.1)
except Exception:
    pass
# Also send 'isp\r\n' in case the port is already open in a 115200 terminal/CLI
try:
    s = serial.Serial('$CDC_PORT', 115200, timeout=0.3)
    s.dtr = True
    time.sleep(0.05)
    s.write(b'isp\r\n')
    time.sleep(0.1)
    s.close()
except Exception:
    pass
" 2>/dev/null || true
        # Wait up to 5 seconds for ISP device (4348:55e0 or 1a86:55e0) to appear in lsusb
        for i in {1..10}; do
            sleep 0.5
            if lsusb 2>/dev/null | grep -q -E "4348:55e0|1a86:55e0"; then
                echo "CH592 successfully entered ISP bootloader mode!"
                sleep 0.3
                break
            fi
        done
    fi
fi

# Fallback instruction if still not in ISP mode
if ! lsusb 2>/dev/null | grep -q -E "4348:55e0|1a86:55e0"; then
    echo "CH592 not detected in ISP mode."
    echo "To enter ISP manually: bridge Pad 2 (BOOT/PB22) to Pad 1 (GND) while connecting USB."
fi

wchisp flash -V "$BIN"
echo "Done! Firmware flashed successfully."
