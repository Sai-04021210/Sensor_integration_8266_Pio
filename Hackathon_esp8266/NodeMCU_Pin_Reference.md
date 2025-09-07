# NodeMCU ESP8266 Pin Reference

## Digital Pins (GPIO)
| NodeMCU Pin | GPIO | Notes |
|-------------|------|-------|
| D0 | GPIO16 | Built-in LED, Wake from deep sleep |
| D1 | GPIO5  | I2C SCL |
| D2 | GPIO4  | I2C SDA |
| D3 | GPIO0  | Flash button, Boot mode |
| D4 | GPIO2  | Built-in LED (blue) |
| D5 | GPIO14 | SPI SCLK |
| D6 | GPIO12 | SPI MISO |
| D7 | GPIO13 | SPI MOSI |
| D8 | GPIO15 | SPI CS, Boot mode |
| D9 | GPIO3  | UART RX |
| D10| GPIO1  | UART TX |

## Power Pins
| Pin | Voltage | Description |
|-----|---------|-------------|
| 3V3 | 3.3V | 3.3V power output |
| VU  | 5V   | 5V power (when USB connected) |
| GND | 0V   | Ground |

## Analog Pin
| Pin | Description |
|-----|-------------|
| A0  | **LM35 Temperature Sensor** (our current setup), Analog input (0-1V, 10-bit ADC) |

## Current LM35 Temperature Sensor Setup
- **Analog Pin**: A0 (ADC0)
- **Power**: 3V3
- **Ground**: GND

## Common Sensor Connections
- **LM35 Temperature**: OUT → A0, VCC → 3V3, GND → GND
- **Rain Sensor (Analog)**: AO → A0, VCC → 3V3, GND → GND
- **Rain Sensor (Digital)**: DO → D4/D5/D6/D7, VCC → 3V3, GND → GND
- **DHT22/DHT11**: Data → D4/D5/D6/D7, VCC → 3V3, GND → GND
- **I2C Devices**: SDA → D2, SCL → D1
- **SPI Devices**: MOSI → D7, MISO → D6, SCLK → D5, CS → D8
