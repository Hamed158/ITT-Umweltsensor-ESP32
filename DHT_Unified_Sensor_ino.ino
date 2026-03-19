#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "DHT.h"
#include <time.h>
#include <PubSubClient.h>
#include <ESPmDNS.h>

// ==========================================
// 1. KONFIGURATION & NETZWERK
// ==========================================
const char* WIFI_SSID     = "deine WLAN_SSID";
const char* WIFI_PASSWORD = "dein_PASS";

const char* MQTT_HOST_NAME = "11FI6-turbo"; //hostname
const uint16_t MQTT_PORT   = 1883;
const char* MQTT_USER      = "dein mqtt user";
const char* MQTT_PASS      = "dein mqtt pasword";

const char* TOPIC_PUB      = "sensors/esp32/data";
const char* TOPIC_CMD      = "sensors/esp32/cmd";

// ==========================================
// 2. PINS & SENSOREN
// ==========================================
#define DHTPIN 23
#define DHTTYPE DHT11
#define LIGHT_PIN 34     // Dein D0 Anschluss vom Lichtsensor
#define TRIG_PIN 5       // Ultraschall Trig
#define ECHO_PIN 18      // Ultraschall Echo

DHT dht(DHTPIN, DHTTYPE);

// OLED Display
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ==========================================
// 3. VARIABLEN & STATUS
// ==========================================
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);
IPAddress resolvedPiIP;

unsigned long lastPub = 0;
const unsigned long pubMs = 5000;
unsigned long lastWifiCheck = 0;

char lastAiMsg[32] = "Warte auf Pi...";
bool timeSynced = false;

// ==========================================
// 4. HILFSFUNKTIONEN (ULTRASCHALL & ZEIT)
// ==========================================
long getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH, 26000); // Timeout nach ~4m
  if (duration == 0) return 400;
  return duration / 58;
}

void setupTime() {
  // Deutschland: CET/CEST mit automatischer Umstellung
  configTzTime("CET-1CEST,M3.5.0,M10.5.0", "de.pool.ntp.org", "pool.ntp.org");
  Serial.println("NTP Zeit-Sync gestartet...");
}

// ==========================================
// 5. MQTT CALLBACK (Empfängt KI-Nachrichten)
// ==========================================
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Nachricht vom Pi: ");
  unsigned int i;
  for (i = 0; i < length && i < sizeof(lastAiMsg) - 1; i++) {
    lastAiMsg[i] = (char)payload[i];
  }
  lastAiMsg[i] = '\0';
  Serial.println(lastAiMsg);
}

// ==========================================
// 6. VERBINDUNGS-MANAGEMENT (STABILITÄT)
// ==========================================
void ensureConnections() {
  // WiFi Check
  if (WiFi.status() != WL_CONNECTED) {
    if (millis() - lastWifiCheck > 5000) {
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      lastWifiCheck = millis();
    }
    return;
  }

  // Pi IP auflösen via mDNS
  if (resolvedPiIP.toString() == "0.0.0.0") {
    resolvedPiIP = MDNS.queryHost(MQTT_HOST_NAME);
    if (resolvedPiIP.toString() != "0.0.0.0") {
      mqtt.setServer(resolvedPiIP, MQTT_PORT);
      mqtt.setCallback(mqttCallback);
    }
  }

  // MQTT Reconnect
  if (resolvedPiIP.toString() != "0.0.0.0" && !mqtt.connected()) {
    String clientId = "ESP32-Client-" + String(random(0xffff), HEX);
    if (mqtt.connect(clientId.c_str(), MQTT_USER, MQTT_PASS)) {
      mqtt.subscribe(TOPIC_CMD);
      Serial.println("MQTT Verbunden & Subscribed");
    }
  }
}

// ==========================================
// 7. SETUP
// ==========================================
void setup() {
  Serial.begin(115200);
  
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LIGHT_PIN, INPUT);
  
  dht.begin();
  Wire.begin(21, 22);
  
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED Fehler!");
    for(;;);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,20);
  display.println("System Start...");
  display.display();

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  MDNS.begin("esp32-sensor");
  setupTime();
}

// ==========================================
// 8. HAUPTSCHLEIFE (LOOP)
// ==========================================
void loop() {
  ensureConnections();
  if (mqtt.connected()) mqtt.loop();

  // Sensoren auslesen
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  int lightStatus = digitalRead(LIGHT_PIN); // 0 = Hell, 1 = Dunkel (meistens)
  long dist = getDistance();

  // Uhrzeit formatieren
  struct tm timeinfo;
  char ts[15] = "Sync...";
  if (getLocalTime(&timeinfo)) {
    strftime(ts, sizeof(ts), "%H:%M:%S", &timeinfo);
    timeSynced = true;
  }

  // Daten an Pi senden (alle 5 Sek)
  if (mqtt.connected() && millis() - lastPub > pubMs) {
    lastPub = millis();
    char payload[200];
    snprintf(payload, sizeof(payload), 
             "{\"t\":%.1f,\"h\":%.1f,\"l\":%d,\"d\":%ld,\"ts\":\"%s\"}", 
             t, h, lightStatus, dist, ts);
    mqtt.publish(TOPIC_PUB, payload);
  }

  // DISPLAY LAYOUT
  display.clearDisplay();
  
  // Kopfzeile
  display.setTextSize(1);
  display.setCursor(0,0);
  display.print(ts);
  display.setCursor(75,0);
  display.print(mqtt.connected() ? "MQTT OK" : "MQTT ..");
  display.drawFastHLine(0, 10, 128, WHITE);

  // Klima-Werte
  display.setCursor(0, 16);
  display.printf("Temp: %.1f C  Hum: %.0f%%", t, h);
  
  // Licht-Status (Digital)
  display.setCursor(0, 28);
  display.print("Licht: ");
  display.print((lightStatus == 0) ? "HELL" : "DUNKEL");

  // Distanz + Warnung
  display.setCursor(0, 40);
  display.printf("Distanz: %ld cm", dist);
  
  // NAH-WARNUNG (Wieder drin!)
  if (dist < 20 && dist > 1) {
    display.setCursor(95, 40);
    display.print("!NAH!");
  }

  // KI-NACHRICHT (Ganz unten)
  display.drawFastHLine(0, 52, 128, WHITE);
  display.setCursor(0, 56);
  display.print("KI: "); 
  display.print(lastAiMsg);

  display.display();
  delay(500);
}
