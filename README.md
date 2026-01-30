# Station Météo Connectée

Projet de station météo IoT permettant la mesure en temps réel de la température et de l'humidité avec basculement d'unité (Celsius/Fahrenheit) via bouton physique et interface Web.

## Fonctionnalités
- **Mesure précise** : Température et humidité via capteur DHT22.
- **Interaction Hybride** : Switch d'unité via bouton physique (Interrupt) et bouton virtuel (MQTT).
- **Feedback Visuel** : LEDs dédiées pour indiquer l'unité active (°C ou °F).
- **Connectivité** : Publication des données en JSON sur broker MQTT et pilotage à distance.

---

## Architecture du Projet

Le projet est divisé en trois parties principales :
1. **ESP32** : Collecte les données et gère le hardware.
2. **Bridge (Node.js)** : Assure la liaison et le traitement des données MQTT.
3. **Frontend** : Interface utilisateur pour visualiser les données et piloter l'unité.

---

## Structure du Dépôt

```text
|
│── esp32/          # Code Arduino (.ino)
├── bridge/         # Script de liaison MQTT
├── frontend/       # Interface Web (HTML/JS)
├── docs/           # Ressources
└── README.md
```

## Installation et Lancement

1. Configuration du .ENV

```env
MQTT_HOST=captain.dev0.pandor.cloud
MQTT_PORT=1884
MQTT_URL=mqtt://captain.dev0.pandor.cloud:1884
MQTT_TOPIC=GP5-weather/weather
MQTT_TOPIC_CONFIG=GP5-weather/config
WS_PORT=8080
```

2. Configuration Hardware (ESP32)

   Capteur : DHT22 sur Pin 16.
   Bouton : Pin 14 (Input Pullup).
   LEDs : LED_C sur Pin 19, LED_F sur Pin 21.
   Logiciel : Ouvrir esp32/esp32.ino dans Arduino IDE.
   Bibliothèques requises : 
   - **DHT sensor library** : Gère la communication spécifique avec le capteur DHT11/DHT22 pour extraire les données brutes de température et d'humidité.
   - **Adafruit Unified Sensor** : Bibliothèque de base requise par Adafruit pour assurer une compatibilité standardisée entre les différents types de capteurs physiques.
   - **PubSubClient** : Permet à l'ESP32 de devenir un client MQTT pour envoyer des messages vers le broker (Publish) et en recevoir (Subscribe).
   - **ArduinoJson** : Indispensable pour construire les chaînes JSON envoyées au dashboard et pour décoder les commandes JSON reçues du bouton virtuel.

3. Broker MQTT
Le projet utilise le broker suivant :
   Host : captain.dev0.pandor.cloud
   Port : 1884
   Topics :
      Publication : `GP5-weather/weather`
      Souscription : `GP5-weather/config`

4. Bridge
   Naviguer dans le dossier bridge/
   Lancer la commande : `npm install`
   Lancer la commande : `node server.js`


5. Frontend
   Naviguer dans le dossier front/
   Lancer la commande : `npm install`
   Lancer la commande : `npm run dev`

## Format des Messages (Payloads)

Données envoyées par l'ESP32 :
```JSON
{
  "humidity": 43.7,
  "temp": 22.4,
  "unit": "C"
}
```
Commande reçue par l'ESP32 :
```JSON
{
  "unitToggle": "F"
}
```
## Membres de l'équipe (GP5)

- **Amaury Sanchez** - Frontend
- **Christopher De Pasqual** - Bridge
- **Stéphane Descarpentries** - ESP-32 / Montage Hardware
- **Adrien Allard** - ESP-32 / Montage Hardware

### Roussources

- [Montage Wokwi](docs/montage_wokwi.png)
- [ESP32](docs/esp32.jpg)
- [Board](docs/board.jpg)
- [Montage Hardware](docs/montage_hardware.jpg)
- [Hardware](docs/hardware.jpg)
