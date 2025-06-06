#include <WiFi.h>
#include <WebServer.h>

// Wi-Fi AP details
const char* ssid = "ESP32_Battery";
const char* password = "12345678";

WebServer server(80);

// Battery config
const int sensorPin = 35;
const float dividerRatio = 6.1;
const float maxVolt = 4.2;
const float minVolt = 3.0;
const float maxDistance = 200.0;

const int numReadings = 10;
float voltageReadings[numReadings];
int readIndex = 0;
float totalVoltage = 0;
float averageVoltage = 0;

// Motor (L298N)
const int IN1 = 26;
const int IN2 = 27;
int currentSpeed = 170; // medium by default

void setup() {
  Serial.begin(115200);
  WiFi.softAP(ssid, password);
  Serial.println(WiFi.softAPIP());

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  stopMotor();

  for (int i = 0; i < numReadings; i++) {
    voltageReadings[i] = 0.0;
  }

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/motor", handleMotor);
  server.on("/speed", handleSpeed);
  server.begin();
}

void loop() {
  server.handleClient();
}

float getSmoothedVoltage() {
  int raw = analogRead(sensorPin);
  float voltage = (float)raw * 3.3 / 4095.0 * dividerRatio;
  totalVoltage -= voltageReadings[readIndex];
  voltageReadings[readIndex] = voltage;
  totalVoltage += voltage;
  readIndex = (readIndex + 1) % numReadings;
  averageVoltage = totalVoltage / numReadings;
  return averageVoltage;
}

void handleRoot() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>Battery Monitor + Motor</title>";
  html += "<style>";
  html += "body { font-family: -apple-system, sans-serif; text-align: center; padding: 20px; }";
  html += ".meter { width: 90%; height: 30px; background: #eee; border-radius: 20px; overflow: hidden; margin: 10px auto; }";
  html += ".fill { height: 100%; width: 0%; background: #4cd964; transition: width 0.3s; }";
  html += ".label { font-size: 18px; margin-top: 10px; }";
  html += ".btn { padding: 10px 20px; margin: 5px; font-size: 16px; border: none; border-radius: 8px; cursor: pointer; }";
  html += ".btn-black { background: black; color: white; }";
  html += ".btn-blue { background: #007aff; color: white; }";
  html += ".btn-red { background: #ff3b30; color: white; }";
  html += "</style></head><body>";

  html += "<h1>🔋 Battery Status</h1>";
  html += "<div class='meter'><div class='fill' id='batteryBar'></div></div>";
  html += "<div class='label' id='batteryLabel'>-- % | -- V</div>";

  html += "<h2>📍 Distance Remaining</h2>";
  html += "<div class='meter'><div class='fill' id='distanceBar' style='background:#007aff;'></div></div>";
  html += "<div class='label' id='distanceLabel'>-- km</div>";

  html += "<h2>🎚️ Speed</h2>";
  html += "<button class='btn btn-black' onclick='setSpeed(85)'>1</button>";
  html += "<button class='btn btn-black' onclick='setSpeed(170)'>2</button>";
  html += "<button class='btn btn-black' onclick='setSpeed(255)'>3</button>";

  html += "<h2>🔁 Motor</h2>";
  html += "<button class='btn btn-blue' onclick=\"sendMotor('forward')\">Forward</button>";
  html += "<button class='btn btn-blue' onclick=\"sendMotor('reverse')\">Reverse</button>";
  html += "<button class='btn btn-red' onclick=\"sendMotor('stop')\">Stop</button>";

  html += "<script>";
  html += "const maxDistance = " + String(maxDistance) + ";";
  html += "function fetchData() {";
  html += "fetch('/data').then(r => r.json()).then(d => {";
  html += "let volt = d.voltage;";
  html += "let charging = volt > " + String(maxVolt) + ";";
  html += "if (charging) {";
  html += "document.getElementById('batteryBar').style.width = '100%';";
  html += "document.getElementById('batteryBar').style.background = '#007aff';";
  html += "document.getElementById('batteryLabel').innerText = 'Charging... ⚡';";
  html += "} else {";
  html += "let percent = d.percent;";
  html += "let dist = d.distance;";
  html += "document.getElementById('batteryBar').style.width = percent + '%';";
  html += "document.getElementById('batteryBar').style.background = percent > 20 ? '#4cd964' : '#ff3b30';";
  html += "document.getElementById('batteryLabel').innerText = percent.toFixed(0) + '% | ' + volt.toFixed(2) + ' V';";
  html += "document.getElementById('distanceBar').style.width = (dist / maxDistance * 100) + '%';";
  html += "document.getElementById('distanceLabel').innerText = dist.toFixed(2) + ' km';";
  html += "}});";
  html += "}";
  html += "function sendMotor(action) { fetch('/motor?dir=' + action); }";
  html += "function setSpeed(val) { fetch('/speed?val=' + val); }";
  html += "setInterval(fetchData, 1000); fetchData();";
  html += "</script></body></html>";

  server.send(200, "text/html", html);
}

void handleData() {
  float voltage = getSmoothedVoltage();
  float percent = map(voltage * 100, minVolt * 100, maxVolt * 100, 0, 100);
  percent = constrain(percent, 0, 100);
  float distanceLeft = (percent / 100.0) * maxDistance;

  String json = "{";
  json += "\"percent\":" + String(percent, 1) + ",";
  json += "\"voltage\":" + String(voltage, 2) + ",";
  json += "\"distance\":" + String(distanceLeft, 2);
  json += "}";

  server.send(200, "application/json", json);
}

void handleMotor() {
  if (!server.hasArg("dir")) {
    server.send(400, "text/plain", "Missing dir");
    return;
  }
  String dir = server.arg("dir");
  if (dir == "forward") {
    analogWrite(IN1, currentSpeed);
    analogWrite(IN2, 0);
  } else if (dir == "reverse") {
    analogWrite(IN1, 0);
    analogWrite(IN2, currentSpeed);
  } else {
    stopMotor();
  }
  server.send(200, "text/plain", "OK");
}

void handleSpeed() {
  if (server.hasArg("val")) {
    currentSpeed = constrain(server.arg("val").toInt(), 0, 255);
    server.send(200, "text/plain", "Speed set");
  } else {
    server.send(400, "text/plain", "Missing val");
  }
}

void stopMotor() {
  analogWrite(IN1, 0);
  analogWrite(IN2, 0);
}