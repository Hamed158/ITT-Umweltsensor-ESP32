ITT-Projekt: Raum-Monitor mit lokaler KI
Dieses Projekt kombiniert klassische Sensordatenerfassung mit einer lokalen KI-Auswertung. Der Fokus lag dabei auf Edge Computing – also die Datenverarbeitung direkt vor Ort auf einem Raspberry Pi zu machen, statt sie in eine Cloud zu schicken.

Was das System macht
Ein ESP32 sammelt Daten von verschiedenen Sensoren (Temperatur, Licht, Abstand) und schickt diese per MQTT an einen Raspberry Pi. Dort läuft ein Python-Skript, das die Werte in eine CSV-Datei loggt und gleichzeitig das KI-Modell Qwen2 (via Ollama) fragt, was es von den aktuellen Werten hält. Die Antwort wird zurück an den ESP32 gesendet.

Die Besonderheiten beim Aufbau
Duale Anzeige: Die KI-Antwort wird geteilt. Auf dem kleinen OLED-Display am Gerät erscheint nur ein kurzes Fazit (z.B. "Lüften!"), während auf dem Web-Dashboard des ESP32 der komplette Text der KI steht.

Eigener Webserver: Der ESP32 hostet eine Webseite, auf der man die Live-Daten und die KI-Empfehlung im Browser sehen kann.

Autostart: Damit man nicht jedes Mal alles händisch starten muss, habe ich auf dem Pi einen Systemd-Service eingerichtet. Sobald der Pi Strom bekommt, läuft das Skript im Hintergrund los.

Hardware-Setup
ESP32 als Haupteinheit.

DHT11 & HC-SR04 & LDR für die Umgebungswerte.

SSD1306 OLED für die lokale Anzeige.

Raspberry Pi 4 als MQTT-Broker und KI-Server.

Warum die KI-Lösung?
Klar könnte man auch einfach if-else Regeln schreiben (z.B. wenn Temp > 25, dann Warnung). Die KI kann aber mehrere Sensoren gleichzeitig bewerten und gibt natürlichere Tipps. Da das Modell lokal auf dem Pi läuft, bleiben alle Daten privat im eigenen Netzwerk.

Zukünftige Erweiterungen
Anbindung einer InfluxDB zur grafischen Auswertung mit Grafana.

Erweiterung um einen Gassensor (MQ-135) zur Luftqualitätsmessung.
