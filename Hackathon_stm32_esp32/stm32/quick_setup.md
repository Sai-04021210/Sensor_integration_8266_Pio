# STM32-ESP32 UART Communication

## New ESP32 Setup Steps

1. Connect ESP32 to computer via USB
2. Find ESP32 port: `ls /dev/cu.*` (look for usbserial-XXXX)
3. Build project: `pio run`
4. Upload to ESP32: `pio run --target upload`
5. Monitor output: `pio device monitor --baud 115200`

## Hardware Connections
STM32 PB10 (TX) → ESP32 GPIO 3 (RX)
STM32 GND → ESP32 GND

## Troubleshooting
Port busy error: `lsof | grep usbserial-XXXX` then `kill [PID]`
No ESP32 found: Try different USB cable/port
No data: Check wiring and baud rate (115200)

## Expected Output
```
rx_data - Hello from STM32!
rx_data - Data packet: 0
```
