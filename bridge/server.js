const dotenv = require("dotenv");
const mqtt = require("mqtt");
const WebSocket = require("ws");

dotenv.config();

// MQTT settings (use .env if available)
const MQTT_URL = process.env.MQTT_URL ;
const MQTT_TOPIC = process.env.MQTT_TOPIC ;
const WS_PORT = Number(process.env.WS_PORT);

// Connect to MQTT broker
const mqttClient = mqtt.connect(MQTT_URL);

mqttClient.on("connect", () => {
  console.log(`[mqtt] connected to ${MQTT_URL}`);
  mqttClient.subscribe(MQTT_TOPIC, (err) => {
    if (err) {
      console.error("[mqtt] subscribe error", err);
      return;
    }
    console.log(`[mqtt] subscribed to ${MQTT_TOPIC}`);
  });
});

mqttClient.on("error", (err) => {
  console.error("[mqtt] error", err);
});

// WebSocket server for the browser
const wss = new WebSocket.Server({ port: WS_PORT });
console.log(`[ws] listening on port ${WS_PORT}`);

function broadcast(obj) {
  const payload = JSON.stringify(obj);
  wss.clients.forEach((client) => {
    if (client.readyState === WebSocket.OPEN) {
      client.send(payload);
    }
  });
}

// MQTT -> WebSocket (telemetry)
mqttClient.on("message", (topic, msg) => {
  broadcast({ topic, payload: msg.toString() });
});

// WebSocket -> MQTT (commands)
wss.on("connection", (ws) => {
  console.log("[ws] client connected");

  ws.on("message", (raw) => {
    const text = typeof raw === "string" ? raw : raw.toString();
    let message;
    try {
      message = JSON.parse(text);
    } catch (err) {
      ws.send(JSON.stringify({ error: "invalid_json" }));
      return;
    }

    // Format A (legacy):
    // { "id": "station-01", "cmd": { "unit": "C" } }
    if (message && message.id && message.cmd) {
      const targetTopic = `station/${message.id}/cmd`;
      mqttClient.publish(targetTopic, JSON.stringify(message.cmd));
      ws.send(JSON.stringify({ ok: true, topic: targetTopic }));
      return;
    }

    // Format B (direct topic):
    // { "topic": "GP5-weather", "payload": { "unit": "C" } }
    if (message && message.topic && message.payload !== undefined) {
      const payload =
        typeof message.payload === "string"
          ? message.payload
          : JSON.stringify(message.payload);
      mqttClient.publish(message.topic, payload);
      ws.send(JSON.stringify({ ok: true, topic: message.topic }));
      return;
    }

    ws.send(JSON.stringify({ error: "missing_fields" }));
  });

  ws.on("close", () => {
    console.log("[ws] client disconnected");
  });
});
