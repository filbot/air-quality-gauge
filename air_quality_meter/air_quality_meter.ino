#include "config.h" // where the api endpoint including key is stored
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

const char *ssid = STASSID;
const char *password = STAPSK;

WiFiClient client;

// NOTE: GPIO4 and GPIO5 mixed up on the silk screen!
// GPIO6, GPIO7, GPIO11, GPIO8 are blocked for SPI

int GaugePin = 14; // Physical pin location on ESP2866 is D5
int ledPin = 5;    // D1

// airnowapi.org api only allows for 500 requests per hour
const int api_mtbs = 300000; // mean time between api requests set to 5 minutes
long api_lasttime = 0;       // last time api request has been done

void setup()
{
  // Initialize Serial port
  Serial.begin(115200);
  while (!Serial)
    continue;

  pinMode(GaugePin, OUTPUT);
  pinMode(ledPin, OUTPUT);

  digitalWrite(ledPin, HIGH);

  Serial.println();
  Serial.print("connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  delay(5000);

  // First run
  blinkLed();
  getAirQualityData();
}

void loop()
{
  if ((millis() > api_lasttime + api_mtbs))
  {
    getAirQualityData();
    api_lasttime = millis();
  }
}

void blinkLed()
{
  for (int i = 0; i <= 2; i++)
  {
    digitalWrite(ledPin, LOW); // turn the LED on (HIGH is the voltage level)
    delay(500);                // wait for a second
    digitalWrite(ledPin, HIGH +
                             0); // turn the LED off by making the voltage LOW
    delay(500);
  }
}

void getAirQualityData()
{
  // Connect to HTTP server
  client.setTimeout(10000);
  if (!client.connect("airnowapi.org", 80))
  {
    Serial.println(F("Connection failed"));
    return;
  }

  Serial.println(F("Connected to airnowapi.org"));

  // Send HTTP request
  client.println(API_REQUEST); //string: url and api key for host endpoint
  client.println(F("Host: airnowapi.org"));
  client.println(F("Connection: close"));
  if (client.println() == 0)
  {
    Serial.println(F("Failed to send request"));
    return;
  }

  // Check HTTP status
  char status[32] = {0};
  client.readBytesUntil('\r', status, sizeof(status));
  if (strcmp(status, "HTTP/1.1 200 OK") != 0)
  {
    Serial.print(F("Unexpected response: "));
    Serial.println(status);
    return;
  }

  // Skip HTTP headers
  char endOfHeaders[] = "\r\n\r\n";
  if (!client.find(endOfHeaders))
  {
    Serial.println(F("Invalid response"));
    return;
  }

  // Allocate JsonBuffer
  // Used arduinojson.org/assistant to compute the capacity.
  const size_t capacity = JSON_ARRAY_SIZE(2) + 2 * JSON_OBJECT_SIZE(2) + 2 * JSON_OBJECT_SIZE(10) + 625;
  DynamicJsonDocument doc(capacity);

  // Parse JSON object
  DeserializationError error = deserializeJson(doc, client);
  if (error)
  {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }

  // Extract raw value
  JsonObject root_1 = doc[1];
  int AQI = root_1["AQI"];
  Serial.println(F("Raw value:"));
  Serial.println(AQI);

  // Map raw value to 32bit resolution scale
  int val = map(AQI, 0, 500, 5, 35);
  Serial.println(F("Mapped value:"));
  Serial.println(val);

  // Set gauge
  analogWrite(GaugePin, val);

  // Disconnect
  client.stop();
}
