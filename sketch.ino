// Low-voltage model of a home power-source priority system
// Priority: GRID > GENERATOR > BATTERY
// Publishes every source change over MQTT

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
// CHANGE "7431" to your own random number so your topic is unique
const char* TOPIC = "kkm-microgrid-demo-7431/status";

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

String lastSource = "";

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
  mqtt.publish(TOPIC, payload.c_str());
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
  if (!mqtt.connected()) connectMQTT();
  mqtt.loop();

  String source = getActiveSource();

  digitalWrite(LED_GRID, source == "GRID");
  digitalWrite(LED_GEN,  source == "GENERATOR");
  digitalWrite(LED_BATT, source == "BATTERY");

  if (source != lastSource) {
    Serial.println("Active source: " + source);
    publishSource(source);
    lastSource = source;
  }

  delay(50);
}
