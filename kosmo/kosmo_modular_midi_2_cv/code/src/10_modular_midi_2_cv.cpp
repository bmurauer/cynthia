#include <Arduino.h>
#include <MIDI.h>
#include <SPI.h>
#include <ESP8266WiFi.h>

// ESP8266-D1 pins:
// TX   TXD   
// RX   RXD   
// A0   Analog input, max 3.3V input   
// D0   IO   
// D1   IO, SCL   
// D2   IO, SDA   
// D3   IO, 10k Pull-up   
// D4   IO, 10k Pull-up, BUILTIN_LED   
// D5   IO, SCK   
// D6   IO, MISO   
// D7   IO, MOSI   
// D8   IO, 10k Pull-down, SS   
// G    Ground   
// 5V   5V   
// 3V3  3.3V   
// RST  Reset   

// The MIDI interface uses the RX pin.

#define GATE D0
#define CLOCK D1
#define DAC D2
#define CHANNEL 1
#define LED D4

// Rescale 88 notes to 4096 mV:
//    noteMsg = 0 -> 0 mV 
//    noteMsg = 87 -> 4096 mV
// DAC output will be (4095/87) = 47.069 mV per note, and 564.9655 mV per octive
// Note that DAC output will need to be amplified by 1.77X for the standard 1V/octave 
#define NOTE_SF 47.069f // This value can be tuned if CV output isn't exactly 1V/octave                      

MIDI_CREATE_DEFAULT_INSTANCE();

void setup() {
  // we don't want any WIFI - that only draws current.
  WiFi.mode(WIFI_OFF);

  pinMode(GATE, OUTPUT);
  pinMode(CLOCK, OUTPUT);
  pinMode(DAC, OUTPUT);
  pinMode(LED, OUTPUT);

  digitalWrite(GATE, LOW);
  digitalWrite(CLOCK, LOW);
  digitalWrite(DAC, HIGH);

  SPI.begin(); // uses pins 5, 6, and 7 on an ESP8266-D1

  MIDI.begin(MIDI_CHANNEL_OMNI); // uses RX pin
  digitalWrite(LED, HIGH); 
}

// setVoltage -- Set DAC voltage output
// channel: 0 (A, note) or 1 (B, velocity) 
// gain: 0 = 1X, 1 = 2X.  
// mV: integer 0 to 4095. If gain is 1X, mV is in units of half mV (i.e., 0 to 2048 mV).
// If gain is 2X, mV is in units of mV

void setVoltage(bool channel, unsigned int mV)
{
  unsigned int command = channel ? 0x9000 : 0x1000;

  command |= (mV & 0x0FFF);
  
  SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
  digitalWrite(DAC, LOW);
  SPI.transfer(command >> 8);
  SPI.transfer(command & 0xFF);
  digitalWrite(DAC, HIGH);
  SPI.endTransaction();
}

void loop()
{
  static unsigned long clock_timer, ledTime = 0;
  unsigned int noteMsg;

  if ((ledTime > 0) && (millis() - ledTime > 100)) {
    digitalWrite(LED, HIGH);
    ledTime = 0;
  }

  if ((clock_timer > 0) && (millis() - clock_timer > 20)) { 
    digitalWrite(CLOCK, LOW); // Set clock pulse low after 20 msec 
    clock_timer = 0;  
  }
  
  if (MIDI.read()) {                    
    digitalWrite(LED, LOW);
    ledTime = millis();
    byte type = MIDI.getType();
    switch (type) {
      case midi::NoteOn: 
        noteMsg = MIDI.getData1() - 21; // A0 = 21, Top Note = 108
        
        if ((noteMsg < 0) || (noteMsg > 87)) break; // Only 88 notes of keyboard are supported

        // velocity range from 0 to 4095 mV  Left shift d2 by 5 to scale from 0 to 4095, 
        setVoltage(1, MIDI.getData2() << 5);  
 
        setVoltage(0, (unsigned int) ((float) noteMsg * NOTE_SF + 0.5));
        digitalWrite(GATE, HIGH);
        break;
        
      case midi::NoteOff:
        digitalWrite(GATE, LOW);
        break;
        
      case midi::Clock:
          digitalWrite(CLOCK,HIGH); // Start clock pulse
          clock_timer=millis();    
        break;
        
      case midi::ActiveSensing: 
        break;
        
      default:
        break;
    }
  }
  
}



