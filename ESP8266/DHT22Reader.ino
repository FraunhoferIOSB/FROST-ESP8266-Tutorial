#include "DHT.h"

#define DHTTYPE DHT22                             // DHT 22  (AM2302), AM2321
#define DHTPIN 0                                  // what digital pin we're connected to

DHT dht(DHTPIN, DHTTYPE);

const long intervalSensorRead = 15000;           // interval sensor read in ms
unsigned long millisLast = 0;

void setup() {
  Serial.begin(115200);
  //Serial.setDebugOutput(true);
  Serial.println();
  Serial.println("#########################################################");
  Serial.println("Example Program reading sensor values from DHT22 sensor");
  Serial.println("#########################################################");
  Serial.println();
  Serial.print("starting Sensor (DHT22) ...");
  dht.begin();
  Serial.println("started");
  Serial.print("measuring interval ");
  Serial.print(intervalSensorRead);
  Serial.print("ms");
  Serial.println();
}



void loop() {
  unsigned long millisCurrent = millis();
  if (millisCurrent - millisLast >= intervalSensorRead) {
    millisLast = millisCurrent;
    readSensor();
  }
}

void readSensor() {
  float temperature, humidity;

  humidity = dht.readHumidity();
  temperature = dht.readTemperature();

  // Check if any reads failed and exit early (to try again).
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
}
