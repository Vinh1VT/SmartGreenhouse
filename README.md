# Mr Gardener - Serre Connectée

Ce dépôt contient les sources utilisée dans le projet Mr. Gardener, réalisé en 2026 pour la formation Electronique-Informatique d'année 4 à Polytech Sorbonne.

Ce document n'est qu'une courte description du projet, une documentation plus détaillé peut être trouvée [ici](https://github.com/Vinh1VT/SmartGreenhouse/blob/main/Mr.Gardener%20-%20SmartGreenHouse%20-%20User%20Guide.pdf)

##  Objectifs du projet

L'objectif principal est de concevoir un système autonome de mesures et de commandes pour la serre de l'école, sur le campus St Cyr l'école.
Le systême à donc comme caractéristique : 

- **Mesures Environnementale** : Acquisition de données environnementales (Humidité, température, luminosité...) pour la serre et son puits canadien
- **Basse Consommation** : Le projet fonctionne quasi entièrement sur batterie, et grâce à un rechargement par panneau solaire est en permanence autonome
- **IA Embarquée** : Proof of concept d'une détection locale de panne du ventilateur de la serre, via une IA de classification Edge Impulse
- **Collecte et visualisation des données** : Collecte, traitement et visualisation des données, via The Things Network, InfluxDB et Grafana
- **Contrôle du ventilateur** : Prototype d'une contrôleur I2C pour le ventilateur du puits canadien de la serre. Va être remplacé par un système plus robuste

##  Structure du dépôt

Les dossiers de ce dépôt contiennent les élément suivants :

*  **`Iot_PCB/`** : Projets KiCad (schémas et routages) pour la carte mère et les contrôleurs de ventilateurs
*  **`code_esp32_WROOM_32E/`** : Firmware principal (ESP32) gérant la lecture des capteurs, la logique de commande et la connectivité LoRa
*  **`nano_AI/`** : Modèles d'intelligence artificielle et code d'inférence (Edge Impulse) tournant sur l'Arduino Nano
*  **`Server/`** : Dashboard Grafana, Script Python de liaison TTN<->InfluxDB et Parser TTN

## Auteurs : 
- Vinh VO TUAN
- Fallou CORDELETTE
- Quentin LE VOURC'H
- Yanis RAHOUI
- Taha OUERFILI
