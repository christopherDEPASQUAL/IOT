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

// Gestion bouton / unité
volatile bool buttonPressed = false;
bool unitIsF = false;   // false = °C, true = °F
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 200;

unsigned long lastMsgTime = 0;

// --- WIFI ---
void setup_wifi() {
  delay(100);
  Serial.println("\n[DEBUG] Connexion WiFi...");
  WiFi.begin(ssid, password);

  int attempt = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    attempt++;
    if (attempt > 30) {
      Serial.println("\n[ERROR] WiFi bloqué, nouvelle tentative...");
      WiFi.begin(ssid, password);
      attempt = 0;
    }
  }

  Serial.println("\n[SUCCESS] WiFi connecté");
  Serial.print("[INFO] IP : ");
  Serial.println(WiFi.localIP());
}

// --- MQTT ---
void reconnect() {
  while (!client.connected()) {
    Serial.print("[DEBUG] Connexion MQTT... ");

    String clientId = "ESP32-HETIC-";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println("OK");
    } else {
      Serial.print("FAIL (rc=");
      Serial.print(client.state());
      Serial.println(")");
      delay(5000);
    }
  }
}

// --- ISR BOUTON ---
void IRAM_ATTR handleButton() {
  buttonPressed = true;
}

// --- SETUP ---
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n==============================");
  Serial.println(" STATION METEO HETIC - ESP32 ");
  Serial.println("==============================");

  pinMode(PIN_BUTTON, INPUT_PULLUP);
  pinMode(PIN_LED_C, OUTPUT);
  pinMode(PIN_LED_F, OUTPUT);

  digitalWrite(PIN_LED_C, HIGH); // °C par défaut
  digitalWrite(PIN_LED_F, LOW);

  attachInterrupt(PIN_BUTTON, handleButton, FALLING);
  Serial.println("[DEBUG] Bouton OK");

  if (!simulationMode) {
    dht.begin();
    Serial.println("[DEBUG] DHT22 initialisé");
  } else {
    Serial.println("[DEBUG] Mode simulation");
  }

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
}

// --- LOOP ---
void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Gestion bouton (hors ISR)
  if (buttonPressed) {
    unsigned long now = millis();
    if (now - lastDebounceTime > debounceDelay) {
      unitIsF = !unitIsF;
      Serial.print("[BUTTON] Changement unité : ");
      Serial.println(unitIsF ? "°F" : "°C");
      lastDebounceTime = now;
    }
    buttonPressed = false;
  }

  unsigned long now = millis();
  if (now - lastMsgTime > 2000) {
    lastMsgTime = now;

    float h, t;
    if (simulationMode) {
      t = random(200, 350) / 10.0;
      h = random(300, 700) / 10.0;
    } else {
      h = dht.readHumidity();
      t = dht.readTemperature();
    }

    if (isnan(h) || isnan(t)) {
      Serial.println("[ERROR] Lecture DHT échouée");
      return;
    }

    float displayTemp = t;
    String unit = "C";

    if (unitIsF) {
      displayTemp = (t * 1.8) + 32;
      unit = "F";
      digitalWrite(PIN_LED_C, LOW);
      digitalWrite(PIN_LED_F, HIGH);
    } else {
      digitalWrite(PIN_LED_C, HIGH);
      digitalWrite(PIN_LED_F, LOW);
    }

    // JSON
    StaticJsonDocument<200> doc;
    doc["humidity"] = h;
    doc["temp"] = round(displayTemp * 10) / 10.0;
    doc["unit"] = unit;

    char buffer[256];
    serializeJson(doc, buffer);

    Serial.print("[PUBLISH] ");
    Serial.println(buffer);

    if (client.publish("GP5-weather", buffer)) {
      Serial.println("[OK] MQTT envoyé");
    } else {
      Serial.println("[ERROR] MQTT échec");
    }
  }
}
