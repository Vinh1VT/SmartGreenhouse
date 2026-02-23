import os
import requests
import json
import paho.mqtt.client as mqtt


#Ouvertures des variables d'environnement


#TTN
TTN_BROKER = os.getenv("TTN_BROKER")
TTN_USER = os.getenv("TTN_USER")
TTN_PASSWORD = os.getenv("TTN_PASSWORD")
TTN_TOPIC = "v3/+/devices/+.up"

#InfluxDB
INFLUX_URL = os.getenv("INFLUX_URL")
INFLUX_ORG = os.getenv("INFLUX_ORG")
INFLUX_BUCKET = os.getenv("INFLUX_BUCKET")
INFLUX_TOKEN = os.getenv("INFLUX_TOKEN")

#Certifical ssl
SSL_CERT_PATH = os.getenv("SSL_CERT_PATH")



if not all([TTN_USER,TTN_PASSWORD,TTN_BROKER,INFLUX_URL,INFLUX_ORG,INFLUX_BUCKET,INFLUX_TOKEN,SSL_CERT_PATH]):
    print("Variables d'environnement non initialisées")
    exit(1)


##### FONCTION MQTT

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to broker")
        client.subscribe(TTN_TOPIC)
        print("Subscribed to topic")
    else :
        print("Connection failed")


def on_message(client, userdata, msg):
    #Faire le code des messages reçus (besoin d'avoir bien tout les capteurs)
    return
###### FONCTIONS HTTP INFLUXDB

def send_to_influxdb(line_protocol_data):
    headers = {
        "Authorization": f"Bearer {INFLUX_TOKEN}",
        "Content-Type": "text/plain; charset=utf-8",
        "Accept": "application/json"
    }


    params = {
        "org": INFLUX_ORG,
        "bucket": INFLUX_BUCKET,
        "precision" : "s"
    }


    try :
        response = requests.request("POST", INFLUX_URL, params=params,headers=headers, data=line_protocol_data,verify=SSL_CERT_PATH,timeout=5)

        if response.status_code == 204 :
            print("Data pushed")
        else :
            print(f"Failes to push data {response.status_code} : {response.text}")

    except requests.exceptions.RequestException as e :
        print(f"Exception : {e}")


if __name__ == "__main__":
    client = mqtt.Client()
    client.username_pw_set(username=TTN_USER, password=TTN_PASSWORD)
    client.on_connect = on_connect
    client.on_message = on_message

    client.connect(TTN_BROKER, port=1883, keepalive=60)

    try:
        client.loop_forever()
    except KeyboardInterrupt:
        print("\nStopping")
        client.disconnect()

