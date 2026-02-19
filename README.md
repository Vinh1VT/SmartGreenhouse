# Projet IoT Serre Connectée


## Lancer le serveur : 

Il faut avoir installé docker et docker compose, se placer dans le dossier Server/, puis suivre ces étapes : 

1. Ajouter un fichier .env dans Server/ (modèle dans .env.example)
2. Lancer et build l'image via la commande :
``` docker compose up -d --build```

3. Ajouter manuellement dans node red les informations du serveur TTN ( (TTN_APP_ID) et (TTN_API_KEY) dans le .env) 




Apres c'est la suite que j'ai pas encore implémenté, mais ça devrait être la config de influxDB et Grafana


## Eteindre le serveur : 

Toujours dans le dossier Server/, on peut éteindre le serveur via la commande : 
``` docker compose down```

Auquel on peut ajouter l'option -v, pour aussi effacer les données.
