#include <Arduino.h>
#include "HX711.h"
 
 
const int LOADCELL_DOUT_PIN = 12;
const int LOADCELL_SCK_PIN = 13;

 
HX711 scale;

void setup() {
  Serial.begin(115200);
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  

}
 
void loop() {
  if (scale.is_ready()) {
    long reading = scale.get_units(10);
 
    
        Serial.println(scale.get_units(10));
      } 
 
      // Add a small delay to avoid multiple state changes due to bouncing
      delay(1000);
    }