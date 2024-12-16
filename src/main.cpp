#include <Arduino.h>
#include "DHT.h"
#include <WiFi.h>
#include <map>
#include <ESPAsyncWebServer.h>
#include "env.cpp"
AsyncWebServer server(80);

#include <ArduinoJson.h>
#include <FirebaseClient.h>
#include <WiFiClientSecure.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>


DefaultNetwork network;
UserAuth user_auth(API_KEY, USER_EMAIL, USER_PASSWORD);
FirebaseApp app;
WiFiClientSecure ssl_client;
using AsyncClientx = AsyncClientClass;
AsyncClientx aClient(ssl_client, getNetwork(network));
RealtimeDatabase Database;
AsyncResult aResult_no_callback;
void printError(int code, const String &msg);


const int LDR = 33;
const int BUZZ = 19;
const int THS = 23;
const int FAN = 34;
const int LCD_SDA = 27;
const int LCD_SCL = 26;

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


DHT dht(THS, DHT11);
LiquidCrystal_I2C lcd(0x27, 16, 2);

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
  lcd.setCursor(0, 1);
  lcd.print("Connecting to XD");  
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
  // short Info Buzz
  digitalWrite(BUZZ, HIGH);
  delay(100);
  digitalWrite(BUZZ, LOW);
}

void BUZZZZ() {
  // long Error Buzz
  digitalWrite(BUZZ, HIGH);
  delay(400);
  digitalWrite(BUZZ, LOW);
}

float previousTemp = 0;
unsigned long previousTempTime = 0;

void DHT_Mon() {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  
  if (isnan(temp) || isnan(hum)) {
    Serial.println("Failed to read from DHT sensor!");
  } else {
    if (maxTemp == 0){
        maxTemp = temp;
        minTemp = temp;
        previousTemp = temp;
        previousTempTime = millis();
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
    
    // Check for sudden temperature change
    if (abs(temp - previousTemp) >= 5) {
      if (millis() - previousTempTime < 5000) {
        for (int i = 0; i < 5; i++) {
          buzz();
        }
      }
    }
    previousTemp = temp;
    previousTempTime = millis();
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
  if (ldrValue == 0){
    return;
  }
  Serial.print("Light Intensity: ");
  Serial.println(ldrValue);
  
//   if (ldrValue < 100) {
//     buzz();
//   }
}
void FAN_mon() {
  int WinSpeed = analogRead(FAN);

if (WinSpeed == 0 || (WinSpeed > 19.0 && WinSpeed < 29.9)) {
    return;
}
// sensorVoltage = adc0 * (3.3 / 4095.0); // Divide the controller working voltage by the analog input resolution;
// WindSpeed = (((sensorVoltage - 0.4) / 1.6) * 32.4) * 2.237;
  float voltage = WinSpeed * 5.0 / 1023.0;

  float windSpeed = (voltage) / 0.1;

  if (windSpeed < 0){
    windSpeed = 0;
  }

  live.speed = windSpeed;
  Serial.print("Wind Speed: ");
  Serial.println(windSpeed);

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

void printResult(AsyncResult &aResult)
{
    if (aResult.isEvent())
    {
        Firebase.printf("Event task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.appEvent().message().c_str(), aResult.appEvent().code());
    }

    if (aResult.isDebug())
    {
        Firebase.printf("Debug task: %s, msg: %s\n", aResult.uid().c_str(), aResult.debug().c_str());
    }

    if (aResult.isError())
    {
        Firebase.printf("Error task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.error().message().c_str(), aResult.error().code());
    }
}

void authHandler()
{
    unsigned long ms = millis();
    while (app.isInitialized() && !app.ready() && millis() - ms < 120 * 1000)
    {
        JWT.loop(app.getAuth());
        printResult(aResult_no_callback);
    }
}

void initializeFirebase() {
    Serial.println("Initializing Firebase...");
    ssl_client.setInsecure();
    initializeApp(aClient, app, getAuth(user_auth), aResult_no_callback);
    authHandler();
    app.getApp<RealtimeDatabase>(Database);
    Database.url(DATABASE_URL);
    aClient.setAsyncResult(aResult_no_callback);
}

void syncLiveData() {
    DynamicJsonDocument doc(1024);
    // doc["dt"] = live.dt;
    doc["temp"]["now"] = live.temp.now;
    doc["temp"]["min"] = live.temp.min;
    doc["temp"]["max"] = live.temp.max;
    doc["humidity"] = live.humidity;
    doc["speed"] = live.speed;
    doc["sun"] = live.sun;
    doc["deg"] = live.deg;
    doc["code"] = live.code;

    String jsonString;
    serializeJson(doc, jsonString);
    Database.set(aClient, "/liveData", object_t(jsonString));
}

void printError(int code, const String &msg)
{
    Firebase.printf("Error, msg: %s, code: %d\n", msg.c_str(), code);
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

  Wire.begin(LCD_SDA, LCD_SCL); // Initialize I2C with custom pins


  // Initialize the LCD
  lcd.init();
  lcd.backlight(); // Turn on the LCD backlight

  // Test message
  lcd.setCursor(0, 0);
  lcd.print("SKYSNAP 2.0 XD");
  lcd.setCursor(0, 1);
  lcd.print("TO THE MOON & BACK");


  connectToWiFi();
  while (WiFi.status() != WL_CONNECTED) {
    for (int x=0; x< nLEDS; x++){
        digitalWrite(LEDPINS[x], HIGH);
        delay(100);
        digitalWrite(LEDPINS[x], LOW);
    }
    Serial.print(".");
  }

  Firebase.printf("Firebase Client v%s\n", FIREBASE_CLIENT_VERSION);
  Serial.println("Initializing app...");
  initializeFirebase();
  server.on("/live", HTTP_GET, handleLiveData);
  server.begin();
  Serial.println("HTTP server started");

  dht.begin();

  BUZZZ();
}

unsigned long previousDisplayMillis = 0;
const unsigned long DISPLAY_INTERVAL = 2000;
int displayState = 0;

void loop() {
  unsigned long currentMillis = millis();
  
  if (currentMillis - previousDHTMillis >= DHT_INTERVAL) {
    DHT_Mon();
    previousDHTMillis = currentMillis;
    live.dt = millis();
  }
  if (currentMillis % 1000 == 100) {
      syncLiveData();
  }
  
  if (currentMillis - previousLDRMillis >= LDR_INTERVAL) {
    FAN_mon();
    LDR_mon();
    previousLDRMillis = currentMillis;
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Lost connection to WiFi. Reconnecting...");
    lcd.setCursor(0, 0);
    lcd.print("SKYSNAP 2.0 XD");
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

  authHandler();

  Database.loop();

  if (currentMillis - previousDisplayMillis >= DISPLAY_INTERVAL) {
    previousDisplayMillis = currentMillis;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("SKYSNAP 2.0 XD");
    lcd.setCursor(0, 1);
    switch (displayState) {
      case 0:
        lcd.print("Temp: ");
        lcd.print(live.temp.now);
        lcd.print(" C");
        break;
      case 1:
        lcd.print("Hum: ");
        lcd.print(live.humidity);
        lcd.print(" %");
        break;
      case 2:
        lcd.print("Wind: ");
        lcd.print(live.speed);
        break;
      case 3:
        lcd.print("Sun: ");
        lcd.print(live.sun);
        lcd.print(" %");
        break;
    }
    displayState = (displayState + 1) % 4;
  }
}
