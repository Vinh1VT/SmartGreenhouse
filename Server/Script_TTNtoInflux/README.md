# Guide d'utilisation de la version de test locale

## Installation de influxdb
Il faut installer influxdb (moi je l'ai fait par docker)
Dans influxDB, tout configurer et récupérer sa clé API (/!\ LA GARDER)

## Mise en place de l'environnement
- Créer un .env à partir du .env.example, dans le même dossier
  - Les données sont soit celles rentrées sur influxDB, soit trouvables sur TTN
  - Le certificat SSL doit être "False"
  - L'url de influx db est "http://localhost:8086/api/v2/write" en local.
- Lancer le script python (en ayant installé les libs dans requirements.txt)
- Entrer des données dans TTN
- Normalement elles devraient rentrer dans influxDB

## Lier Grafana à InfluxDB
Il faut lancer grafana (pareil je l'ai fait via docker), puis créer une nouvelle datasource.
Sélectionner : 
- Langage FLUX
- DESACTIVER BASIC AUTH
- Rentrer l'organisation et le toke


Et normalement magie ça marche, y'a plus qu'a envoyer des données à TTN et ça fonctionne :DD

##  Structure de la Base de Données (InfluxDB)



Toutes les données issues des capteurs sont stockées dans la base InfluxDB. L'architecture respecte le modèle standard de séries temporelles (Time-Series) avec des *Measurements*, des *Tags* et des *Fields*.

* **Bucket (Base de données) :** Choisit dans le .env
* **Measurement (Table) :** `environnement`

### 1. Modèle de données

| Concept InfluxDB | Notre Utilisation | Exemple de valeur |
| :--- | :--- | :--- |
| **Tag** (Index) | Identifie le capteur physique exact. | `device = interieur-puits` |
| **Fields Numériques** | Les grandeurs physiques mesurées à l'instant T. | `temperature_ambiant = 22.5`<br>`humidite_est_10cm = 55.0`<br>`co2 = 400` |
| **Fields Booléens** | Statut du capteur. | `sensor_anomaly = false` |
| **Fields États** | Historique de fonctionnement. | `etat_ventilateur = 1` |

---

###  2. Spécificités importantes pour Grafana

Le programme est bien conçu pour recevoir toutes les 30 mins mais il y a des cas particuliers : 

#### A. Le champ `etat_ventilateur` (Rétro-horodatage)
Le microcontrôleur enregistre l'état du ventilateur (1 = ON, 0 = OFF) par **tranches de 5 minutes**. 
Lorsque le backend Python reçoit la trame de 30 minutes, il effectue un **rétro-horodatage** : il écrit la température à l'heure de réception, mais il insère les 6 états du ventilateur dans le passé (ex: T-30min, T-25min, T-20min...).

#### B. Le champ `sensor_anomaly` (Booléen)
C'est un flag de sécurité généré par le parseur si la trame reçue est corrompue ou incomplète.
