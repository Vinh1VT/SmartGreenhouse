import os
import requests
import json
import time
import calendar
from datetime import datetime
import paho.mqtt.client as mqtt

from dotenv import load_dotenv
load_dotenv()

# ==========================================
# VARIABLES D'ENVIRONNEMENT
# ==========================================

# TTN
TTN_BROKER = os.getenv("TTN_BROKER")
TTN_USER = os.getenv("TTN_USER")
TTN_PASSWORD = os.getenv("TTN_PASSWORD")
TTN_TOPIC = "v3/+/devices/+/up"

# InfluxDB
INFLUX_URL = os.getenv("INFLUX_URL")
INFLUX_ORG = os.getenv("INFLUX_ORG")
INFLUX_BUCKET = os.getenv("INFLUX_BUCKET")
INFLUX_TOKEN = os.getenv("INFLUX_TOKEN")

# Certificat SSL
SSL_CERT_PATH = os.getenv("SSL_CERT_PATH")

if not all([TTN_USER, TTN_PASSWORD, TTN_BROKER, INFLUX_URL, INFLUX_ORG, INFLUX_BUCKET, INFLUX_TOKEN, SSL_CERT_PATH]):
    print("Erreur : Variables d'environnement manquantes. Vérifiez votre fichier .env")
    exit(1)


# ==========================================
# FONCTIONS MQTT
# ==========================================

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connecté au broker TTN avec succès")
        client.subscribe(TTN_TOPIC)
        print(f"Souscrit au topic : {TTN_TOPIC}")
    else:
        print(f"Échec de la connexion (Code: {rc})")


def on_message(client, userdata, msg):
    try:
        payload = json.loads(msg.payload.decode("utf-8"))

        device_id = payload.get("end_device_ids", {}).get("device_id", "unknown_device")
        uplink_message = payload.get("uplink_message", {})
        decoded_payload = uplink_message.get("decoded_payload")

        if not decoded_payload:
            print(f"[{device_id}] Aucun payload décodé, message ignoré")
            return

        # 1. RÉCUPÉRATION DE L'HEURE EXACTE TTN
        ttn_time_str = uplink_message.get("received_at")
        if ttn_time_str:
            clean_time = ttn_time_str.split('.')[0].replace('Z', '')
            dt = datetime.strptime(clean_time, "%Y-%m-%dT%H:%M:%S")
            base_timestamp = calendar.timegm(dt.timetuple())
        else:
            base_timestamp = int(time.time())

        # 2. TAGS DE BASE (Communs à tous les capteurs)
        anomaly = decoded_payload.pop("sensor_anomaly", False)
        base_tags = f"device={device_id},anomalie={'true' if anomaly else 'false'}"

        lines_to_send = []

        # 3. EXTRACTION ET RANGEMENT DES CAPTEURS
        fan_states = decoded_payload.pop("ventilateur_etats", None)

        for key, value in decoded_payload.items():
            specific_tags = ""
            field_name = "valeur" # Sécurité par défaut
            
            # --- TEMPÉRATURE ---
            if key.startswith("temperature_"):
                position = key.replace("temperature_", "")
                specific_tags = f",type_capteur=temperature,position={position}"
                field_name = "temperature"

            # --- HUMIDITÉ ---
            elif key.startswith("humidite_"):
                parts = key.split("_")
                position = parts[1]
                profondeur = f",profondeur={parts[2]}" if len(parts) > 2 else ""
                specific_tags = f",type_capteur=humidite,position={position}{profondeur}"
                field_name = "humidite"

            # --- LUMINANCE ---
            elif key.startswith("luminance_"):
                position = key.replace("luminance_", "")
                specific_tags = f",type_capteur=luminance,position={position}"
                field_name = "luminance"

            # --- GAZ ---
            elif key in ["co2", "o2"]:
                specific_tags = f",type_capteur=gaz,type_gaz={key}"
                field_name = "concentration"

            # Assemblage de la ligne InfluxDB
            lines_to_send.append(f"environnement,{base_tags}{specific_tags} {field_name}={value} {base_timestamp}")

        # 4. TRAITEMENT DU VENTILATEUR (Historisé)
        if fan_states and isinstance(fan_states, list) and len(fan_states) > 0:
            cycle_total_secondes = 30 * 60
            nb_tranches = len(fan_states)
            duree_tranche = cycle_total_secondes // nb_tranches

            for i, state in enumerate(fan_states):
                ts_tranche = base_timestamp - ((nb_tranches - 1 - i) * duree_tranche)
                specific_tags = ",type_capteur=ventilateur,composant=moteur"
                lines_to_send.append(f"environnement,{base_tags}{specific_tags} etat={state} {ts_tranche}")

        # 5. ENVOI EN BLOC VERS INFLUXDB
        if lines_to_send:
            line_protocol_data = "\n".join(lines_to_send)
            print(f"[{device_id}] Envoi de {len(lines_to_send)} points à InfluxDB...")
            send_to_influxdb(line_protocol_data)

    except json.JSONDecodeError:
        print("Erreur : Le JSON reçu est invalide")
    except Exception as e:
        print(f"Erreur inattendue : {e}")


# ==========================================
# FONCTIONS HTTP INFLUXDB
# ==========================================

def send_to_influxdb(line_protocol_data):
    headers = {
        "Authorization": f"Bearer {INFLUX_TOKEN}",
        "Content-Type": "text/plain; charset=utf-8",
        "Accept": "application/json"
    }

    params = {
        "org": INFLUX_ORG,
        "bucket": INFLUX_BUCKET,
        "precision": "s"
    }

    try:
        # Note : INFLUX_URL doit pointer vers l'endpoint d'écriture, ex: "https://serveur/api/v2/write"
        response = requests.post(INFLUX_URL, params=params, headers=headers, data=line_protocol_data, verify=SSL_CERT_PATH, timeout=5)

        if response.status_code == 204:
            print("-> Données insérées avec succès")
        else:
            print(f"-> Échec de l'insertion ({response.status_code}) : {response.text}")

    except requests.exceptions.RequestException as e:
        print(f"-> Exception réseau InfluxDB : {e}")


# ==========================================
# POINT D'ENTRÉE
# ==========================================

if __name__ == "__main__":
    print("Démarrage du pont MQTT vers InfluxDB...")
    
    # Configuration du client MQTT
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION1)
    client.username_pw_set(username=TTN_USER, password=TTN_PASSWORD)
    client.on_connect = on_connect
    client.on_message = on_message

    try:
        client.connect(TTN_BROKER, port=1883, keepalive=60)
        client.loop_forever()
    except KeyboardInterrupt:
        print("\nArrêt manuel du script.")
        client.disconnect()
    except Exception as e:
        print(f"Erreur fatale au lancement : {e}")
