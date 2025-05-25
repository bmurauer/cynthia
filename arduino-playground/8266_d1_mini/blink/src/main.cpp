#include <Arduino.h>
#define LED_BUILTINN D4
       
void setup() {
    pinMode(LED_BUILTINN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
}

// the loop function runs over and over again forever
void loop() {
    digitalWrite(LED_BUILTINN, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
    delay(500);                      // Wait for a second
    digitalWrite(LED_BUILTINN, HIGH);  // Turn the LED off by making the voltage HIGH
    delay(500);                      // Wait for two seconds (to demonstrate the active low LED)
  }
