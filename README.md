# ITT-Umweltsensor-ESP32
# Entwicklung eines IoT-Systems zur Umweltdatenerfassung mit KI-gestützter Datenanalyse auf einem lokalen Edge-Server.
# IoT-Umweltsystem mit KI-Analyse

Projektübersicht:
Dieses Projekt ist ein IoT-System zur Erfassung von Umweltdaten mit anschließender KI-gestützter Analyse auf einem lokalen Server.
Ein Mikrocontroller erfasst Sensordaten und überträgt diese per MQTT an einen Raspberry Pi.
Der Raspberry Pi verarbeitet die Daten und analysiert sie mithilfe eines lokalen KI-Servers.

Projektziel:
Ziel des Projektes ist der Aufbau eines verteilten IoT-Systems mit:
Sensordatenerfassung
Netzwerkkommunikation über MQTT
Edge-Computing auf einem lokalen Server
KI-basierter Datenauswertung

Verwendete Hardware:
ESP32
DHT11 (Temperatur & Luftfeuchtigkeit)
HC-SR04 Ultraschallsensor (Abstand / Hindernis)
Lichtsensor (Helligkeit)
Raspberry Pi 4
Breadboard & Jumper-Kabel

Verwendete Software:
Arduino IDE
Mqtt Explorer (MQTT Broker)
Raspberry Pi OS
Python (MQTT Subscriber & KI-Analyse)
Ollama (Lokaler KI-Server)

Systemarchitektur:
ESP32 liest Sensordaten
Temperatur
Luftfeuchtigkeit
Helligkeit
Abstand
Daten werden per MQTT an den Raspberry Pi gesendet.

Der Raspberry Pi:
Empfängt die Daten
Speichert sie (optional)
Analysiert sie mit einem lokalen KI-Modell

MQTT Topics (Beispiel):
iot/temperature
iot/humidity
iot/light
iot/distance

KI-Funktion:
Die KI bewertet eingehende Sensordaten und gibt eine textuelle Interpretation aus, z. B.:
„Die Temperatur ist sehr hoch.“
„Ein Hindernis befindet sich in geringer Entfernung.“
„Die Lichtintensität ist stark.“
Die Verarbeitung erfolgt lokal auf dem Raspberry Pi (Edge-Computing).

 Projektstruktur:
IoT-KI-Umweltsystem/
│
├── esp32/
├── raspi/
├── docs/
└── bilder/

Erweiterungsmöglichkeiten:
Web-Dashboard
Datenbank (z. B. SQLite)
Automatische Warnmeldungen
Grafische Auswertung der Sensordaten
