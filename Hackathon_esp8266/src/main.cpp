#include <Arduino.h>
#include "Bonezegei_DHT11.h"
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <time.h>

// WiFi credentials
const char* WIFI_SSID = "Ram";
const char* WIFI_PASS = "12345678";

// Server endpoint configuration
const char* SERVER_URL = "http://172.20.10.10:5000/api/weather/customstation";  // Your server endpoint
const unsigned long SEND_INTERVAL = 60000;  // 10 seconds in milliseconds

// Station configuration
const float LATITUDE = 49.8464;
const float LONGITUDE = 7.8440;
const String STATION_ID = "3924";
const String USER_ID = "30924";

// Web server on port 80 (backup local API)
ESP8266WebServer server(80);
String deviceIp = "";
String deviceId = "";

// HTTP client for posting data
WiFiClient wifiClient;
HTTPClient httpClient;

// Multi-Sensor Configuration
#define DHT1_PIN D4       // First DHT sensor on D4 (GPIO2)
#define DHT2_PIN D5       // Second DHT sensor on D5 (GPIO14)
#define LM35_PIN A0       // LM35 temperature sensor on A0 (analog)
#define RAIN_PIN D6       // Rain sensor digital output on D6 (GPIO12)
#define LED_PIN LED_BUILTIN

// Initialize DHT11 sensors
Bonezegei_DHT11 dht1(DHT1_PIN);
Bonezegei_DHT11 dht2(DHT2_PIN);

// Latest sensor values exposed via API
// Latest sensor values
volatile float last_dht1_temp = NAN;
volatile float last_dht1_hum = NAN;
volatile bool  last_dht1_ok = false;

volatile float last_dht2_temp = NAN;
volatile float last_dht2_hum = NAN;
volatile bool  last_dht2_ok = false;

volatile float last_lm35_temp = NAN;
volatile bool  last_rain_detected = false;
volatile unsigned long last_uptime = 0;

// Timing variables
unsigned long lastSendTime = 0;
unsigned long lastReadTime = 0;
const unsigned long READ_INTERVAL = 5000;  // Read sensors every 5 seconds

// Function declarations
void handleWeatherPost();
void handleRoot();
void connectWiFi();
void waitWithServer(unsigned long ms);
void readSensors();
void sendDataToServer();
String createJsonPayload();
String getTimestamp();
String getDeviceId();
// Simple root to show device IP and usage
void handleRoot() {
  String html = String("<html><body><h2>ESP8266 Weather Station</h2>") +
                "<p>IP: " + deviceIp + "</p>" +
                "<p>Use GET or POST: /weather (JSON)</p>" +
                "</body></html>";
  server.send(200, "text/html", html);
}

// Return latest readings on POST
void handleWeatherPost() {
  String json = "{";
  json += "\"ip\":\"" + deviceIp + "\",";
  json += "\"uptime_s\":" + String(last_uptime) + ",";
  json += "\"leafWetness\":{\"ok\":" + String(last_dht1_ok ? "true" : "false") + ",\"t\":" + String(last_dht1_temp,1) + ",\"h\":" + String(last_dht1_hum,1) + "},";
  json += "\"relativeHumidity\":{\"ok\":" + String(last_dht2_ok ? "true" : "false") + ",\"t\":" + String(last_dht2_temp,1) + ",\"h\":" + String(last_dht2_hum,1) + "},";
  json += "\"temperature\":" + String(last_lm35_temp,1) + ",";
  json += "\"precipitation\":\"" + String(last_rain_detected ? "WET" : "DRY") + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

void connectWiFi() {
  Serial.println("==========================================");
  Serial.println("WiFi Connection Debug Info:");
  Serial.print("SSID: '");
  Serial.print(WIFI_SSID);
  Serial.println("'");
  Serial.print("Password: '");
  Serial.print(WIFI_PASS);
  Serial.println("'");
  Serial.println("==========================================");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connecting to WiFi ");
  Serial.print(WIFI_SSID);
  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 20) {  // Reduced from 60 to 20
    delay(500);
    Serial.print(".");
    retries++;

    // Print status every 5 attempts
    if (retries % 5 == 0) {
      Serial.print(" [Status: ");
      Serial.print(WiFi.status());
      Serial.print("] ");
    }
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    deviceIp = WiFi.localIP().toString();
    deviceId = getDeviceId();
    Serial.println("✅ WiFi connected successfully!");
    Serial.print("IP: ");
    Serial.println(deviceIp);
    Serial.print("Device ID: ");
    Serial.println(deviceId);

    // Configure time for timestamps
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    Serial.println("Waiting for time sync...");
    while (time(nullptr) < 8 * 3600 * 2) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("\nTime synchronized");
  } else {
    Serial.print("❌ WiFi connection FAILED! Final status: ");
    Serial.println(WiFi.status());
    Serial.println("WiFi Status Codes:");
    Serial.println("0=IDLE, 1=NO_SSID, 3=CONNECTED, 4=CONNECT_FAILED, 6=DISCONNECTED");
    Serial.println("Continuing offline...");
  }
}

