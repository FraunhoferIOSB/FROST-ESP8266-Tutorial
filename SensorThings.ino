#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include "DHT.h"
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>


#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
#define DHTPIN 0     // what digital pin we're connected to



DHT dht(DHTPIN, DHTTYPE);

const char* ssid     = "Your WiFi Name";
const char* password = "Your WiFi Password";

const long intervalSensorRead = 15000;       // update interval new sensor values
const long intervalDataSend = 4;      // update interval new sensor values

unsigned long millisLast = 0;
unsigned long skip = 1;

// SensorThing
int SensorThing_temperature_count = 1;            // elements in temperature arry
// DS 7 = Test Temp
String SensorThing_temperature[] = {
  "http://leffe:8080/SensorThingsService/v1.0/Datastreams(7)/Observations",
};
// DS 8 = Test Humid
int SensorThing_humidity_count = 1;               // elements in humidity arry
String SensorThing_humidity[] = {
  "http://leffe:8080/SensorThingsService/v1.0/Datastreams(8)/Observations",
};

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  //Serial.setDebugOutput(true);
  Serial.println();
  Serial.println("________________________");
  Serial.println("DHTxx SensorThingsSender");
  Serial.println();
  Serial.println("______________");
  Serial.println("DHT22 starting");
  dht.begin();
  Serial.println("DHT22 started");

  printMac();

  Serial.println();
  Serial.println("__________________");
  Serial.println("Connecting to WIFI");
  Serial.println(ssid);
  // Connect to WiFi network
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Connecting");
    WiFi.begin(ssid, password);
  }

  // Wait for connection
  unsigned int count = 0;
  unsigned int status = WiFi.status();
  while (status != WL_CONNECTED) {
    count++;
    switch (status) {
      case 1:
        Serial.println();
        Serial.println("SSID Not Available");
        count = 0;
        break;

      case 4:
        Serial.println();
        Serial.println("Connect Failed");
        count = 0;
        break;

      case 5:
        Serial.println();
        Serial.println("Connection Lost");
        count = 0;
        break;

      default:
        Serial.print(".");
    }
    if (count >= 10) {
      count = 0;
      Serial.println();
    }
    delay(1000);
    status = WiFi.status();
  }
  Serial.println();
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.println();
}

void loop() {
  unsigned long millisCurrent = millis();
  if (millisCurrent - millisLast >= intervalSensorRead) {
    Serial.println(millisCurrent - millisLast);
    millisLast = millisCurrent;
    readSensorAndSend();
  }
}

void readSensorAndSend() {
  float temperature, humidity;

  humidity = dht.readHumidity();
  temperature = dht.readTemperature();

  // Check if any reads failed and exit early (to try again).
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  } else {
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.print(" *C ");
    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.print(" %");
    Serial.println();
  }

  skip--;
  if (skip <= 0) {
    skip = intervalDataSend;

    StaticJsonBuffer<30> JSONbuffer;
    JsonObject& JSONencoder = JSONbuffer.createObject();
    JSONencoder["result"] = humidity;
    char JSON_humidity[30];
    JSONencoder.printTo(JSON_humidity, sizeof(JSON_humidity));
    Serial.println(JSON_humidity);
    char JSON_humidity2[30];
    os_sprintf(JSON_humidity2, "{\"result\":%2.2d}", humidity);
    Serial.println(JSON_humidity2);

    JSONencoder["result"] = temperature;
    char JSON_temperature[30];
    JSONencoder.printTo(JSON_temperature, sizeof(JSON_temperature));
    Serial.println(JSON_temperature);

    for (int i = 0; i < SensorThing_temperature_count; i++) {
      postToUrl(SensorThing_temperature[i], JSON_temperature);
    }

    for (int i = 0; i < SensorThing_humidity_count; i++) {
      postToUrl(SensorThing_humidity[i], JSON_humidity);
    }
  }
}

void postToUrl(String url, char* data) {
  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  int httpCode = http.POST(data);
  if (httpCode > 0) {
    if (httpCode != 201) {
      Serial.println("Datastream: " + url);
      String payload = http.getString();
      Serial.println(payload);
    }
  } else {
    Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
}

void printMac() {
  // print your MAC address:
  byte mac[6];
  WiFi.macAddress(mac);
  Serial.print("MAC address: ");
  Serial.print(mac[5], HEX);
  Serial.print(":");
  Serial.print(mac[4], HEX);
  Serial.print(":");
  Serial.print(mac[3], HEX);
  Serial.print(":");
  Serial.print(mac[2], HEX);
  Serial.print(":");
  Serial.print(mac[1], HEX);
  Serial.print(":");
  Serial.println(mac[0], HEX);
}

