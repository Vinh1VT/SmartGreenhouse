import os
import requests
import json
import time
from datetime import datetime
import base64
import paho.mqtt.client as mqtt

from fastapi import FastAPI, Request
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import JSONResponse
import uvicorn
from dotenv import load_dotenv

load_dotenv()

# ==========================================
# CONFIGURATIONS
# ==========================================
NOM_CAPTEUR_TEMPERATURE = "temperature_ambiant"
LORA_F_PORT = 1
FICHIER_CONSIGNE = "consignes_sauvegarde.json"

TTN_BROKER = os.getenv("TTN_BROKER", "eu1.cloud.thethings.network")
TTN_USER = os.getenv("TTN_USER")
TTN_PASSWORD = os.getenv("TTN_PASSWORD")
TTN_TOPIC = "v3/+/devices/+/up"

INFLUX_WRITE_URL = os.getenv("INFLUX_WRITE_URL")
INFLUX_DATABASE = os.getenv("INFLUX_DATABASE")
INFLUX_TOKEN = os.getenv("INFLUX_TOKEN")

# ==========================================
# SERVEUR WEB FASTAPI (Écoute Grafana)
# ==========================================
app = FastAPI(title="Passerelle Grafana-TTN")

# Configuration du CORS pour autoriser le navigateur de Grafana
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],  # En production, on peut mettre l'IP de Grafana ici
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)


@app.post('/update_consignes')
async def update_consignes(request: Request):
    try:
        # Récupération asynchrone du JSON envoyé par Grafana
        data = await request.json()

        # Sauvegarde locale immédiate
        with open(FICHIER_CONSIGNE, "w") as f:
            json.dump(data, f)

        print("✅ [WEB] Nouvelles consignes reçues de Grafana et sauvegardées !")
        return {"status": "success", "message": "Consignes appliquées"}

    except Exception as e:
        print(f"❌ [WEB] Erreur lors de la réception : {e}")
        return JSONResponse(status_code=400, content={"status": "error", "message": str(e)})

@app.get('/get_consignes')
async def get_consignes():
    try:
        consignes = lire_consignes_locales()
        if consignes:
            return consignes
        else:
            # Si le fichier n'existe pas encore, on renvoie un dictionnaire vide
            return {}
    except Exception as e:
        return JSONResponse(status_code=500, content={"status": "error", "message": str(e)})


def lire_consignes_locales():
    if os.path.exists(FICHIER_CONSIGNE):
        with open(FICHIER_CONSIGNE, "r") as f:
            return json.load(f)
    return None


# ==========================================
# FONCTIONS INFLUXDB & TTN
# ==========================================
def send_to_influxdb(line_protocol_data):
    headers = {
        "Authorization": f"Token {INFLUX_TOKEN}",
        "Content-Type": "text/plain; charset=utf-8",
        "Accept": "application/json"
    }
    params = {"db": INFLUX_DATABASE, "precision": "second"}
    try:
        response = requests.post(INFLUX_WRITE_URL, params=params, headers=headers, data=line_protocol_data, timeout=5)
        if response.status_code not in [200, 204]:
            print(f"❌ [INFLUX] Échec de l'insertion ({response.status_code}) : {response.text}")
    except Exception as e:
        print(f"❌ [INFLUX] Exception réseau : {e}")


def envoyer_downlink_ttn(client, device_id, vitesse_pourcentage):
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
    print(f"📥 [TTN] DOWNLINK ENVOYÉ au {device_id} : Vitesse {vitesse_int}%")


def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("✅ [MQTT] Connecté au broker TTN !")
        client.subscribe(TTN_TOPIC)
    else:
        print(f"❌ [MQTT] Échec de la connexion (Code: {rc})")


def on_message(client, userdata, msg):
    try:
        payload = json.loads(msg.payload.decode("utf-8"))
        device_id = payload.get("end_device_ids", {}).get("device_id", "unknown_device")
        uplink_message = payload.get("uplink_message", {})
        decoded_payload = uplink_message.get("decoded_payload")

        if not decoded_payload:
            return

        # 1. Traitement InfluxDB
        base_timestamp = int(time.time())
        champs_influx = []
        for nom_capteur, valeur in decoded_payload.items():
            if nom_capteur not in ["sensor_anomaly", "ventilateur_etats"]:
                val_fmt = f"{float(valeur)}" if isinstance(valeur, (int, float)) else f'"{valeur}"'
                champs_influx.append(f'{nom_capteur}={val_fmt}')

        if champs_influx:
            champs_str = ",".join(champs_influx)
            send_to_influxdb(f'environnement,device={device_id} {champs_str} {base_timestamp}')

        # 2. Logique de Downlink
        temp_actuelle = decoded_payload.get(NOM_CAPTEUR_TEMPERATURE, 0)
        consignes = lire_consignes_locales()

        if consignes:
            seuil_temp = consignes.get("seuil_temp", 999)

            # OVERRIDE TEMPÉRATURE
            if temp_actuelle >= seuil_temp:
                vitesse_cible = consignes.get("seuil_vitesse", 100)
                print(f"⚠️ [ALERTE] Temp {temp_actuelle}°C >= {seuil_temp}°C : Override activé ! ({vitesse_cible}%)")

            # COURBE HORAIRE NORMAL
            else:
                maintenant = datetime.now()
                minute_arrondie = "30" if maintenant.minute >= 30 else "00"
                tranche_actuelle = f"{maintenant.hour:02d}:{minute_arrondie}"
                vitesse_cible = consignes.get(tranche_actuelle, 20)
                print(f"🕒 [NORMAL] Tranche {tranche_actuelle} -> Cible : {vitesse_cible}%")

            envoyer_downlink_ttn(client, device_id, vitesse_cible)
        else:
            print(f"⚠️ [{device_id}] Aucune consigne configurée (attente de Grafana).")

    except Exception as e:
        print(f"❌ [MQTT] Erreur inattendue dans la réception TTN : {e}")


# ==========================================
# POINT D'ENTRÉE
# ==========================================
if __name__ == "__main__":
    print("Démarrage du systeme..")

    # 1. Configuration et lancement du client MQTT en tâche de fond
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION1)
    client.username_pw_set(username=TTN_USER, password=TTN_PASSWORD)
    client.on_connect = on_connect
    client.on_message = on_message

    try:
        client.connect(TTN_BROKER, port=1883, keepalive=60)
        # loop_start() crée un thread automatiquement, plus besoin de le gérer nous-mêmes !
        client.loop_start()
    except Exception as e:
        print(f"❌ [MQTT] Erreur critique au lancement : {e}")

    # 2. Lancement de FastAPI via Uvicorn sur le thread principal
    # On écoute sur le port 5000 pour rester compatible avec ton code JS
    uvicorn.run(app, host="0.0.0.0", port=5000)