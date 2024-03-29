#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "HX711.h"

const char* ssid = "Tigerr";
const char* password = "88888888";

const int LOADCELL_DOUT_PIN = 12;
const int LOADCELL_SCK_PIN = 13;
const float THRESHOLD_WEIGHT = 8000000; // Adjust this value based on your scale readings

HX711 scale;
boolean countingTime = false;
unsigned long startTime = 0;
unsigned long totalElapsedTime = 0;
boolean weightDetectedPreviously = false;

struct TimingSession {
  unsigned long startTime;
  unsigned long endTime;
};

const int MAX_SESSIONS = 10;
TimingSession sessions[MAX_SESSIONS];
int sessionCount = 0;

ESP8266WebServer server(80);

void handleRoot() {
  String html = "<html><head><meta http-equiv='refresh' content='1'>";
  html += "<style>";
  html += "body { background-color: #f0f0f0; font-family: Arial, sans-serif; }";
  html += "h1 { color: #333; }";
  html += "table { width: 100%; border-collapse: collapse; }";
  html += "th, td { padding: 8px; text-align: left; border-bottom: 1px solid #ddd; }";
  html += "tr:hover { background-color: #d1e0e0; }";
  html += "th { background-color: #4CAF50; color: white; }";
  html += "</style></head><body>";
  html += "<h1 id='elapsedTime'>Total Elapsed Time: " + String(totalElapsedTime / 1000) + " seconds</h1>";
  html += "<p id='status'>Waiting for weight...</p>";

  if (sessionCount > 0) {
    html += "<table>";
    html += "<tr><th>Lap</th><th>Elapsed Time (seconds)</th></tr>";
    for (int i = 0; i < sessionCount; i++) {
      html += "<tr><td>" + String(i + 1) + "</td>";
      html += "<td>" + String((sessions[i].endTime - sessions[i].startTime) / 1000.0, 2) + "</td></tr>";
    }
    html += "</table>";
  }

  html += "</body></html>";
  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  server.on("/", HTTP_GET, handleRoot);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();

  static bool weightWasOn = false;
  static unsigned long debounceTimer = 0;
  const unsigned long debounceDelay = 1000; // Wait for 1 second to stabilize the reading

  if (scale.is_ready()) {
    long reading = scale.get_units(10);
    Serial.print("Current reading: ");
    Serial.println(reading);

    // Check if someone has just stepped on the scale
    if (reading > WEIGHT_ON_THRESHOLD && !weightWasOn && millis() - debounceTimer > debounceDelay) {
      if (!countingTime) {
        countingTime = true;
        startTime = millis();
        Serial.println("Weight detected. Time counting started.");
        debounceTimer = millis(); // Reset debounce timer
      } else {
        // If we were already counting, this means they stepped off and on again
        unsigned long currentTime = millis();
        totalElapsedTime = currentTime - startTime;
        Serial.println("Weight detected again. Time counting stopped.");
        Serial.print("Total Elapsed Time: ");
        Serial.print(totalElapsedTime / 1000.0);
        Serial.println(" seconds.");
        if (sessionCount < MAX_SESSIONS) {
          sessions[sessionCount].startTime = startTime;
          sessions[sessionCount].endTime = currentTime;
          sessionCount++;
        }
        countingTime = false;
        totalElapsedTime = 0; // Reset total elapsed time for the next measurement
        debounceTimer = millis(); // Reset debounce timer
      }
      weightWasOn = true;
    }
    // Check if someone has just stepped off the scale
    else if (reading < WEIGHT_OFF_THRESHOLD && weightWasOn && millis() - debounceTimer > debounceDelay) {
      weightWasOn = false;
      debounceTimer = millis(); // Reset debounce timer
    }
  } else {
    Serial.println("HX711 not found.");
  }

  delay(100);
}

