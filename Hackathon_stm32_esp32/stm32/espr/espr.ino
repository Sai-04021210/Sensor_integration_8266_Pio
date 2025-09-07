// ESP32 Code - Using Serial2 for STM32, Serial for monitor
// RX2 = GPIO16, TX2 = GPIO17 for STM32 communication
// Serial remains free for debugging

void setup() {
    Serial.begin(115200);      // For Serial Monitor
    Serial2.begin(115200);     // For STM32 (pins 16,17)
    
    Serial.println("Ready to receive data from STM32");
}

void loop() {
    if (Serial2.available()) {
        String data = Serial2.readString();
        data.trim();
        
        // Print received data to Serial Monitor
        Serial.println(data);
    }
    
    delay(10);
}