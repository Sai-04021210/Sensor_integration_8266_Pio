// ESP32 to NodeMCU UART Communication
#define RXD2 16  // ESP32 RX2 -> NodeMCU TX
#define TXD2 17  // ESP32 TX2 -> NodeMCU RX
#define UART_BAUD 115200

void setup() {
  // USB Serial for debugging
  Serial.begin(115200);
  Serial.println("ESP32 UART Master Started");
  
  // UART2 for NodeMCU communication
  Serial2.begin(UART_BAUD, SERIAL_8N1, RXD2, TXD2);
  
  delay(2000);
  Serial.println("Ready to communicate with NodeMCU");
}

void loop() {
  // Send periodic data to NodeMCU
  static unsigned long lastSend = 0;
  static int counter = 0;
  
  if (millis() - lastSend > 3000) {  // Every 3 seconds
    String message = "ESP32 Msg #" + String(counter++) + "\n";
    Serial2.print(message);
    Serial.print("Sent: " + message);
    lastSend = millis();
  }
  
  // Receive from NodeMCU
  if (Serial2.available()) {
    String received = "";
    while (Serial2.available()) {
      char c = Serial2.read();
      received += c;
      if (c == '\n') break;
    }
    Serial.print("Received: " + received);
    
    // Process commands from NodeMCU
    if (received.indexOf("TEMP") >= 0) {
      float temp = 25.5 + random(-10, 10) / 10.0;  // Simulated temperature
      String response = "TEMP:" + String(temp) + "\n";
      Serial2.print(response);
      Serial.print("Responded: " + response);
    }
  }
  
  // Forward USB Serial to NodeMCU
  if (Serial.available()) {
    String input = Serial.readString();
    Serial2.print(input);
    Serial.print("Forwarded: " + input);
  }
}