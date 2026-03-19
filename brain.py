import paho.mqtt.client as mqtt
import json
import requests
import csv
import os
from datetime import datetime

# --- KONFIGURATION ---
MQTT_BROKER = "localhost" 
OLLAMA_URL = "http://localhost:11434/api/generate"
MODEL_NAME = "qwen2:0.5b"  # Schnelles Modell für Raspberry Pi 4
CSV_FILE = "sensor_daten.csv"
def get_ai_advice(t, l, d):
    # Minimal-Logik: Wir entscheiden VOR der KI, ob es kritisch ist
    if d < 15:
        state = "Danger!"
    elif d < 25:
        state = "Too Close"
    elif t > 30:
        state = "Too Hot"
    else:
        return "System OK"

    # Die KI soll jetzt NUR noch das Wort bestätigen (als Test)
    payload = {
        "model": MODEL_NAME,
        "prompt": f"Summarize this state in 1 word: {state}",
        "stream": False,
        "options": {"num_predict": 5, "temperature": 0.0}
    }
    
    try:
        response = requests.post(OLLAMA_URL, json=payload, timeout=15)
        return response.json()['response'].strip()
    except Exception as e:
        print(f"Fehler: {e}")
        return "AI Error"

def save_to_csv(data):
    """Speichert Daten strukturiert (Punkt 2 deines Plans)."""
    header = ['Zeitpunkt', 'Temp', 'Hum', 'Licht', 'Distanz']
    write_header = not os.path.exists(CSV_FILE)
    
    with open(CSV_FILE, 'a', newline='') as f:
        writer = csv.writer(f)
        if write_header:
            writer.writerow(header)
        writer.writerow([
            datetime.now().strftime("%Y-%m-%d %H:%M:%S"), 
            data.get('t', 0), 
            data.get('h', 0), 
            data.get('l', 0), 
            data.get('d', 0)
        ])

def on_message(client, userdata, msg):
    try:
        # 1. Daten vom ESP32 empfangen
        payload = msg.payload.decode()
        data = json.loads(payload)
        print(f"[{datetime.now().strftime('%H:%M:%S')}] Daten empfangen: {data}")

        # 2. In CSV Datei loggen
        save_to_csv(data)

        # 3. Logik: Wann soll die KI antworten? (Schont den Pi)
        # Wir fragen die KI nur bei Hitze oder wenn etwas zu nah ist
        if data.get('t', 0) > 28 or data.get('d', 0) < 20:
            advice = get_ai_advice(data['t'], data['l'], data['d'])
            print(f"   -> KI Empfehlung: {advice}")
            # Nachricht zurück an den ESP32 (Topic für das OLED)
            client.publish("sensors/esp32/cmd", advice)
        else:
            # Im Normalbetrieb zeigen wir einen Standard-Status
            client.publish("sensors/esp32/cmd", "Status: OK")

    except Exception as e:
        print(f"Fehler bei Nachrichtenverarbeitung: {e}")

# --- MQTT SETUP ---
# VERSION1 ist wichtig für Paho MQTT Library 2.0+
client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION1)

# Falls du ein Passwort vergeben hast (vorhin erwähnt):
client.username_pw_set("dein mqtt_user", "dein Pass")

client.on_message = on_message

print(f"Verbinde mit Broker: {MQTT_BROKER}...")
try:
    client.connect(MQTT_BROKER, 1883, 60)
    client.subscribe("sensors/esp32/data")
    print("KI-Bridge läuft. Warte auf Daten vom ESP32 (Topic: sensors/esp32/data)...")
    client.loop_forever()
except Exception as e:
    print(f"Verbindungsfehler: {e}")
