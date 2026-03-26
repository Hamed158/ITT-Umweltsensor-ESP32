🌍 KI-gestütztes IoT-Umwelt-Dashboard (Edge Computing)
Entwicklung eines intelligenten IoT-Systems zur Umweltdatenerfassung mit lokaler KI-Analyse auf einem Raspberry Pi und einem interaktiven Web-Dashboard auf dem ESP32.

🚀 Highlights & Neue Features
Duales Interface: Live-Status auf einem physischen OLED-Display und ein detailliertes Web-Dashboard (HTTP-Server).

Edge-KI Integration: Lokale Datenverarbeitung mit Ollama (Qwen2) auf dem Raspberry Pi – 100% datenschutzkonform ohne Cloud.

Intelligentes JSON-Parsing: Der ESP32 verarbeitet komplexe Datenpakete und trennt kurze Statusmeldungen für das Display von ausführlichen Empfehlungen für die Webseite.

Automatisierung: Vollständiger Autostart-Service (systemd) auf dem Raspberry Pi für 24/7 Betrieb.

🛠 Hardware
Controller: ESP32 (NodeMCU)

Sensoren: * DHT11 (Temperatur & Luftfeuchtigkeit)

HC-SR04 (Ultraschall-Distanz)

LDR (Lichtintensität via Fotowiderstand)

Display: SSD1306 OLED (I2C)

Zentrale: Raspberry Pi 4 (8GB) als MQTT-Broker & KI-Server

💻 Software-Stack
Firmware: C++ / Arduino Framework (PubSubClient, ArduinoJson, WebServer)

Kommunikation: MQTT (Mosquitto Broker)

Backend: Python 3 (paho-mqtt)

KI-Engine: Ollama (Modell: qwen2:0.5b)

Datenhaltung: Lokales CSV-Logging für Langzeitanalysen

📐 Systemarchitektur
Erfassung: ESP32 liest Sensoren und sendet JSON-Rohdaten via MQTT.

Verarbeitung: Python-Skript auf dem Pi empfängt Daten, loggt diese in eine CSV und generiert einen KI-Kontext.

Analyse: Ollama erstellt eine Handlungsanweisung (z.B. "Lüften empfohlen").

Feedback: Die KI-Antwort wird als JSON zurück an den ESP32 gesendet.

Visualisierung: ESP32 zeigt die Daten im lokalen Web-Dashboard (/) und auf dem OLED-Display an.

📂 Projektstruktur
Plaintext
IoT-KI-Umweltsystem/
├── esp32/              # C++ Code für Sensoren, MQTT & Webserver
├── raspi/              # Python-Skript (Brain) & Systemd-Service
├── data/               # CSV-Logdateien
├── docs/               # Dokumentation & Diagramme
└── assets/             # Screenshots vom Web-Dashboard
📈 Zukünftige Erweiterungen
Anbindung einer InfluxDB zur grafischen Auswertung mit Grafana.

Erweiterung um einen Gassensor (MQ-135) zur Luftqualitätsmessung.
Grafische Auswertung der Sensordaten
