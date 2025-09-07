
## PlatformIO Setup Flow

1. Install VS Code
2. Install PlatformIO extension in VS Code
3. Open project folder in VS Code
4. Connect ESP32 via USB
5. Check port: `ls /dev/cu.*`
6. Build: `pio run`
7. Upload: `pio run --target upload`
8. Monitor: `pio device monitor --baud 115200`
9. Connect STM32 hardware
10. See data flowing
