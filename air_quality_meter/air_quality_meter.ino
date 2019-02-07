#include <ArduinoJson.h>
#include <WiFiManager.h>
#include <WiFiClient.h>
#include "config.h" // where the api endpoint including key is stored

WiFiClient client;

int GaugePin = 4; // Physical pin location on ESP2866 is D2

// airnowapi.org api only allows for 500 requests per hour
const int api_mtbs = 300000; // mean time between api requests set to 5 minutes
long api_lasttime = 0;   // last time api request has been done

void setup() {
  // Initialize Serial port
  Serial.begin(115200);
  while (!Serial) continue;

  pinMode(GaugePin, OUTPUT);

  WiFiManager wifiManager;
  wifiManager.autoConnect("AutoConnectAP");
  Serial.println("WiFi connected");

  // First run
  getAirQualityData();
}

void loop() {
  if ((millis() > api_lasttime + api_mtbs))  {
    getAirQualityData();
    api_lasttime = millis();
  }
}

void getAirQualityData() {
  // Connect to HTTP server
  client.setTimeout(10000);
  if (!client.connect("airnowapi.org", 80)) {
    Serial.println(F("Connection failed"));
    return;
  }

  Serial.println(F("Connected to airnowapi.org"));

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
  // Used arduinojson.org/assistant to compute the capacity.
  const size_t capacity = JSON_ARRAY_SIZE(3) + 3 * JSON_OBJECT_SIZE(2) + 3 * JSON_OBJECT_SIZE(10);
  DynamicJsonBuffer jsonBuffer(capacity);

  // Parse JSON object
  JsonArray& root = jsonBuffer.parseArray(client);
  if (!root.success()) {
    Serial.println(F("Parsing failed!"));
    return;
  }

  // Extract raw value
  Serial.println(F("Raw value:"));
  Serial.println(root[0]["AQI"].as<char*>());

  // Map raw value to 32bit resolution scale
  int AQI = root[0]["AQI"];
  int val = map(AQI, 0, 500, 0, 1023);
  Serial.println(F("Mapped value:"));
  Serial.println(val);

  // Set gauge
  analogWrite(GaugePin, val);

  // Disconnect
  client.stop();
}
