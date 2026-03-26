import paho.mqtt.client as mqtt
import json
import requests
import csv
import os
from datetime import datetime

# --- KONFIGURATION ---
MQTT_BROKER = "localhost"
OLLAMA_URL = "http://localhost:11434/api/generate"
MODEL_NAME = "qwen2:0.5b"
CSV_FILE = "sensor_daten.csv"

def get_ai_advice(t, l, d, long_version=False):
    """
    Holt KI-Rat. 
    long_version=False -> Kurzes Wort fürs OLED.
    long_version=True  -> Ausführlicher Satz für die Webseite.
    """
    # Zustandsbeschreibung für den Prompt
    if d < 20: status = "Objekt sehr nah"
    elif t > 28: status = "Es ist zu warm"
    elif l == 1: status = "Es ist dunkel" # 1 = Dunkel laut deinem ESP-Code
    else: status = "Alles im grünen Bereich"

    if long_version:
        prompt = f"Du bist ein smarter Raum-Assistent. Status: {status}. Temperatur: {t}C, Distanz: {d}cm. Gib eine hilfreiche Empfehlung in EINEM kurzen, freundlichen Satz auf Deutsch."
        num_predict = 50 # Mehr Text für die Webseite
    else:
        prompt = f"Summarize this in exactly 1 or 2 words: {status}"
        num_predict = 5 # Ganz kurz fürs OLED

    payload = {
        "model": MODEL_NAME,
        "prompt": prompt,
        "stream": False,
        "options": {"num_predict": num_predict, "temperature": 0.7}
    }

    try:
        response = requests.post(OLLAMA_URL, json=payload, timeout=15)
        return response.json()['response'].strip()
    except Exception as e:
        return "KI Fehler"

def save_to_csv(data):
    header = ['Zeitpunkt', 'Temp', 'Hum', 'Licht', 'Distanz']
    write_header = not os.path.exists(CSV_FILE)
    with open(CSV_FILE, 'a', newline='') as f:
        writer = csv.writer(f)
        if write_header: writer.writerow(header)
        writer.writerow([
            datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
            data.get('t', 0), data.get('h', 0), data.get('l', 0), data.get('d', 0)
        ])

def on_message(client, userdata, msg):
    try:
        payload = msg.payload.decode()
        data = json.loads(payload)
        print(f"[{datetime.now().strftime('%H:%M:%S')}] Daten empfangen.")

        save_to_csv(data)

        # Logik für zwei verschiedene Nachrichten
        if data.get('t', 0) > 28 or data.get('d', 0) < 20 or data.get('l', 0) == 1:
            # 1. Lange Nachricht für die Webseite generieren
            web_advice = get_ai_advice(data['t'], data['l'], data['d'], long_version=True)
            # 2. Kurze Nachricht fürs OLED generieren
            oled_advice = get_ai_advice(data['t'], data['l'], data['d'], long_version=False)
        else:
            web_advice = "Das System läuft stabil. Alle Parameter sind im optimalen Bereich."
            oled_advice = "Status: OK"

        # Wir packen BEIDE Nachrichten in ein JSON-Paket zurück an den ESP32
        # So kann der ESP entscheiden, was er wo anzeigt
        combined_msg = {
            "oled": oled_advice,
            "web": web_advice
        }
        client.publish("sensors/esp32/cmd", json.dumps(combined_msg))
        print(f"Gesendet -> OLED: {oled_advice} | WEB: {web_advice}")

    except Exception as e:
        print(f"Fehler: {e}")

# --- MQTT SETUP ---
client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION1)
client.username_pw_set("mqttuser", "SchrottKiste")
client.on_message = on_message

try:
    client.connect(MQTT_BROKER, 1883, 60)
    client.subscribe("sensors/esp32/data")
    print("KI-Bridge mit Web-Support läuft...")
    client.loop_forever()
except Exception as e:
    print(f"Verbindungsfehler: {e}")
