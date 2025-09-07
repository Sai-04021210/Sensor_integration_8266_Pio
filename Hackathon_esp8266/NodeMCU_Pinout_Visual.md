# NodeMCU ESP8266 Pinout - A0 Location

```
                    NodeMCU ESP8266 (USB at top)
                           ┌─────────┐
                           │   USB   │
                           └─────────┘
                               │
    LEFT SIDE                  │                  RIGHT SIDE
    ─────────                  │                  ──────────
    
    3V3  ●                     │                     ● VU (5V)
    GND  ●                     │                     ● GND
    TX   ●                     │                     ● 3V3
    RX   ●                     │                     ● S3
    D8   ●                     │                     ● S2
    D7   ●                     │                     ● S1
    D6   ●                     │                     ● SC
    D5   ●                     │                     ● S0
    GND  ●                     │                     ● SK
    3V3  ●                     │                     ● GND
    D4   ●                     │                     ● 3V3
    D3   ●                     │                     ● EN
    D2   ●                     │                     ● RST
    D1   ●                     │                     ● GND
    D0   ●                     │                     ● VIN
                               │                     ● A0  ← HERE!
                               │
                        ┌─────────────┐
                        │   ESP8266   │
                        │    CHIP     │
                        └─────────────┘
```

## 🎯 **A0 Pin Location:**

**A0 is on the RIGHT SIDE, at the BOTTOM** of the NodeMCU board!

- **Position**: Right side, bottom pin
- **Label**: Usually marked as "A0" on the board
- **Function**: Only analog input pin on ESP8266
- **Voltage Range**: 0-1V input (with voltage divider for 0-3.3V)

## 🔍 **How to Find A0:**

1. **Hold your NodeMCU** with USB connector at the top
2. **Look at the RIGHT side** of the board
3. **Find the bottom-most pin** on the right side
4. **It should be labeled "A0"** on the PCB

## 📏 **Physical Identification:**

- **Color**: Usually the same as other pins (black plastic connector)
- **Size**: Same size as digital pins
- **Marking**: Should have "A0" printed on the board nearby
- **Position**: Last pin on the right side when USB is at top

## ⚠️ **Important Notes:**

- **Only ONE analog pin**: A0 is the ONLY analog input on ESP8266
- **Voltage limit**: Maximum 1V input (higher voltages need voltage divider)
- **No pinMode needed**: A0 is analog input by default

Your rain sensor should connect:
- **Rain Sensor AO** → **NodeMCU A0** (bottom right pin)
- **Rain Sensor VCC** → **NodeMCU 3V3**
- **Rain Sensor GND** → **NodeMCU GND**

Can you locate the A0 pin on your board now? It should be the bottom-right pin when the USB is at the top! 📍
