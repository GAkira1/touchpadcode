#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "HX711.h"

const char *ssid = "Tigerr";
const char *password = "88888888";

const int LOADCELL_DOUT_PIN = 12;
const int LOADCELL_SCK_PIN = 13;
const float THRESHOLD_WEIGHT = 650000;

HX711 scale;
boolean countingTime = false;
unsigned long startTime = 0;
unsigned long totalElapsedTime = 0;

ESP8266WebServer server(80);

void handleRoot() {
  String html = "<html><body>";
  html += "<h1 id='elapsedTime'>Total Elapsed Time: " + String(totalElapsedTime / 1000) + " seconds</h1>";
  html += "<p id='status'>Waiting for weight...</p>";
  html += "<script>";
  html += "function updateTime() {";
  html += "  var elapsedElement = document.getElementById('elapsedTime');";
  html += "  var statusElement = document.getElementById('status');";
  html += "  var xhttp = new XMLHttpRequest();";
  html += "  xhttp.onreadystatechange = function() {";
  html += "    if (this.readyState == 4 && this.status == 200) {";
  html += "      var response = JSON.parse(this.responseText);";
  html += "      elapsedElement.innerHTML = 'Total Elapsed Time: ' + response.time + ' seconds';";
  html += "      statusElement.innerHTML = response.status;";
  html += "    }";
  html += "  };";
  html += "  xhttp.open('GET', '/elapsed', true);";
  html += "  xhttp.send();";
  html += "  setTimeout(updateTime, 1000);";
  html += "}";
  html += "updateTime();";
  html += "</script>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleElapsed() {
  String status;
  if (countingTime) {
    unsigned long currentTime = millis();
    unsigned long elapsedTime = currentTime - startTime;
    totalElapsedTime += elapsedTime;
    startTime = currentTime;
    status = "Counting...";
  } else {
    status = "Waiting for weight...";
  }

  String jsonResponse = "{\"time\":" + String(totalElapsedTime / 1000) + ", \"status\":\"" + status + "\"}";
  server.send(200, "application/json", jsonResponse);
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
  server.on("/elapsed", HTTP_GET, handleElapsed);

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();

  if (scale.is_ready()) {
    long reading = scale.get_units(10);

    // Toggle counting state when the threshold is crossed.
    if (reading > THRESHOLD_WEIGHT) {
      if (!countingTime) {
        // Start counting
        countingTime = true;
        startTime = millis();
        Serial.println("Weight detected. Time counting started.");
      } else {
        // Stop and reset counting
        countingTime = false;
        totalElapsedTime = 0;
        Serial.println("Weight detected again. Time counting stopped and reset.");
      }
      // Debounce
      while (scale.get_units(10) < THRESHOLD_WEIGHT) {
        delay(10);
      }
    }
  } else {
    Serial.println("HX711 not found.");
  }

  delay(100);
}
