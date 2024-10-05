
#include <SPI.h>

#define NP_SEL1 A0  // Note priority is set by pins A0 and A2
#define NP_SEL2 A2  
#define DAC1  8 
#define NOTE_SF 47.069f

#define ACD_PRECISION 1024
#define MIN_NOTE 21 // A0 = 21, Top Note = 108
#define MAX_NOTE 108

int value = 0;

// 0V =    0 ->  21
// 5V = 1024 -> 108    <-- this logic can be changed when using a deparate adc with more bits
float ACD_FACTOR = 108.0 / 1024.0;

// 0  Bb    1  Dd    Eb       Gb    Ab    Bb    2  Dd    Eb       Gb    Ab    Bb    3  Dd    Eb       Gb    Ab    Bb    4  Dd    Eb       Gb    Ab    Bb    
// AA    BB CC    DD    EE FF    GG    AA    BB CC    DD    EE FF    GG    AA    BB CC    DD    EE FF    GG    AA    BB CC    DD    EE FF    GG    AA    BB
// 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 48 49 50 51 52 53 54 56 57 58 59 60 61 62 63 64 65 66 67 68 69 70 71 72

//
// A Bb  B  C Dd  D Eb  E  F Gb  G Ab  A
// 0  1  2  3  4  5  6  7  8  9 10 11  ...

// int8_t[] SCALE_MAJOR = {0, 2, 4, 5, 7, 9, 11};
int8_t SCALE_MINOR[7] = {0, 2, 3, 5, 7, 9, 11};

int getOverlapNote(int n0, int n1) {
  for (int octave=0; octave<6; octave++) {
    for (int note_no=0; note_no<7; note_no++) {
      int note = octave * 12 + SCALE_MINOR[note_no];
      if (n0 <= note && note < n1) {
        return note; 
      }
    }
  }
  return -1;
}

// now, we have to search for the nearest note that is in scale.
// start searching at the input voltage and go in both directions in half-note steps.


void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(DAC1, OUTPUT);
  digitalWrite(DAC1,HIGH);
  
  SPI.begin();

}

void loop() {
  // put your main code here, to run repeatedly:
  int input = analogRead(A3);
 
  // lets say, input is 2.5V = 512
  // first, map this input to the corresponding note.
  float note = ((float)input) * ACD_FACTOR;
  
  
  for (int i=0; i<MAX_NOTE; i++) {
    int candidate = getOverlapNote(note, note + i);
    if (candidate > 0) {
      commandNote(candidate);
      break;
    } else {
      candidate = getOverlapNote(note-i, note);
      if (candidate > 0) {
        commandNote(candidate);
        break;
      }
    }
  }

}

void commandNote(int noteMsg) {
  
  unsigned int mV = (unsigned int) ((float) noteMsg * NOTE_SF + 0.5); 
  setVoltage(DAC1, 0, 1, mV);  // DAC1, channel 0, gain = 2X
}

void setVoltage(int dacpin, bool channel, bool gain, unsigned int mV)
{
  unsigned int command = channel ? 0x9000 : 0x1000;

  command |= gain ? 0x0000 : 0x2000;
  command |= (mV & 0x0FFF);
  
  SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
  digitalWrite(dacpin,LOW);
  SPI.transfer(command>>8);
  SPI.transfer(command&0xFF);
  digitalWrite(dacpin,HIGH);
  SPI.endTransaction();
}
