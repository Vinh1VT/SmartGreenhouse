import os
import requests
import json
import paho.mqtt.client as mqtt


#Ouvertures des variables d'environnement


#TTN
TTN_BROKER = os.getenv("TTN_BROKER")
TTN_USER = os.getenv("TTN_USER")
TTN_PASSWORD = os.getenv("TTN_PASSWORD")
TTN_TOPIC = "v3/+/devices/+/up"

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
    try :
        payload = json.loads(msg.payload.decode("utf-8"))

        device_id = payload.get("end_device_ids",{}).get("device_id","unknown_device")
        uplink_message = payload.get("uplink_message",{})
        decoded_payload = uplink_message.get("decoded_payload")

        if not decoded_payload:
            print(f"No decoded payload, message ignored")
            return

        fields = []

        anomaly = decoded_payload.get("sensor_anomaly",False)
        fields.append(f"sensor_anomaly={str(anomaly)}")

        for key,value in decoded_payload.items():
            if key == "sensor_anomaly":
                continue

            if isinstance(value,dict):
                for position, sensor_val in value.items():
                    fields.append(f"{key}_{position}={sensor_val}")
            else:
                fields.append(f"{key}={value}")


        if fields:
            fields_str = ",".join(fields)

            line_protocol_data = f"environnement,device={device_id} {fields_str}"
            print("Sending message to DB")
            send_to_influxdb(line_protocol_data)



    except json.JSONDecodeError:
        print("Error : JSON is invalid")
    except Exception as e :
        print(f"Error : {e}")





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
            print(f"Failed to push data {response.status_code} : {response.text}")

    except requests.exceptions.RequestException as e :
        print(f"Exception : {e}")


if __name__ == "__main__":
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION1)
    client.username_pw_set(username=TTN_USER, password=TTN_PASSWORD)
    client.on_connect = on_connect
    client.on_message = on_message

    client.connect(TTN_BROKER, port=1883, keepalive=60)

    try:
        client.loop_forever()
    except KeyboardInterrupt:
        print("\nStopping")
        client.disconnect()

