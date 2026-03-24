import os
import requests
import json
import time
import calendar
from datetime import datetime
import paho.mqtt.client as mqtt
import base64

from dotenv import load_dotenv

load_dotenv()

# ==========================================
# CONFIGURATION SPÉCIFIQUE AU DOWNLINK
# ==========================================
# ⚠️ Remplace par le nom exact de ta variable de température dans TTN
NOM_CAPTEUR_TEMPERATURE = "temperature_ambiant" # J'ai mis à jour avec le nom probable de ton capteur
# Port LoRaWAN cible pour les commandes
LORA_F_PORT = 1

# ==========================================
# VARIABLES D'ENVIRONNEMENT
# ==========================================

TTN_BROKER = os.getenv("TTN_BROKER", "eu1.cloud.thethings.network")
TTN_USER = os.getenv("TTN_USER")      # Correspond à TTN_APP_ID
TTN_PASSWORD = os.getenv("TTN_PASSWORD")  # Correspond à TTN_API_KEY
TTN_TOPIC = "v3/+/devices/+/up"

INFLUX_WRITE_URL = os.getenv("INFLUX_WRITE_URL")
INFLUX_DATABASE = os.getenv("INFLUX_DATABASE")
INFLUX_TOKEN = os.getenv("INFLUX_TOKEN")

if not all([TTN_USER, TTN_PASSWORD, TTN_BROKER, INFLUX_WRITE_URL, INFLUX_DATABASE, INFLUX_TOKEN]):
    print("Erreur : Variables d'environnement manquantes. Vérifiez votre fichier .env")
    exit(1)

# ==========================================
# CACHE EN MÉMOIRE
# ==========================================
# Ce Set va stocker les identifiants uniques "device_nomDuCapteur"
capteurs_connus = set()

# ==========================================
# FONCTIONS MQTT ET TTN
# ==========================================

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connecté au broker TTN avec succès")
        client.subscribe(TTN_TOPIC)
        print(f"Souscrit au topic : {TTN_TOPIC}")
    else:
        print(f"Échec de la connexion (Code: {rc})")

