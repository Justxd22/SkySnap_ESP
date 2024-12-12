#include <Arduino.h>
#include "DHT.h"
#include <WiFi.h>
#include <map>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>


const String FHOST = "https://skysnap-12d5a-default-rtdb.europe-west1.firebasedatabase.app/";
const String FAUTH  = "";

const int LDR = 33;
const int BUZZ = 19;
const int THS = 23;
const int FAN = 34;


const int LEDPINS[] = {12,13};
const int nLEDS = sizeof(LEDPINS) / sizeof(LEDPINS[0]);

float maxTemp = 0;
float minTemp = 0;
unsigned long lastUpdate = 0;


unsigned long previousDHTMillis = 0;
unsigned long previousLDRMillis = 0;
const unsigned long DHT_INTERVAL = 1500;
const unsigned long LDR_INTERVAL = 150;

const char* ssid = "XD";
const char* pass = "12312345";

bool wifiConnected = false;


AsyncWebServer server(80);
DHT dht(THS, DHT11);


// Live data structure
struct LiveData {
    unsigned long dt;
    struct {
        float now;
        float min;
        float max;
    } temp;
    int humidity = 0;
    int speed = 0;
    int sun = 0;
    int deg = 0;
    String code = "sunrise";
} live;

void connectToWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  delay(500);
  Serial.println("");
  if (WiFi.status() == WL_CONNECTED){
  Serial.println("WiFi connected");
  Serial.print  ("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("Connected to WiFi");
  }
}


void buzz() {
digitalWrite(BUZZ, HIGH);
delay(100);
digitalWrite(BUZZ, LOW);
delay(100);
digitalWrite(BUZZ, HIGH);
delay(300);
digitalWrite(BUZZ, LOW);
delay(200);
digitalWrite(BUZZ, HIGH);
delay(200);
digitalWrite(BUZZ, LOW);
delay(500);
}

void BUZZZ() {
  digitalWrite(BUZZ, HIGH);
  delay(100);
  digitalWrite(BUZZ, LOW);
}

void BUZZZZ() {
  digitalWrite(BUZZ, HIGH);
  delay(400);
  digitalWrite(BUZZ, LOW);
}

void DHT_Mon() {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  
  if (isnan(temp) || isnan(hum)) {
    Serial.println("Failed to read from DHT sensor!");
  } else {
    if (maxTemp == 0){
        maxTemp = temp;
        minTemp = temp;
    }
    live.temp.now = temp;
    live.humidity = hum;

    if (temp > maxTemp) {
        maxTemp = temp;
    }
    if (temp < minTemp) {
        minTemp = temp;
    }

    live.temp.min = minTemp;
    live.temp.max = maxTemp;
    
    // Serial.print("Temperature: ");
    // Serial.print(temp);
    // Serial.println(" °C");
    // Serial.print("Humidity: ");
    // Serial.print(hum);
    // Serial.println(" %");
  }
}

void LDR_mon() {
  int ldrValue = analogRead(LDR);
  live.sun = map(ldrValue, 0, 4095, 0, 100);
  Serial.print("Light Intensity: ");
  Serial.println(ldrValue);
  
  if (ldrValue < 100) {
    buzz();
  }
}
void FAN_mon() {
  int WinSpeed = analogRead(FAN);
  live.speed = WinSpeed;
  Serial.print("Wind Speed: ");
  Serial.println(WinSpeed);

}


void handleLiveData(AsyncWebServerRequest *request) {
    DynamicJsonDocument doc(1024);
    doc["dt"] = live.dt;
    doc["temp"]["now"] = live.temp.now;
    doc["temp"]["min"] = live.temp.min;
    doc["temp"]["max"] = live.temp.max;
    doc["humidity"] = live.humidity;
    doc["speed"] = live.speed;
    doc["sun"] = live.sun;
    doc["deg"] = live.deg;
    doc["code"] = live.code;

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void setup() {
  pinMode(BUZZ, OUTPUT);
  pinMode(LDR, INPUT);
  pinMode(FAN, INPUT);
  pinMode(THS, INPUT);
  pinMode(12, OUTPUT);
  pinMode(13, OUTPUT);
  
  
  Serial.begin(115200);
  Serial.println("Serial initialized. Pin set to HIGH.");
  connectToWiFi();
  while (WiFi.status() != WL_CONNECTED) {
    for (int x=0; x< nLEDS; x++){
        digitalWrite(LEDPINS[x], HIGH);
        delay(100);
        digitalWrite(LEDPINS[x], LOW);
    }
    Serial.print(".");
  }

  server.on("/live", HTTP_GET, handleLiveData);
  server.begin();
  Serial.println("HTTP server started");

  dht.begin();

  BUZZZ();
}

void loop() {
  unsigned long currentMillis = millis();
  
  if (currentMillis - previousDHTMillis >= DHT_INTERVAL) {
    DHT_Mon();
    previousDHTMillis = currentMillis;
    live.dt = millis();
  }
  
  if (currentMillis - previousLDRMillis >= LDR_INTERVAL) {
    FAN_mon();
    LDR_mon();
    previousLDRMillis = currentMillis;
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Lost connection to WiFi. Reconnecting...");
    BUZZZZ();
    connectToWiFi();
    while (WiFi.status() != WL_CONNECTED) {
    for (int x=0; x< nLEDS; x++){
        digitalWrite(LEDPINS[x], HIGH);
        delay(100);
        digitalWrite(LEDPINS[x], LOW);
    }
    Serial.print(".");
    }
    BUZZZ();

  }

}

