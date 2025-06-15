#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>

// ======= WiFi Credentials =======
const char *ssid = "ESP_FAN";
const char *password = "123456789";
const char* http_username = "admin";
const char* http_password = "admin123";

// ======= Pins =======
const int RELAY1_PIN = 5;         // Fan relay
const int DHT_PIN = 14;           // DHT22 data pin

#define DHTTYPE DHT22
DHT dht(DHT_PIN, DHTTYPE);

// ======= Globals =======
float tempThreshold = 30.0;       // Default fan-on threshold (°C)
bool manualMode = false;          // false = Auto, true = Manual
WebServer server(80);

// ======= Helper Functions =======
void setupRelays() {
  pinMode(RELAY1_PIN, OUTPUT);
  digitalWrite(RELAY1_PIN, HIGH); // OFF by default (assuming LOW = ON)
}

void handleRoot() {
  if (!server.authenticate(http_username, http_password)) return server.requestAuthentication();

  String modeStr = manualMode ? "Manual" : "Auto";
  String otherModeStr = manualMode ? "auto" : "manual";

  String html = "<!DOCTYPE html><html><head><title>Fan Control</title><meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>body{font-family:sans-serif;text-align:center;background:#eef;}button,input{padding:15px 30px;margin:10px;font-size:20px;}</style></head><body>";
  html += "<h2>ESP32 Fan Control</h2>";
  html += "<p><b>Mode:</b> " + modeStr + "</p>";
  html += "<form action='/setMode'><input type='hidden' name='mode' value='" + otherModeStr + "'><input type='submit' value='Switch to " + otherModeStr + "'></form><br>";
  html += "<button onclick=\"location.href='/toggle?relay=1'\">Toggle Fan</button><br><br>";
  html += "<a href='/config'>Go to Config</a>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleToggle() {
  if (!server.authenticate(http_username, http_password)) return server.requestAuthentication();

  if (!server.hasArg("relay")) {
    server.send(400, "text/plain", "Missing relay arg");
    return;
  }

  manualMode = true;

  int relay = server.arg("relay").toInt();
  if (relay == 1) digitalWrite(RELAY1_PIN, !digitalRead(RELAY1_PIN));

  server.sendHeader("Location", "/");
  server.send(303);
}

void handleConfig() {
  if (!server.authenticate(http_username, http_password)) return server.requestAuthentication();

  String html = "<!DOCTYPE html><html><head><title>Config</title></head><body>";
  html += "<h2>Set Temperature Threshold</h2>";
  html += "<form action='/setThreshold'>";
  html += "<input type='number' step='0.1' name='value' value='" + String(tempThreshold) + "'>";
  html += "<input type='submit' value='Update'>";
  html += "</form><p>Current Threshold: " + String(tempThreshold) + " °C</p>";
  html += "<a href='/'>Back</a>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleSetThreshold() {
  if (!server.authenticate(http_username, http_password)) return server.requestAuthentication();

  if (server.hasArg("value")) tempThreshold = server.arg("value").toFloat();
  server.sendHeader("Location", "/config");
  server.send(303);
}

void handleSetMode() {
  if (!server.authenticate(http_username, http_password)) return server.requestAuthentication();

  if (server.hasArg("mode")) {
    String mode = server.arg("mode");
    manualMode = (mode == "manual");
    Serial.println("Mode changed to: " + mode);
  }
  server.sendHeader("Location", "/");
  server.send(303);
}

// ======= Setup & Loop =======
void setup() {
  Serial.begin(115200);
  dht.begin();

  WiFi.softAP(ssid, password);
  Serial.println("WiFi AP started. Connect to: ESP_FAN");

  setupRelays();

  server.on("/", handleRoot);
  server.on("/toggle", handleToggle);
  server.on("/config", handleConfig);
  server.on("/setThreshold", handleSetThreshold);
  server.on("/setMode", handleSetMode);

  server.begin();
  Serial.println("Web server started.");
}

void loop() {
  server.handleClient();

  if (!manualMode) {
    float temp = dht.readTemperature();
    if (isnan(temp)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }

    Serial.print("Temperature: ");
    Serial.print(temp);
    Serial.println(" °C");

    if (temp > tempThreshold) {
      digitalWrite(RELAY1_PIN, LOW); // Fan ON
      Serial.println("🔥 Too hot → Fan ON");
    } else {
      digitalWrite(RELAY1_PIN, HIGH); // Fan OFF
      Serial.println("😌 Cool enough → Fan OFF");
    }
  } else {
    Serial.println("Manual Mode Active — Automation Paused");
  }

  delay(3000);
}
