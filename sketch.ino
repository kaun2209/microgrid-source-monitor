// Low-voltage model of a home power-source priority system
// Priority: GRID > GENERATOR > BATTERY
// Publishes source changes + periodic heartbeat over MQTT (retained)

#include <WiFi.h>
#include <PubSubClient.h>

// ---- Pins ----
const int GRID_PIN = 32;
const int GEN_PIN  = 33;
const int LED_GRID = 25;
const int LED_GEN  = 26;
const int LED_BATT = 27;

// ---- WiFi (Wokwi's simulated network, no password) ----
const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASS = "";

// ---- MQTT (public test broker: anyone can read it, send nothing private) ----
const char* MQTT_HOST = "broker.hivemq.com";
const int   MQTT_PORT = 1883;
// Must match TOPIC in dashboard.html exactly
const char* TOPIC = "kkm-microgrid-demo-7431/status";

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

// ---- State ----
String lastSource = "";
String candidate = "";
String stableSource = "";
unsigned long candidateSince = 0;
unsigned long lastPublish = 0;

const unsigned long HEARTBEAT_MS = 15000;  // republish state every 15 s
const unsigned long DEBOUNCE_MS  = 200;    // ignore switch bounce shorter than this

String getActiveSource() {
  bool gridOn = (digitalRead(GRID_PIN) == LOW);
  bool genOn  = (digitalRead(GEN_PIN)  == LOW);
  if (gridOn) return "GRID";
  if (genOn)  return "GENERATOR";
  return "BATTERY";
}

void connectWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASS, 6);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print(".");
  }
  Serial.println(" connected");
}

void connectMQTT() {
  while (!mqtt.connected()) {
    Serial.print("Connecting to MQTT...");
    String clientId = "esp32-kkm-" + String(random(0xffff), HEX);
    if (mqtt.connect(clientId.c_str())) {
      Serial.println(" connected");
    } else {
      Serial.print(" failed, rc=");
      Serial.println(mqtt.state());
      delay(2000);
    }
  }
}

void publishSource(const String& source) {
  String payload = "{\"source\":\"" + source + "\",\"uptime_s\":" + String(millis() / 1000) + "}";
  mqtt.publish(TOPIC, payload.c_str(), true);   // true = retained message
  Serial.println("Published: " + payload);
}

void setup() {
  Serial.begin(115200);
  pinMode(GRID_PIN, INPUT_PULLUP);
  pinMode(GEN_PIN, INPUT_PULLUP);
  pinMode(LED_GRID, OUTPUT);
  pinMode(LED_GEN, OUTPUT);
  pinMode(LED_BATT, OUTPUT);

  connectWiFi();
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  connectMQTT();
  Serial.println("Power source monitor started");
}

void loop() {
  if (!mqtt.connected()) {
    connectMQTT();
    lastSource = "";            // forces a republish after reconnect
  }
  mqtt.loop();

  // Debounce: accept a new reading only after it stays stable
  String reading = getActiveSource();
  if (reading != candidate) {
    candidate = reading;
    candidateSince = millis();
  }
  if (millis() - candidateSince >= DEBOUNCE_MS) stableSource = candidate;
  if (stableSource == "") { delay(10); return; }

  digitalWrite(LED_GRID, stableSource == "GRID");
  digitalWrite(LED_GEN,  stableSource == "GENERATOR");
  digitalWrite(LED_BATT, stableSource == "BATTERY");

  bool changed   = (stableSource != lastSource);
  bool heartbeat = (millis() - lastPublish >= HEARTBEAT_MS);
  if (changed || heartbeat) {
    if (changed) Serial.println("Active source: " + stableSource);
    publishSource(stableSource);
    lastSource = stableSource;
    lastPublish = millis();
  }
  delay(20);
}
