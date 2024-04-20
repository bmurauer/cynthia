/* 
 *    POLY MIDI 2 CV 
 *    Copyright (C) 2024 Benjamin Murauer
 *    
 *    Based on
 *    MID2CV by Larry McGovern 2017
 *  
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License <http://www.gnu.org/licenses/> for more details.
 */
 
#include <MIDI.h>
#include <SPI.h>

#define GATE1 5
#define GATE2 4
#define GATE3 3
#define GATE4 2


#define SWITCH1 A0
#define SWITCH2 A1
#define SWITCH3 A2
#define SWITCH4 A3
#define LED LED_BUILTIN
#define DAC1  9 
#define DAC2  8  


uint8_t switches[4] = {SWITCH1, SWITCH2, SWITCH3, SWITCH4};
uint8_t gates[4] = {GATE1, GATE2, GATE3, GATE4};

MIDI_CREATE_DEFAULT_INSTANCE();

void setup()
{
 pinMode(gates[0], OUTPUT);
 pinMode(gates[1], OUTPUT);
 pinMode(gates[2], OUTPUT);
 pinMode(gates[3], OUTPUT);

 pinMode(switches[0], INPUT_PULLUP);
 pinMode(switches[1], INPUT_PULLUP);
 pinMode(switches[2], INPUT_PULLUP);
 pinMode(switches[3], INPUT_PULLUP);

 pinMode(LED, OUTPUT);
 
 pinMode(DAC1, OUTPUT);
 pinMode(DAC2, OUTPUT);
 
 digitalWrite(gates[0],LOW);
 digitalWrite(gates[1],LOW);
 digitalWrite(gates[2],LOW);
 digitalWrite(gates[3],LOW);
 digitalWrite(DAC1,HIGH);
 digitalWrite(DAC2,HIGH);

 
 //Serial.begin(9600);
 SPI.begin();
 MIDI.begin(MIDI_CHANNEL_OMNI);

}

uint8_t registers[4] = {0, 0, 0, 0};
uint8_t active = 3;
uint8_t noteMsg; 
uint8_t type;

void loop()
{
  /**/
  if (MIDI.read()) {                    
    type = MIDI.getType();
    if (type == midi::NoteOn || type == midi::NoteOff) {
      noteMsg = MIDI.getData1() - 21; // A0 = 21, Top Note = 108
      if (type == midi::NoteOn && MIDI.getData2() > 0) { // velocity
        noteOn(noteMsg);
      } else {
        noteOff(noteMsg);
      }
    }
  }
  /**/
  /**
  for (int i=0; i<10; i++) {
    noteOn(40);
    delay(1000);
    noteOff(40);
    delay(1000);
  }
  /**/
  /**
  for (int i=0; i<3; i++) {
    noteOn(40);
    delay(500);
    noteOn(41);
    delay(500);
    noteOn(42);
    delay(500);
    noteOn(43);
    delay(500);
    noteOn(44);
    delay(500);

    delay(1000);

    noteOff(40);
    delay(500);
    noteOff(41);
    delay(500);
    noteOff(42);
    delay(500);
    noteOff(43);
    delay(500);
    noteOff(44);
    delay(500);
    
    delay(1000);
  }
  /**/
}

bool isEnabled(uint8_t idx) {
  /**
  if (idx == 1) return twoEnabled;
  return true;
  /**/
  return !digitalRead(switches[idx]);
  // return true;
}

void noteOn(uint8_t note)
{
  // first, check if there are registers that are free and not switched off.
  // as long as there are free registers, we don't have to advance the last register index.
  for (uint8_t i=0; i<4; i++){
    
    // check if switch is on "ON" position
    if (!isEnabled(i)) {
      continue;
    }
    
    // check if register is free
    if (registers[i] == 0){ 
      registers[i] = note;
      commandNote(note, i);
      // Serial.println("playing  " + String(note) + " on free " + String(i));
      return;
    } 
  }

  // if we have reached this point, all registers are switched off.
  // we better wait for some time for someone to switch one on again.
  delay(100);
}

void noteOff(uint8_t note)
{
  for (uint8_t i=0; i<4; i++) {
    if (registers[i] == note) {
      registers[i] = 0;
      digitalWrite(gates[i], LOW);
      // no break, no switch-checks - note off should put all notes off.
    }
  }
}


// Rescale 88 notes to 4096 mV:
//    noteMsg = 0 -> 0 mV 
//    noteMsg = 87 -> 4096 mV
// DAC output will be (4095/87) = 47.069 mV per note, and 564.9655 mV per octive
// Note that DAC output will need to be amplified by 1.77X for the standard 1V/octave
// This is done using a trimmer pot on the circuit board. 
#define NOTE_SF 47.069f 
  
void commandNote(uint8_t note, uint8_t gate) {
  digitalWrite(gates[gate],HIGH);
  unsigned int mV = (unsigned int) ((float) note * NOTE_SF + 0.5); 
  // register 0 = dac 0 channel 0
  // register 1 = dac 0 channel 1
  // register 2 = dac 1 channel 0
  // register 3 = dac 1 channel 1
  uint8_t pin = gate < 2 ? DAC1 : DAC2;
  bool chn = gate & 0x0001;
  //Serial.println("writing " + note + " (=" + mV + "mV) to gate "+gate+ " (=pin " + pin + "), channel " + chn);
  setVoltage(pin, chn, mV); 
}

// setVoltage -- Set DAC voltage output
// dacpin: chip select pin for DAC. 
// channel: 0 (A) or 1 (B).  Note and pitch bend on 0, velocity and CC on 2.
// mV: integer 0 to 4095.
void setVoltage(uint8_t dacpin, bool channel, unsigned int mV) {
  unsigned int command = channel ? 0x9000 : 0x1000;
  command |= (mV & 0x0FFF);
  
  SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
  digitalWrite(dacpin,LOW);
  SPI.transfer(command >> 8);
  SPI.transfer(command & 0xFF);
  digitalWrite(dacpin,HIGH);
  SPI.endTransaction();
}