void waitWithServer(unsigned long ms) {
  unsigned long start = millis();
  while (millis() - start < ms) {
    server.handleClient();
    delay(1);
  }
}

// Generate unique device ID from MAC address
String getDeviceId() {
  String mac = WiFi.macAddress();
  mac.replace(":", "");
  return "ESP8266_" + mac;
}

// Get ISO timestamp
String getTimestamp() {
  time_t now = time(nullptr);
  struct tm* timeinfo = gmtime(&now);
  char buffer[32];
  strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", timeinfo);
  return String(buffer);
}

// Create JSON payload with sensor data
String createJsonPayload() {
  // Generate random values for non-measured fields
  float temperature2m = last_lm35_temp + random(-20, 50) / 10.0; // ±5°C variation
  float feelsLikeTemperature = temperature2m + random(-30, 30) / 10.0; // ±3°C variation
  float soilTemperature5cm = last_lm35_temp + random(-50, 50) / 10.0; // ±5°C variation
  float soilTemperature20cm = soilTemperature5cm + random(-10, 20) / 10.0; // slight variation
  float longitude = LONGITUDE + random(-1000, 1000) / 100000.0; // small random offset
  float latitude = LATITUDE + random(-1000, 1000) / 100000.0; // small random offset
  int elevation = 500 + random(-100, 200); // 400-700m elevation range

  String json = "{";
  json += "\"temperature\":" + String(last_lm35_temp, 1) + ",";
  json += "\"precipitation\":" + String(last_rain_detected ? 1 : 0) + ",";
  json += "\"leafWetness\":" + String(last_dht1_hum, 2) + ",";
  json += "\"relativeHumidity\":" + String(last_dht2_hum, 2) + ",";
  json += "\"stationId\":\"" + STATION_ID + "\",";
  json += "\"temperature2m\":" + String(temperature2m, 1) + ",";
  json += "\"feelsLikeTemperature\":" + String(feelsLikeTemperature, 1) + ",";
  json += "\"soilTemperature5cm\":" + String(soilTemperature5cm, 1) + ",";
  json += "\"soilTemperature20cm\":" + String(soilTemperature20cm, 1) + ",";
  json += "\"timestamp\":\"" + getTimestamp() + "\",";
  json += "\"location\":{";
  json += "\"type\":\"Point\",";
  json += "\"coordinates\":[" + String(longitude, 6) + "," + String(latitude, 6) + "],";
  json += "\"elevation\":" + String(elevation);
  json += "},";
  json += "\"userId\":\"" + USER_ID + "\"";
  json += "}";
  return json;
}

// Read all sensors and update global variables
void readSensors() {
  // Read DHT1 sensor
  float dht1_humidity = 0;
  float dht1_temperature = 0;
  bool dht1_success = false;
  if (dht1.getData()) {
    dht1_temperature = dht1.getTemperature();
    dht1_humidity = dht1.getHumidity();
    dht1_success = true;
  }

  // Read DHT2 sensor
  float dht2_humidity = 0;
  float dht2_temperature = 0;
  bool dht2_success = false;
  if (dht2.getData()) {
    dht2_temperature = dht2.getTemperature();
    dht2_humidity = dht2.getHumidity();
    dht2_success = true;
  }

  // Read LM35 temperature sensor (analog)
  int lm35_analog = analogRead(LM35_PIN);
  float lm35_voltage = (lm35_analog / 1024.0) * 1.0;
  float lm35_temperature = lm35_voltage * 100.0;

  // Read rain sensor (digital)
  int rain_status = digitalRead(RAIN_PIN);
  bool rain_detected = (rain_status == LOW);  // LOW = rain detected

  // Update global variables
  last_dht1_temp = dht1_temperature;
  last_dht1_hum = dht1_humidity;
  last_dht1_ok = dht1_success;
  last_dht2_temp = dht2_temperature;
  last_dht2_hum = dht2_humidity;
  last_dht2_ok = dht2_success;
  last_lm35_temp = lm35_temperature;
  last_rain_detected = rain_detected;
  last_uptime = millis() / 1000;

  // Print readings to serial
  Serial.print("⏰ Time: ");
  Serial.print(last_uptime);
  Serial.println("s");

  Serial.println("🌡️ WEATHER SENSORS:");
  Serial.println("--------------------");

  if (dht1_success) {
    Serial.print("📊 leafWetness: ");
    Serial.print(dht1_temperature, 1);
    Serial.print("°C, ");
    Serial.print(dht1_humidity, 1);
    Serial.println("% humidity");
  } else {
    Serial.println("❌ leafWetness: Failed to read");
  }

  if (dht2_success) {
    Serial.print("📊 relativeHumidity: ");
    Serial.print(dht2_temperature, 1);
    Serial.print("°C, ");
    Serial.print(dht2_humidity, 1);
    Serial.println("% humidity");
  } else {
    Serial.println("❌ relativeHumidity: Failed to read");
  }

  Serial.print("📊 temperature: ");
  Serial.print(lm35_temperature, 1);
  Serial.println("°C");

  Serial.println("🌧️ PRECIPITATION SENSOR:");
  Serial.println("------------------------");
  Serial.print("📊 precipitation: ");
  if (rain_status == HIGH) {
    Serial.println("☀️ NO PRECIPITATION (Dry)");
  } else {
    Serial.println("🌧️ PRECIPITATION DETECTED (Wet)");
  }

  Serial.println("📍 STATION INFORMATION:");
  Serial.println("-----------------------");
  Serial.print("📊 stationId: ");
  Serial.println(STATION_ID);
  Serial.print("📊 userId: ");
  Serial.println(USER_ID);
  Serial.print("📊 latitude: ");
  Serial.println(LATITUDE, 4);
  Serial.print("📊 longitude: ");
  Serial.println(LONGITUDE, 4);

  Serial.println("========================");
  Serial.println();
}

