import os
import requests
import time
import json
from datetime import datetime
import base64
import paho.mqtt.client as mqtt
from dotenv import load_dotenv

load_dotenv()

# ==========================================
# CONFIGURATIONS
# ==========================================
NOM_CAPTEUR_TEMPERATURE = "temperature_ambiant"
LORA_F_PORT = 1

TTN_BROKER = os.getenv("TTN_BROKER", "eu1.cloud.thethings.network")
TTN_USER = os.getenv("TTN_USER")
TTN_PASSWORD = os.getenv("TTN_PASSWORD")
TTN_TOPIC = "v3/+/devices/+/up"

INFLUX_WRITE_URL = os.getenv("INFLUX_WRITE_URL")
INFLUX_QUERY_URL = os.getenv("INFLUX_QUERY_URL")
INFLUX_DATABASE = os.getenv("INFLUX_DATABASE", "Serre-puit")
INFLUX_TOKEN = os.getenv("INFLUX_TOKEN")


# ==========================================
# FONCTIONS INFLUXDB
# ==========================================
def send_to_influxdb(line_protocol_data):
    """Envoie les données du capteur vers InfluxDB."""
    headers = {
        "Authorization": f"Token {INFLUX_TOKEN}",
        "Content-Type": "text/plain; charset=utf-8",
        "Accept": "application/json"
    }
    params = {"db": INFLUX_DATABASE, "precision": "second"}
    try:
        response = requests.post(INFLUX_WRITE_URL, params=params, headers=headers, data=line_protocol_data, timeout=5)
        if response.status_code not in [200, 204]:
            print(f"❌ [INFLUX WRITE] Échec de l'insertion ({response.status_code}) : {response.text}")
    except Exception as e:
        print(f"❌ [INFLUX WRITE] Exception réseau : {e}")


def get_latest_consignes():
    """Récupère les dernières consignes sauvegardées par Grafana dans InfluxDB."""
    headers = {
        "Authorization": f"Token {INFLUX_TOKEN}",
        "Accept": "application/json"
    }
    params = {
        "db": INFLUX_DATABASE,
        "q": 'SELECT * FROM "consignes_ventilateur" ORDER BY time DESC LIMIT 1'
    }
    try:
        response = requests.get(INFLUX_QUERY_URL, params=params, headers=headers, timeout=5)
        if response.status_code == 200:
            data = response.json()

            # Format spécifique à ton API InfluxDB (liste contenant un dictionnaire plat)
            if isinstance(data, list) and len(data) > 0:
                return data[0] # On retourne directement le dictionnaire !
            else:
                print(f"⚠️ [INFLUX QUERY] Aucune donnée trouvée ou liste vide.")
                return None
        else:
            print(f"❌ [INFLUX QUERY] Erreur HTTP ({response.status_code}) : {response.text}")
    except Exception as e:
        print(f"❌ [INFLUX QUERY] Exception réseau : {e}")

    return None


# ==========================================
# FONCTIONS TTN (MQTT & DOWNLINK)
# ==========================================
def envoyer_downlink_ttn(client, device_id, vitesse_pourcentage):
    """Envoie la consigne de vitesse (en %) vers The Things Network via MQTT."""
    topic_downlink = f"v3/{TTN_USER}/devices/{device_id}/down/push"
    vitesse_int = int(vitesse_pourcentage)

    # Encodage de l'entier (0-100) en base64 pour TTN
    payload_b64 = base64.b64encode(bytes([vitesse_int])).decode('utf-8')

    downlink_msg = {
        "downlinks": [{
            "f_port": LORA_F_PORT,
            "frm_payload": payload_b64,
            "priority": "NORMAL"
        }]
    }
    client.publish(topic_downlink, json.dumps(downlink_msg))
    print(f"📥 [TTN] DOWNLINK ENVOYÉ au {device_id} : Vitesse {vitesse_int}%")


def on_connect(client, userdata, flags, rc):
    """Callback déclenché lors de la connexion au broker MQTT."""
    if rc == 0:
        print(" [MQTT] Connecté au broker TTN !")
        client.subscribe(TTN_TOPIC)
    else:
        print(f" [MQTT] Échec de la connexion (Code: {rc})")


def on_message(client, userdata, msg):
    """Callback déclenché à chaque message reçu depuis TTN."""
    try:
        payload = json.loads(msg.payload.decode("utf-8"))
        device_id = payload.get("end_device_ids", {}).get("device_id", "unknown_device")
        uplink_message = payload.get("uplink_message", {})
        decoded_payload = uplink_message.get("decoded_payload")

        if not decoded_payload:
            return

        # 1. SAUVEGARDE DES DONNÉES CAPTEUR DANS INFLUXDB
        base_timestamp = int(time.time())
        champs_influx = []
        for nom_capteur, valeur in decoded_payload.items():
            if nom_capteur not in ["sensor_anomaly", "ventilateur_etats"]:
                val_fmt = f"{float(valeur)}" if isinstance(valeur, (int, float)) else f'"{valeur}"'
                champs_influx.append(f'{nom_capteur}={val_fmt}')

        if champs_influx:
            champs_str = ",".join(champs_influx)
            send_to_influxdb(f'environnement,device={device_id} {champs_str} {base_timestamp}')

        # 2. LOGIQUE DU VENTILATEUR (DOWNLINK)
        consignes = get_latest_consignes()
        temp_actuelle = decoded_payload.get(NOM_CAPTEUR_TEMPERATURE, 0)

        if consignes:
            seuil_temp = consignes.get("seuil_temp", 999)

            # OVERRIDE : Si la température dépasse le seuil max défini dans Grafana
            if temp_actuelle >= seuil_temp:
                vitesse_cible = consignes.get("seuil_vitesse", 100)
                print(f" [ALERTE] Temp {temp_actuelle}°C >= {seuil_temp}°C : Override activé ! ({vitesse_cible}%)")

            # NORMAL : Suivi de la courbe dessinée dans Grafana selon l'heure
            else:
                maintenant = datetime.now()
                minute_arrondie = "30" if maintenant.minute >= 30 else "00"

            
                tranche_actuelle = f"v_{maintenant.hour:02d}_{minute_arrondie}"

                vitesse_cible = consignes.get(tranche_actuelle, 50) # 50% par défaut si tranche introuvable
                print(f" [NORMAL] Tranche {tranche_actuelle} -> Cible : {vitesse_cible}%")

            # Envoi de l'ordre au ventilateur
            envoyer_downlink_ttn(client, device_id, vitesse_cible)
        else:
            print(f" [{device_id}] Aucune consigne trouvée dans InfluxDB (attente d'envoi via Grafana).")

    except Exception as e:
        print(f" [MQTT] Erreur inattendue dans la réception TTN : {e}")


# ==========================================
# POINT D'ENTRÉE
# ==========================================
if __name__ == "__main__":
    print("Démarrage de la passerelle TTN <> InfluxDB...")

    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION1)
    client.username_pw_set(username=TTN_USER, password=TTN_PASSWORD)
    client.on_connect = on_connect
    client.on_message = on_message

    try:
        client.connect(TTN_BROKER, port=1883, keepalive=60)
        # loop_forever() bloque le script et écoute les messages MQTT indéfiniment
        client.loop_forever()
    except KeyboardInterrupt:
        print(" Arrêt manuel du script.")
    except Exception as e:
        print(f"Erreur critique au lancement : {e}")
