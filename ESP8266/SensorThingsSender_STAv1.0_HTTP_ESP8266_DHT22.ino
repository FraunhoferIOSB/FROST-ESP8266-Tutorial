#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include "DHT.h"

#define DHTTYPE DHT22                            // DHT 22  (AM2302), AM2321
#define DHTPIN 0                                 // what digital pin we're connected to
#define ARRAY_SIZE(x) sizeof(x)/sizeof(x[0])     // helper function to determine array size

DHT dht(DHTPIN, DHTTYPE);

const char* ssid     = "frost-network";          // your WiFi SSID
const char* password = "frostWEB";               // your WiFi password

const long intervalSensorRead = 15000;           // interval sensor read in ms
const long intervalDataSend = 4;                 // interval send data (in multiples of intervalSensorRead)
const long jsonSize = 23;                        // compute with https://arduinojson.org/v6/assistant/
unsigned long millisLast = 0;
unsigned long skip = 1;
unsigned long httpRetries = 3;                   // sometimes HTTP connectino gets refused, so do retries before actually failing

const char* DatastreamUrlPattern = "http://frost-server/FROST-Server/v1.0/Datastreams(%i)/Observations";   // put your SensorThings Server URL here

// IDs of datastreams to insert temperature measurements
int DatastreamsTemperature[] = {
  1
};

// IDs of datastreams to insert humidity measurements
int DatastreamsHumidity[] = {
  2
};

HTTPClient http;

void setup() {
  Serial.begin(115200);
  //Serial.setDebugOutput(true);
  Serial.println();
  Serial.println("#############################################################");
  Serial.println("Fraunhofer IOBS SensorThingsSender");
  Serial.println("#############################################################");
  Serial.println("SensorThingsAPI Version:        v1.0");
  Serial.println("Protocol:                       HTTP");
  Serial.println("Platform:                       NodeMCU 1.0 (ESP8266)");
  Serial.println("Sensor:                         DHT22 (temperature, humidity)");   
  Serial.print("measuring interval:             ");
  Serial.print(intervalSensorRead);
  Serial.print("ms");
  Serial.println();
  Serial.print("publish every _ measurements:   ");
  Serial.print(intervalDataSend);
  Serial.println();
  
  printMac();
  startSensor();  
  connectToWiFi();
  http.setReuse(true);
}

void loop() {
  unsigned long millisCurrent = millis();
  if (millisCurrent - millisLast >= intervalSensorRead) {
    millisLast = millisCurrent;
    readSensorAndSend();
  }
}

void readSensorAndSend() {
  float temperature, humidity;
  // actually read data from sensor
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();

  // Check if any read failed and exit early (to try again).
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  } else {  
    Serial.print("temperature: ");
    Serial.print(temperature);
    Serial.print(" °C ");
    Serial.print("humidity: ");
    Serial.print(humidity);
    Serial.print(" %");
    Serial.println();
  }

  skip--;
  if (skip <= 0) {
    skip = intervalDataSend;
    sendData(DatastreamsTemperature, temperature);
    sendData(DatastreamsHumidity, humidity);    
  }
}

void sendData(int* datastreams, float data) {
    // create JSON document only once for each measurement
    StaticJsonDocument<jsonSize> jsonDoc;
    jsonDoc["result"] = serialized(String(data,2));   // force 2 decimal places
    char jsonData[jsonSize];
    serializeJson(jsonDoc, jsonData);
    for (int i = 0; i < ARRAY_SIZE(datastreams); i++) {
        sendToDatastream(datastreams[i], jsonData);
    }
}

void sendToDatastream(int datastreamID, char* data) {
  // generate URL for datastream ID
  char url[strlen(DatastreamUrlPattern)];
  sprintf(url, DatastreamUrlPattern, datastreamID);
  doHttpPost(url, data);
}

void doHttpPost(String url, char* data) {
  Serial.print("sending data: ");
  Serial.print(data);
  Serial.print(" to url: ");
  Serial.print(url);
  Serial.println();  
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  long tries = 0;
  int httpCode = 0;
  do {
    httpCode = http.POST(data);
    tries++;
    if (tries > 1) {
      Serial.print("HTTP POST failed, error: ");
      Serial.print(http.errorToString(httpCode).c_str());
      Serial.print(" - retrying");
      Serial.println();
    }
  } while (httpCode < 0 && tries <= httpRetries);
  if (httpCode > 0) {
    if (httpCode != 201) {
      Serial.print("Unexpected HTTP result, code: ");
      Serial.print(httpCode);
      Serial.println(", message: ");
      Serial.print(http.getString());
      Serial.println();
    }
  } else {
    Serial.print("HTTP POST failed, error: ");
    Serial.print(http.errorToString(httpCode).c_str());
    Serial.println();
  }
  http.end();
}

void startSensor() {
  Serial.print("starting Sensor (DHT22) ...");
  dht.begin();
  Serial.println("started");
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

void connectToWiFi() {
  Serial.println();
  Serial.print("Connecting to WIFI \"");
  Serial.print(ssid);
  Serial.print("\" ...");
  // Connect to WiFi network
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(ssid, password);
  }

  // Wait for connection
  unsigned int count = 0;
  unsigned int status = WiFi.status();
  while (status != WL_CONNECTED) {
    count++;
    switch (status) {
      case 1:
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
  Serial.print("connected. IP address: ");
  Serial.print(WiFi.localIP());
  Serial.println();
}