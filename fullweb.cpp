#include <Arduino.h>
#include "DHT.h"
#include <FS.h>
#include <SPIFFS.h>
#include <WiFi.h>
// #include <WebServer.h>
#include <ArduinoJson.h>
#include <map>
#include <ESPAsyncWebServer.h>


const int LDR = 33;
const int BUZZ = 19;
const int THS = 23;
const int FAN = 35;

DHT dht(THS, DHT11);

unsigned long previousDHTMillis = 0;
unsigned long previousLDRMillis = 0;
const unsigned long DHT_INTERVAL = 1500;
const unsigned long LDR_INTERVAL = 150;

const char* ssid = "Skysnap";

// WebServer server(80);
AsyncWebServer server(80);


std::map<String, String> fileMapping = {
    {"/404.html", "/aa8eb2fe.html"},
    {"/_next/static/S8oN9MNqOTgHldNuDXXUA/_buildManifest.js", "/9115cba3.js"},
    {"/_next/static/S8oN9MNqOTgHldNuDXXUA/_ssgManifest.js", "/628eb54a.js"},
    {"/_next/static/chunks/162-45f3b7b7ab559fa5.js", "/dafe477d.js"},
    {"/_next/static/chunks/472-c60bc20bf4845b8d.js", "/4eb58cae.js"},
    {"/_next/static/chunks/app/_not-found-b8ca7972db22cd62.js", "/3c7cb361.js"},
    {"/_next/static/chunks/app/layout-5a3827e34b3609c5.js", "/95134019.js"},
    {"/_next/static/chunks/app/page-32a9b7e90343d148.js", "/1cfb7e42.js"},
    {"/_next/static/chunks/fd9d1056-88ce05afd639010c.js", "/1e54bc67.js"},
    {"/_next/static/chunks/framework-8883d1e9be70c3da.js", "/288c4722.js"},
    {"/_next/static/chunks/main-622c2fca317b865d.js", "/783bbd23.js"},
    {"/_next/static/chunks/main-app-176f18854cb852bb.js", "/a3616701.js"},
    {"/_next/static/chunks/pages/_app-1534f180665c857f.js", "/f3730076.js"},
    {"/_next/static/chunks/pages/_error-b646007f40c4f0a8.js", "/2e3779eb.js"},
    {"/_next/static/chunks/polyfills-c67a75d1b6f99dc8.js", "/6888a6f4.js"},
    {"/_next/static/chunks/webpack-1b6cbe758c209d84.js", "/fc89ff9a.js"},
    {"/_next/static/css/5121504a7faab006.css", "/69bc6e6e.css"},
    {"/_next/static/media/26a46d62cd723877-s.woff2", "/5bc4c8ea.woff2"},
    {"/_next/static/media/55c55f0601d81cf3-s.woff2", "/0c7417bf.woff2"},
    {"/_next/static/media/581909926a08bbc8-s.woff2", "/9f3a8b60.woff2"},
    {"/_next/static/media/6d93bde91c0c2823-s.woff2", "/68f2331d.woff2"},
    {"/_next/static/media/97e0cb1ae144a2a9-s.woff2", "/846234df.woff2"},
    {"/_next/static/media/a34f9d1faa5f3315-s.p.woff2", "/48e7a964.woff2"},
    {"/_next/static/media/df0a9ae256c0569c-s.woff2", "/559110ee.woff2"},
    {"/favicon.ico", "/8af3a74e.ico"},
    {"/icons/wi-solar-eclipse.svg", "/b2ea4f85.svg"},
    {"/icons/wi-sprinkle.svg", "/1d0e3bd6.svg"},
    {"/icons/wi-stars.svg", "/589390ec.svg"},
    {"/icons/wi-storm-showers.svg", "/624d9634.svg"},
    {"/icons/wi-storm-warning.svg", "/327419b9.svg"},
    {"/icons/wi-strong-wind.svg", "/0b1af928.svg"},
    {"/icons/wi-sunrise.svg", "/f03fba21.svg"},
    {"/icons/wi-sunset.svg", "/99a51571.svg"},
    {"/icons/wi-thermometer-exterior.svg", "/1bcb8939.svg"},
    {"/icons/wi-thermometer-internal.svg", "/f98c8c03.svg"},
    {"/icons/wi-thermometer.svg", "/1f5ddd6e.svg"},
    {"/icons/wi-thunderstorm.svg", "/d1a66ac3.svg"},
    {"/icons/wi-tornado.svg", "/4546269c.svg"},
    {"/icons/wi-train.svg", "/aec53501.svg"},
    {"/icons/wi-tsunami.svg", "/4c22cdc4.svg"},
    {"/icons/wi-umbrella.svg", "/cfe33285.svg"},
    {"/icons/wi-volcano.svg", "/b85e1c26.svg"},
    {"/icons/wi-wind-deg.svg", "/19e41675.svg"},
    {"/icons/wi-windy.svg", "/96b47700.svg"},
    {"/images/dem.webp", "/b5ed0ae6.webp"},
    {"/index.txt", "/974d4d4e.txt"},
    {"/next.svg", "/9149982c.svg"},
    {"/vercel.svg", "/6ea7dba2.svg"}
};


float maxTemp = 0; // Set initial max temp to a very low value
float minTemp = 0;  // Set initial min temp to a very high value
unsigned long lastUpdate = 0; // To track time of last update

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

void handleFileRequest(AsyncWebServerRequest* request) {
    String path = request->url();

    if (path == "/") {
        path = "/index.html";
    }

    if (fileMapping.count(path) > 0) {
        path = fileMapping[path];
    }

    String contentType = "text/html";
    if (path.endsWith(".html")) contentType = "text/html";
    else if (path.endsWith(".css")) contentType = "text/css";
    else if (path.endsWith(".js")) contentType = "application/javascript";
    else if (path.endsWith(".ico")) contentType = "image/x-icon";
    else if (path.endsWith(".png")) contentType = "image/png";
    else if (path.endsWith(".svg")) contentType = "image/svg+xml";

    if (!path.startsWith("/")) {
        path = "/" + path;
    }

    if (SPIFFS.exists(path)) {
        request->send(SPIFFS, path, contentType);
    } else {
        request->send(404, "text/plain", "File Not Found: " + path);
    }
}

void setup() {
  pinMode(BUZZ, OUTPUT);
  pinMode(LDR, INPUT);
  pinMode(FAN, INPUT);
  pinMode(THS, INPUT);
  
  
  Serial.begin(115200);
  Serial.println("Serial initialized. Pin set to HIGH.");
  if (!SPIFFS.begin(true)) {  // Replace `SPIFFS` with `LittleFS` if needed
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  WiFi.softAP(ssid);
  Serial.println("Access point started");
  Serial.println("IP address: " + WiFi.softAPIP().toString());

  // Serve files
  server.onNotFound([](AsyncWebServerRequest* request) {
        handleFileRequest(request);
  });
  server.on("/live", HTTP_GET, handleLiveData);
  // Start the server
  server.begin();
  Serial.println("HTTP server started");

  dht.begin();

  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  while(file){
    Serial.print("FILE: ");
    Serial.println(file.name());
    file = root.openNextFile();
  }

  digitalWrite(BUZZ, HIGH);
  delay(100);
  digitalWrite(BUZZ, LOW);
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
    // LDR_mon();
    previousLDRMillis = currentMillis;
  }

}