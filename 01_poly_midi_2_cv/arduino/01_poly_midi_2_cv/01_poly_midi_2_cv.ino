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

#define DAC1  9 
#define DAC2  8

uint8_t gates[4] = {GATE1, GATE2, GATE3, GATE4};

MIDI_CREATE_DEFAULT_INSTANCE();

void setup()
{
 pinMode(gates[0], OUTPUT);
 pinMode(gates[1], OUTPUT);
 pinMode(gates[2], OUTPUT);
 pinMode(gates[3], OUTPUT);
 pinMode(DAC1, OUTPUT);
 pinMode(DAC2, OUTPUT);
 
 digitalWrite(gates[0],LOW);
 digitalWrite(gates[1],LOW);
 digitalWrite(gates[2],LOW);
 digitalWrite(gates[3],LOW);
 digitalWrite(DAC1,HIGH);
 digitalWrite(DAC2,HIGH);

 SPI.begin();

 MIDI.begin(MIDI_CHANNEL_OMNI);

}

bool registers[4] = {0,0,0,0};
uint8_t active = 0;
uint8_t noteMsg; 
uint8_t type;

void loop()
{
  if (MIDI.read()) {                    
    type = MIDI.getType();
    switch (type) {
      
      case midi::NoteOn: 
        noteMsg = MIDI.getData1() - 21; // A0 = 21, Top Note = 108
        if ((noteMsg < 0) || (noteMsg > 87)) break; // Only 88 notes of keyboard are supported
        noteOff(noteMsg);
        break;
        
      case midi::NoteOff:
        noteMsg = MIDI.getData1() - 21;
        if ((noteMsg < 0) || (noteMsg > 87)) break; 
        noteOn(noteMsg);
        break;
        
      default:
        break;
    }
  }
}

void noteOn(uint8_t note)
{
  for (uint8_t i=0; i<4; i++){
    if (registers[(active+i)% 4] == 0){ 
      registers[(active+i)% 4] = true;
      commandNote(note, (active+i)%4);
      return;
    } 
  }
  commandNote(note, active);
  active++;
}

void noteOff(uint8_t note)
{
  for (uint8_t i=0; i<4; i++) {
    if (registers[i] == note) {
      registers[i] = false;
      digitalWrite(gates[i], LOW);
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

void commandNote(uint8_t noteMsg, uint8_t gate) {
  digitalWrite(gates[gate],HIGH);
  unsigned int mV = (unsigned int) ((float) noteMsg * NOTE_SF + 0.5); 
  // register 0 = dac 0 channel 0
  // register 1 = dac 0 channel 1
  // register 2 = dac 1 channel 0
  // register 3 = dac 1 channel 1
  uint8_t pin = gate & 0x0010 ? DAC1 : DAC2;
  bool chn = gate & 0x0001;
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
