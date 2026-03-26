#include <ArduinoJson.h> 
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "DHT.h"
#include <time.h>
#include <PubSubClient.h>
#include <ESPmDNS.h>
#include <WebServer.h>


// ==========================================
// 1. KONFIGURATION & NETZWERK
// ==========================================
const char* WIFI_SSID     = "wlan-SSID";
const char* WIFI_PASSWORD = "wlan_passwort";

const char* MQTT_HOST_NAME = "11FI6-turbo"; // Raspi hostname 
const uint16_t MQTT_PORT   = 1883;
const char* MQTT_USER      = "mqttuser"; //mqttuser name 
const char* MQTT_PASS      = "mqtt_Password"; 

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
WebServer server(80); // Erstellt den Server auf Port 80

unsigned long lastPub = 0;
const unsigned long pubMs = 5000;
unsigned long lastWifiCheck = 0;

char lastAiMsg[32] = "Warte auf Pi...";
char lastWebAiMsg[512] = "System bereit";  // NEU: Für die Webseite (viel größer!)
bool timeSynced = false;

//
float t = 0.0; 
float h = 0.0;
int l = 0;
long d = 0;

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

void handleRoot() {
  // Logik für Licht-Text
  String lichtText = (l == 0) ? "HELL" : "DUNKEL";
  String lichtFarbe = (l == 0) ? "#ffcc00" : "#2c3e50"; // Gelb für hell, Dunkelblau für dunkel

  String html = "<!DOCTYPE html><html lang='de'>";
  html += "<head><meta name='viewport' content='width=device-width, initial-scale=1' charset='utf-8'>";
  html += "<meta http-equiv='refresh' content='5'>"; 
  html += "<style>";
  html += "body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; text-align: center; background-color: #eef2f7; margin: 0; padding: 20px; color: #333; }";
  html += ".container { max-width: 500px; margin: auto; background: white; padding: 30px; border-radius: 25px; box-shadow: 0 15px 35px rgba(0,0,0,0.1); }";
  html += "h1 { color: #1a73e8; margin-bottom: 25px; font-size: 26px; border-bottom: 2px solid #f0f0f0; padding-bottom: 10px; }";
  html += ".grid { display: grid; grid-template-columns: 1fr 1fr; gap: 15px; }";
  html += ".card { background: #f8f9fa; padding: 20px; border-radius: 15px; border-bottom: 4px solid #1a73e8; }";
  html += ".full-card { grid-column: span 2; }";
  html += ".label { font-size: 12px; color: #7f8c8d; text-transform: uppercase; letter-spacing: 1px; }";
  html += ".value { font-size: 24px; font-weight: bold; margin-top: 5px; }";
  html += ".ki-box { background: #fff; border: 2px solid #1a73e8; padding: 20px; border-radius: 15px; margin-top: 25px; text-align: left; line-height: 1.6; }";
  html += ".net-info { font-size: 11px; color: #bdc3c7; margin-top: 20px; background: #34495e; color: white; padding: 10px; border-radius: 10px; }";
  html += ".status-dot { height: 10px; width: 10px; background-color: #2ecc71; border-radius: 50%; display: inline-block; margin-right: 5px; }";
  html += "</style></head><body>";
  
  html += "<div class='container'>";
  html += "<h1>Room-Monitoring Smart System</h1>";
  
  html += "<div class='grid'>";
  // Temperatur & Luftfeuchtigkeit
  html += "<div class='card'><div class='label'>Temperatur</div><div class='value'>" + String(t, 1) + " °C</div></div>";
  html += "<div class='card'><div class='label'>Feuchte</div><div class='value'>" + String(h, 0) + " %</div></div>";
  
  // Licht & Distanz
  html += "<div class='card'><div class='label'>Licht</div><div class='value' style='color:" + lichtFarbe + "'>" + lichtText + "</div></div>";
  html += "<div class='card'><div class='label'>Distanz</div><div class='value'>" + String(d) + " cm</div></div>";
  
  // KI Bereich (Groß)
 // KI Bereich (Groß und schön)
  html += "<div class='card full-card'>"; // Öffnet die Karte
    html += "<div class='label' style='color:#1a73e8; font-weight:bold;'>🤖 KI Analyse & Empfehlung:</div>";
    html += "<div style='margin-top:10px; font-size: 16px; line-height: 1.5;'>";
      html += String(lastWebAiMsg); // Hier die LANGE Nachricht für die Webseite
    html += "</div>";
  html += "</div>"; // Schließt die Karte
  
  html += "</div>"; // Schließt das Grid (Grid Ende)

  // Netzwerk Infos
  html += "<div class='net-info'>";
  html += "<span><span class='status-dot'></span> System Online</span><br>";
  html += "Device: ESP32-Sensor-Node | IP: " + WiFi.localIP().toString() + "<br>";
  html += "WLAN: " + String(WIFI_SSID) + " | Signal: " + String(WiFi.RSSI()) + " dBm";
  html += "</div>";
  
  html += "</div></body></html>";

  server.send(200, "text/html", html);
}
// ==========================================
// 5. MQTT CALLBACK (Empfängt KI-Nachrichten)
// ==========================================

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  // 1. JSON-Dokument erstellen
  StaticJsonDocument<1024> doc; // 1024 Bytes Platz für die lange Nachricht

  // 2. Das Paket vom Pi auspacken
  DeserializationError error = deserializeJson(doc, payload, length);

  if (!error) {
    // 3. Den kurzen Teil fürs OLED extrahieren
    const char* oledPart = doc["oled"] | "Status OK";
    strlcpy(lastAiMsg, oledPart, sizeof(lastAiMsg));

    // 4. Den langen Teil für die Webseite extrahieren
    const char* webPart = doc["web"] | "System läuft stabil.";
    strlcpy(lastWebAiMsg, webPart, sizeof(lastWebAiMsg));
    
    Serial.println("Daten erfolgreich entpackt!");
  } else {
    Serial.print("JSON Fehler: ");
    Serial.println(error.f_str());
  }
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

  // Webserver Routen definieren
  server.on("/", handleRoot); 
  server.begin();
  Serial.println("HTTP Webserver gestartet");

  // mDNS starten (optional, damit du http://raummonitor.local im Browser tippen kannst)
  if (MDNS.begin("raummonitor")) {
    Serial.println("mDNS Responder gestartet: http://raummonitor.local");
  }
}

// ==========================================
// 8. HAUPTSCHLEIFE (LOOP)
// ==========================================
void loop() {
  server.handleClient(); 
  ensureConnections();
  if (mqtt.connected()) mqtt.loop();

  // WICHTIG: Hier kein "float" oder "long" davor schreiben! 
  // Nur die Werte zuweisen:
  h = dht.readHumidity();
  t = dht.readTemperature();
  l = digitalRead(LIGHT_PIN); 
  d = getDistance();
  

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
