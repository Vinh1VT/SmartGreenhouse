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

TTN_BROKER = os.getenv("TTN_BROKER")
TTN_USER = os.getenv("TTN_USER")
TTN_PASSWORD = os.getenv("TTN_PASSWORD")
TTN_TOPIC = "v3/+/devices/+/up"

INFLUX_URL = os.getenv("INFLUX_URL")
INFLUX_ORG = os.getenv("INFLUX_ORG")
INFLUX_BUCKET = os.getenv("INFLUX_BUCKET")
INFLUX_TOKEN = os.getenv("INFLUX_TOKEN")
SSL_CERT_PATH = os.getenv("SSL_CERT_PATH")

if not all([TTN_USER, TTN_PASSWORD, TTN_BROKER, INFLUX_URL, INFLUX_ORG, INFLUX_BUCKET, INFLUX_TOKEN, SSL_CERT_PATH]):
    print("Erreur : Variables d'environnement manquantes. Vérifiez votre fichier .env")
    exit(1)

# ==========================================
# CACHE EN MÉMOIRE
# ==========================================
# Ce Set va stocker les identifiants uniques "device_nomDuCapteur"
capteurs_connus = set()


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

        # 1. RÉCUPÉRATION DE L'HEURE
        ttn_time_str = uplink_message.get("received_at")
        if ttn_time_str:
            clean_time = ttn_time_str.split('.')[0].replace('Z', '')
            dt = datetime.strptime(clean_time, "%Y-%m-%dT%H:%M:%S")
            base_timestamp = calendar.timegm(dt.timetuple())
        else:
            base_timestamp = int(time.time())

        lines_to_send = []

        # 2. FONCTION D'AIDE POUR L'AJOUT UNIQUE
        def ajouter_capteur_si_nouveau(nom_capteur, timestamp):
            identifiant_unique = f"{device_id}_{nom_capteur}"
            if identifiant_unique not in capteurs_connus:
                # C'est un nouveau capteur ! On l'ajoute au cache et on l'envoie à la DB
                capteurs_connus.add(identifiant_unique)
                lines_to_send.append(f'capteurs,device={device_id} nom="{nom_capteur}" {timestamp}')

        # 3. GESTION DE L'ANOMALIE
        anomaly = decoded_payload.pop("sensor_anomaly", False)
        ajouter_capteur_si_nouveau("sensor_anomaly", base_timestamp)
        lines_to_send.append(
            f'mesures_puits,device={device_id},nom=sensor_anomaly valeur={"true" if anomaly else "false"} {base_timestamp}')

        # 4. EXTRACTION DES CAPTEURS ET MESURES
        fan_states = decoded_payload.pop("ventilateur_etats", None)

        for nom_capteur, valeur in decoded_payload.items():
            # Ajout dans la table capteurs UNIQUEMENT si c'est la première fois
            ajouter_capteur_si_nouveau(nom_capteur, base_timestamp)

            # Ajout dans la table mesures_puits (À CHAQUE FOIS)
            lines_to_send.append(f'mesures_puits,device={device_id},nom={nom_capteur} valeur={valeur} {base_timestamp}')

        # 5. TRAITEMENT DU VENTILATEUR (Historisé)
        if fan_states and isinstance(fan_states, list) and len(fan_states) > 0:
            cycle_total_secondes = 30 * 60
            nb_tranches = len(fan_states)
            duree_tranche = cycle_total_secondes // nb_tranches
            nom_ventilo = "ventilateur_etats"

            # Enregistrement du ventilateur dans la table capteurs (une seule fois)
            ajouter_capteur_si_nouveau(nom_ventilo, base_timestamp)

            for i, state in enumerate(fan_states):
                ts_tranche = base_timestamp - ((nb_tranches - 1 - i) * duree_tranche)
                lines_to_send.append(f'mesures_puits,device={device_id},nom={nom_ventilo} valeur={state} {ts_tranche}')

        # 6. ENVOI EN BLOC VERS INFLUXDB
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
        response = requests.post(INFLUX_URL, params=params, headers=headers, data=line_protocol_data,
                                 verify=SSL_CERT_PATH, timeout=5)

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