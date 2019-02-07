#include <ArduinoJson.h>
#include <WiFiManager.h>
#include <WiFiClient.h>
#include "config.h"

WiFiClient client;

int GaugePin = 4;

void setup() {
  // Initialize Serial port
  Serial.begin(115200);
  while (!Serial) continue;

  pinMode(GaugePin, OUTPUT);

  WiFiManager wifiManager;
  wifiManager.autoConnect("AutoConnectAP");
  Serial.println("WiFi connected");

  getAirQualityData();
}

void loop() {
  // not used in this example
}

void getAirQualityData() {
  // Connect to HTTP server
  client.setTimeout(10000);
  if (!client.connect("airnowapi.org", 80)) {
    Serial.println(F("Connection failed"));
    return;
  }

  Serial.println(F("Connected to airnowapi.org!"));

  // Send HTTP request
  client.println(API_REQUEST); //string: url and api key for host endpoint
  client.println(F("Host: airnowapi.org"));
  client.println(F("Connection: close"));
  if (client.println() == 0) {
    Serial.println(F("Failed to send request"));
    return;
  }

  // Check HTTP status
  char status[32] = {0};
  client.readBytesUntil('\r', status, sizeof(status));
  if (strcmp(status, "HTTP/1.1 200 OK") != 0) {
    Serial.print(F("Unexpected response: "));
    Serial.println(status);
    return;
  }

  // Skip HTTP headers
  char endOfHeaders[] = "\r\n\r\n";
  if (!client.find(endOfHeaders)) {
    Serial.println(F("Invalid response"));
    return;
  }

  // Allocate JsonBuffer
  // Use arduinojson.org/assistant to compute the capacity.
  const size_t capacity = JSON_ARRAY_SIZE(3) + 3 * JSON_OBJECT_SIZE(2) + 3 * JSON_OBJECT_SIZE(10);
  DynamicJsonBuffer jsonBuffer(capacity);

  // Parse JSON object
  JsonArray& root = jsonBuffer.parseArray(client);
  if (!root.success()) {
    Serial.println(F("Parsing failed!"));
    return;
  }

  // Extract values
  Serial.println(F("Response:"));
  Serial.println(root[0]["AQI"].as<char*>());
//  int AQI = root[0]["AQI"];
  int AQI = 250;
  int val = map(AQI, 0, 500, 0, 1023);
  Serial.println(F("val response:"));
  Serial.println(val);
  analogWrite(GaugePin, val);

  // Disconnect
  client.stop();
}
