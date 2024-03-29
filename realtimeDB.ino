#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Firebase_ESP_Client.h>
#include "HX711.h"
#include <RTClib.h> // Include RTC library
 
// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"
 
// Insert your network credentials
#define WIFI_SSID "iPhone X"
#define WIFI_PASSWORD "bhuwadit"
 
// Insert Firebase project API Key
#define API_KEY "AIzaSyDsg-L9MUXikPcNtZ-h211Iei24W2_HBS0"
 
// Insert RTDB URL
#define DATABASE_URL "https://senior1-d5603-default-rtdb.asia-southeast1.firebasedatabase.app/"
 
const int LOADCELL_DOUT_PIN = 12;
const int LOADCELL_SCK_PIN = 13;
const float THRESHOLD_WEIGHT = 400000; // Set your threshold weight in the appropriate units
const float LAP_DISTANCE = 50.0;        // Distancer lap in meters
 
HX711 scale;
RTC_DS3231 rtc; // RTC module object
boolean countingTime = false;
unsigned long startTime = 0;
int lapCount = 0;
 
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
 
void connectToWiFi()
{
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
}
 
void setup()
{
  Serial.begin(115200);
  connectToWiFi();
 
  /* Initialize RTC module */
  if (!rtc.begin())
  {
    Serial.println("Couldn't find RTC");
    while (1)
      ;
  }
 
  /* Set RTC time (uncomment and replace with the correct date and time) */
  rtc.adjust(DateTime(__DATE__, __TIME__));
  Serial.println("Time Set");
 
  /* Or set RTC time manually (uncomment and set the correct date and time) */
  // rtc.adjust(DateTime(2024, 3, 15, 12, 0, 0));
 
  /* Assign the api key (required) */
  config.api_key = API_KEY;
 
  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;
 
  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", ""))
  {
    Serial.println("ok");
  }
  else
  {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }
 
  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h
 
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
 
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  Serial.println("Place a known weight on the scale to start/stop counting time.");
}
 
void sendDataToFirebase(int lap, unsigned long elapsedTime, DateTime dateTime)
{
  // Calculate speed in km/h
  float speed = (LAP_DISTANCE / 1000.0) / (elapsedTime / 3600000.0); // Convert milliseconds to hours
 
  if (Firebase.RTDB.setInt(&fbdo, "/laps/" + String(lap) + "/Lap", lap) &&
      Firebase.RTDB.setInt(&fbdo, "/laps/" + String(lap) + "/ElapsedTime", elapsedTime / 1000) &&
      Firebase.RTDB.setString(&fbdo, "/laps/" + String(lap) + "/DateTime", String(dateTime.year()) + "-" + String(dateTime.month()) + "-" + String(dateTime.day()) + " " + String(dateTime.hour()) + ":" + String(dateTime.minute()) + ":" + String(dateTime.second())) &&
      Firebase.RTDB.setFloat(&fbdo, "/laps/" + String(lap) + "/Speed", speed))
  {
    Serial.println("Data sent to Firebase successfully.");
  }
  else
  {
    Serial.println("Failed to send data to Firebase.");
    Serial.println(fbdo.errorReason());
  }
}
 
void loop()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    connectToWiFi();
  }
 
  if (scale.is_ready())
  {
    long reading = scale.get_units(10);
 
    if (reading  THRESHOLD_WEIGHT)
    {
      if (!countingTime)
      {
        countingTime = true;
        startTime = millis();
        lapCount++; // Increment lap count
        Serial.println("Weight reached. Time counting started.");
        Serial.println(scale.get_units(10));
      }
    }
    else
    {
      if (countingTime)
      {
        countingTime = false;
        unsigned long endTime = millis();
        unsigned long elapsedTime = endTime - startTime;

        Serial.println("Weight no longer reached. Time counting stopped.");
        Serial.print("Total time: ");
        Serial.print(elapsedTime / 1000); // Convert milliseconds to seconds
        Serial.println(" seconds");

        Serial.println("Weight no longer reached. Time counting stopped.");
      }
    }
 
    // Update elapsed time continuously
    if (countingTime)
    {
      unsigned long currentTime = millis();
      unsigned long elapsedTime = currentTime - startTime;
 
      Serial.print("Time Elapsed: ");
      Serial.print(elapsedTime / 1000); // Convert milliseconds to seconds
      Serial.println(" seconds");
 
      // Update elapsed time in Firebase
      Firebase.RTDB.setInt(&fbdo, "/CountelapsedTime", elapsedTime / 1000);
      DateTime now = rtc.now();
 
      sendDataToFirebase(lapCount, elapsedTime, now);
    }
  }
  else
  {
    Serial.println("HX711 not found.");
  }
 
  delay(1000); // Update elapsed time every second
}
 