// Send data to server via HTTP POST
void sendDataToServer() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("❌ WiFi not connected, skipping server send");
    return;
  }

  String jsonPayload = createJsonPayload();

  Serial.println("📡 Sending data to server...");
  Serial.println("URL: " + String(SERVER_URL));
  Serial.println("Payload: " + jsonPayload);

  httpClient.begin(wifiClient, SERVER_URL);
  httpClient.addHeader("Content-Type", "application/json");
  httpClient.addHeader("User-Agent", "ESP8266-WeatherStation/1.0");

  int httpResponseCode = httpClient.POST(jsonPayload);

  if (httpResponseCode > 0) {
    String response = httpClient.getString();
    Serial.print("✅ Server response code: ");
    Serial.println(httpResponseCode);
    Serial.print("Response: ");
    Serial.println(response);
  } else {
    Serial.print("❌ HTTP POST failed, error: ");
    Serial.println(httpClient.errorToString(httpResponseCode));
  }

  httpClient.end();
  Serial.println("📡 Send complete");
  Serial.println();
}



void setup() {
  // Initialize serial communication at 115200 baud rate
  Serial.begin(115200);
  delay(1000);

  // Initialize the LED pin as an output
  pinMode(LED_PIN, OUTPUT);

  // Initialize rain sensor pin as input
  pinMode(RAIN_PIN, INPUT);

  // Print a welcome message
  Serial.println();
  Serial.println("==========================================");
  Serial.println("ESP8266 Multi-Sensor Weather Station");
  Serial.println("==========================================");
  Serial.println("Sensors connected:");
  Serial.println("- leafWetness");
  Serial.println("- relativeHumidity");
  Serial.println("- temperature");
  Serial.println("- precipitation");
  Serial.println();

  // Start WiFi and HTTP server (with timeout protection)
  Serial.println("Starting WiFi connection...");
  connectWiFi();

  if (WiFi.status() == WL_CONNECTED) {
    if (MDNS.begin("esp8266-weather")) {
      Serial.println("mDNS responder started (http://esp8266-weather.local)");
    }
    server.on("/", HTTP_GET, handleRoot);
    server.on("/weather", HTTP_POST, handleWeatherPost);
    server.on("/weather", HTTP_GET, handleWeatherPost);
    server.begin();
    Serial.println("HTTP server started");
  } else {
    Serial.println("HTTP server not started (no WiFi)");
  }

  Serial.println("Setup complete!");
}

// OLED removed: keep a no-op function for compatibility
void updateOLED(float, float, bool, float, float, bool, float, bool, unsigned long) {
  // no display
}

void loop() {
  unsigned long currentTime = millis();

  // Handle web server requests
  server.handleClient();

  // Read sensors every 5 seconds
  if (currentTime - lastReadTime >= READ_INTERVAL) {
    digitalWrite(LED_PIN, LOW);   // Turn LED on during reading
    readSensors();
    digitalWrite(LED_PIN, HIGH);  // Turn LED off
    lastReadTime = currentTime;
  }

  // Send data to server every 60 seconds
  if (currentTime - lastSendTime >= SEND_INTERVAL) {
    sendDataToServer();
    lastSendTime = currentTime;
  }

  // Small delay to prevent watchdog reset
  delay(100);
}