def envoyer_downlink_ttn(client, device_id, vitesse_pourcentage):
    """Envoie un ordre de modification de vitesse au ventilateur via TTN"""
    topic_downlink = f"v3/{TTN_USER}/devices/{device_id}/down/push"

    vitesse_int = int(vitesse_pourcentage)
    payload_b64 = base64.b64encode(bytes([vitesse_int])).decode('utf-8')

    downlink_msg = {
        "downlinks": [{
            "f_port": LORA_F_PORT,
            "frm_payload": payload_b64,
            "priority": "NORMAL"
        }]
    }

    client.publish(topic_downlink, json.dumps(downlink_msg))
    print(f"[{device_id}] 📥 DOWNLINK ENVOYÉ : Vitesse réglée sur {vitesse_int}%")

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

        # 2. FONCTIONS D'AIDE
        def format_influx_value(v):
            if isinstance(v, bool):
                return "1.0" if v else "0.0"
            elif isinstance(v, (int, float)):
                return f"{float(v)}"
            else:
                return f'"{v}"'

        # Liste qui va contenir toutes nos colonnes "capteur=valeur"
        champs_influx = []

        # 3. GESTION DE L'ANOMALIE
        anomaly = decoded_payload.pop("sensor_anomaly", False)
        champs_influx.append(f'sensor_anomaly={format_influx_value(anomaly)}')

        # 4. EXTRACTION DES CAPTEURS ET MESURES
        fan_states = decoded_payload.pop("ventilateur_etats", None)

        for nom_capteur, valeur in decoded_payload.items():
            valeur_formatee = format_influx_value(valeur)
            champs_influx.append(f'{nom_capteur}={valeur_formatee}')

        # On assemble et on crée la ligne principale pour InfluxDB (Format Wide)
        if champs_influx:
            champs_str = ",".join(champs_influx)
            lines_to_send.append(f'environnement,device={device_id} {champs_str} {base_timestamp}')

        # 5. TRAITEMENT DU VENTILATEUR (Historisé)
        if fan_states and isinstance(fan_states, list) and len(fan_states) > 0:
            cycle_total_secondes = 30 * 60
            nb_tranches = len(fan_states)
            duree_tranche = cycle_total_secondes // nb_tranches

            for i, state in enumerate(fan_states):
                ts_tranche = base_timestamp - ((nb_tranches - 1 - i) * duree_tranche)
                valeur_state = format_influx_value(state)
                lines_to_send.append(
                    f'environnement,device={device_id} ventilateur_etats={valeur_state} {ts_tranche}'
                )

        # 6. ENVOI EN BLOC VERS INFLUXDB
        if lines_to_send:
            line_protocol_data = "\n".join(lines_to_send)
            print(f"[{device_id}] Envoi de {len(lines_to_send)} points à InfluxDB...")
            send_to_influxdb(line_protocol_data)

        # ==========================================
        # 7. LOGIQUE DE DESCENTE (DOWNLINK)
        # ==========================================
        temp_actuelle = decoded_payload.get(NOM_CAPTEUR_TEMPERATURE, 0)

        consignes = lire_consigne_influxdb()

        if consignes:
            vitesse_cible = 0
            seuil_temp = consignes.get("seuil_temp", 999)

            # RÈGLE 1 : OVERRIDE DE TEMPÉRATURE (Sécurité)
            if temp_actuelle >= seuil_temp:
                vitesse_cible = consignes.get("seuil_vitesse", 100)
                print(f"[{device_id}] ⚠️ Alerte Temp ({temp_actuelle}°C >= {seuil_temp}°C) : Override activé !")

            # RÈGLE 2 : SUIVI DE LA COURBE HORAIRE (Fonctionnement normal)
            else:
                maintenant = datetime.now()
                minute_arrondie = "30" if maintenant.minute >= 30 else "00"
                tranche_actuelle = f"{maintenant.hour:02d}:{minute_arrondie}"

                vitesse_cible = consignes.get(tranche_actuelle, 20) # 20% par défaut
                print(f"[{device_id}] 🕒 Tranche horaire {tranche_actuelle} -> Cible : {vitesse_cible}%")

            envoyer_downlink_ttn(client, device_id, vitesse_cible)
        else:
            print(f"[{device_id}] ❌ Aucune consigne trouvée dans InfluxDB. Downlink ignoré.")

    except json.JSONDecodeError:
        print("Erreur : Le JSON reçu est invalide")
    except Exception as e:
        print(f"Erreur inattendue : {e}")

# ==========================================
# FONCTIONS HTTP INFLUXDB
# ==========================================

def send_to_influxdb(line_protocol_data):
    headers = {
        "Authorization": f"Token {INFLUX_TOKEN}",
        "Content-Type": "text/plain; charset=utf-8",
        "Accept": "application/json"
    }

    params = {
        "db": INFLUX_DATABASE,
        "precision": "second"
    }

    try:
        response = requests.post(
            INFLUX_WRITE_URL,
            params=params,
            headers=headers,
            data=line_protocol_data,
            timeout=5
        )

        if response.status_code in [200, 204]:
            print("-> Données insérées avec succès")
        else:
            print(f"-> Échec de l'insertion ({response.status_code}) : {response.text}")

    except requests.exceptions.RequestException as e:
        print(f"-> Exception réseau InfluxDB : {e}")

def lire_consigne_influxdb():
    read_url = INFLUX_WRITE_URL.replace("/write", "/query").replace("/api/v2/write", "/query")

    headers = {
        "Authorization": f"Token {INFLUX_TOKEN}",
        "Accept": "application/json"
    }

    params = {
        "db": INFLUX_DATABASE,
        "q": "SELECT last(\"payload\") FROM \"consigne_ventilateur\""
    }

    try:
        response = requests.get(read_url, params=params, headers=headers, timeout=5)
        if response.status_code == 200:
            data = response.json()
            series = data.get("results", [{}])[0].get("series", [])
            if series:
                valeur_texte = series[0]["values"][0][1]
                return json.loads(valeur_texte)
    except Exception as e:
        print(f"-> Erreur lors de la lecture de la consigne InfluxDB : {e}")

    return None

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
