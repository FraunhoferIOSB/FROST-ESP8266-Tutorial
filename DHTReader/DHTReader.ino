#include "DHT.h"

#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
#define DHTPIN 0     // what digital pin we're connected to



DHT dht(DHTPIN, DHTTYPE);

const long intervalSensorRead = 15000;       // update interval new sensor values

unsigned long millisLast = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  //Serial.setDebugOutput(true);
  Serial.println();
  Serial.println("____________");
  Serial.println("DHTxx Reader");
  Serial.println();
  Serial.println("______________");
  Serial.println("DHT22 starting");
  dht.begin();
  Serial.println("DHT22 started");
  Serial.println();
}

void loop() {
  unsigned long millisCurrent = millis();
  if (millisCurrent - millisLast >= intervalSensorRead) {
    Serial.println(millisCurrent - millisLast);
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
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.print(" *C ");
    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.print(" %");
    Serial.println();
  }
}
