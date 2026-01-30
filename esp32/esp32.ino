#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <ArduinoJson.h>

// --- CONFIGURATION ---
const char* ssid = "Woodis";
const char* password = "stephsteph";
const char* mqtt_server = "captain.dev0.pandor.cloud";
const int mqtt_port = 1884;

#define PIN_BUTTON 14
#define PIN_LED_C  19
#define PIN_LED_F  21
#define PIN_DHT    16
#define DHTTYPE    DHT22

// --- VARIABLES ---
DHT dht(PIN_DHT, DHTTYPE);
WiFiClient espClient;
PubSubClient client(espClient);

bool simulationMode = false;
volatile bool buttonPressed = false;
bool unitIsF = false; 
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 200;
unsigned long lastMsgTime = 0;

// --- FONCTIONS MATÉRIEL ---
void updateHardware() {
  if (unitIsF) {
    digitalWrite(PIN_LED_C, LOW);
    digitalWrite(PIN_LED_F, HIGH);
  } else {
    digitalWrite(PIN_LED_C, HIGH);
    digitalWrite(PIN_LED_F, LOW);
  }
}

void IRAM_ATTR handleButton() {
  buttonPressed = true;
}

// --- MQTT CALLBACK ---
void callback(char* topic, byte* payload, unsigned int length) {
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, payload, length);
  if (error) return;

  if (doc.containsKey("unitToggle")) {
    const char* val = doc["unitToggle"];
    if (strcmp(val, "F") == 0) unitIsF = true;
    else if (strcmp(val, "C") == 0) unitIsF = false;
    updateHardware();
  }
}

// --- WIFI (VERSION ROBUSTE) ---
void setup_wifi() {
  delay(100);
  Serial.print("\n[WIFI] Tentative de connexion a : ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA); // Force le mode station
  WiFi.begin(ssid, password);

  int counter = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    counter++;
    
    if (counter > 40) { // Si après 20 sec ça échoue, on redémarre le module
      Serial.println("\n[ERROR] Echec WiFi. Redémarrage...");
      ESP.restart();
    }
  }
  Serial.println("\n[SUCCESS] WiFi connecté !");
  Serial.print("[IP] ");
  Serial.println(WiFi.localIP());
}

// --- MQTT RECONNECT ---
void reconnect() {
  while (!client.connected()) {
    Serial.print("[MQTT] Connexion...");
    String clientId = "ESP32-HETIC-" + String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("OK");
      client.subscribe("GP5-weather/config");
    } else {
      Serial.print("Echec, rc=");
      Serial.println(client.state());
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  pinMode(PIN_LED_C, OUTPUT);
  pinMode(PIN_LED_F, OUTPUT);

  updateHardware(); 
  attachInterrupt(PIN_BUTTON, handleButton, FALLING);

  if (!simulationMode) dht.begin();
  
  setup_wifi(); // On connecte d'abord le WiFi
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    setup_wifi(); // Sécurité : reconnexion WiFi si perdu
  }
  
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Bouton physique
  if (buttonPressed) {
    unsigned long now = millis();
    if (now - lastDebounceTime > debounceDelay) {
      unitIsF = !unitIsF;
      updateHardware();
      lastDebounceTime = now;
    }
    buttonPressed = false;
  }

  // Envoi des données
  unsigned long now = millis();
  if (now - lastMsgTime > 2000) {
    lastMsgTime = now;
    
    float h = simulationMode ? 45.0 : dht.readHumidity();
    float t = simulationMode ? 22.0 : dht.readTemperature(unitIsF);

    if (isnan(h) || isnan(t)) return;

    StaticJsonDocument<200> outDoc;
    outDoc["humidity"] = h;
    outDoc["temp"] = round(t * 10) / 10.0;
    outDoc["unit"] = unitIsF ? "F" : "C";

    char buffer[256];
    serializeJson(outDoc, buffer);
    client.publish("GP5-weather/weather", buffer);
    Serial.print("[PUBLISH] ");
    Serial.println(buffer);
  }
}