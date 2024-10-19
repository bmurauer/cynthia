
#include <SPI.h>

#define NP_SEL1 A0  // Note priority is set by pins A0 and A2
#define NP_SEL2 A2  
#define DAC1  8 
#define NOTE_SF 47.069f

#define ACD_PRECISION 1024
#define MIN_NOTE 21 // A0 = 21 Top Note = 108
#define MAX_NOTE 108

#define BASE_CLOCK 7
#define BASE_LATCH 9
#define BASE_DATA 10

#define SCALE_CLOCK 4
#define SCALE_LATCH 5
#define SCALE_DATA 6

int value = 0;

// 0V =    0 ->  21
// 5V = 1024 -> 108    <-- this logic can be changed when using a deparate adc with more bits
float ACD_FACTOR = (108.0 - 21) / 1024.0;

// 0  Bb    1  Dd    Eb       Gb    Ab    Bb    2  Dd    Eb       Gb    Ab    Bb    3  Dd    Eb       Gb    Ab    Bb    4  Dd    Eb       Gb    Ab    Bb    
// AA    BB CC    DD    EE FF    GG    AA    BB CC    DD    EE FF    GG    AA    BB CC    DD    EE FF    GG    AA    BB CC    DD    EE FF    GG    AA    BB
// 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 48 49 50 51 52 53 54 55 56 57 58 59 60 61 62 63 64 65 66 67 68 69 70 71 72

/*  7-segment digits:
 *     A 
 *   F   B
 *     G 
 *   E   C
 *     D   H(.)
 */

// A b C d E F g 
// dot = minor
int bases_leds[] = {
   0b10011100, // C    0
   0b10011101, // C#   1
   0b01111010, // D    2
   0b01111011, // D#   3
   0b10011110, // E    4
   0b10001110, // F    5
   0b10001111, // F#   6
   0b10111110, // G    7
   0b10111111, // G#   8
   0b11101110, // A    9
   0b11101111, // A#  10
   0b00111110  // B   11
};

String base_names[12] = {
  "C ",
  "C#",
  "D ",
  "D#",
  "E ",
  "F ",
  "F#",
  "G ",
  "G#",
  "A ",
  "A#",
  "B "
};

int scales[] = {
  0b101011010101, // major
  0b101101011010, // minor
  0b100101110010, // blues
  0b111111111111, // chromatic
};

String scale_names[] = {
  "major",
  "minor",
  "blues",
  "chromatic",
};


int get_scale() {
  // TODO: implement
  return 0;
}

int get_base() {
  // TODO: implement
  return 0;
}

float get_input() {
  int input = analogRead(A3);
  return 21.0 + ((float)input) * ACD_FACTOR;
}

bool is_in_scale(int note, int scale) {
  int rel_note = note%12; // offset octaves
  // in the example, rel_note should be 7
  return scales[scale] & (0b100000000000 >> rel_note);
}

int get_right_note(int start, int scale) {
  for(int i=start; i<=MAX_NOTE; i++){
    if (is_in_scale(i, scale)) {
      Serial.print("R: ");
      Serial.print(i);
      Serial.print(" (");
      Serial.print(base_names[i%12]);
      Serial.print(")  ");
      return i;
    }
  }
  return MAX_NOTE;
}

int get_left_note(int start, int scale) {
  for(int i=start; i>=MIN_NOTE; i--){
     if (is_in_scale(i, scale)) {
      Serial.print("L: ");
      Serial.print(i);
      Serial.print(" (");
      Serial.print(base_names[i%12]);
      Serial.print(")  ");
      return i;
    }
  }
  return MIN_NOTE;
}

int get_nearest_note(float note, int base, int scale) {
  // note can be anything, e.g., 54.3
  int first_right = get_right_note(ceil(note), scale);
  int first_left = get_left_note(floor(note), scale);
  Serial.print("[");
  Serial.print(first_left);
  Serial.print(",");
  Serial.print(first_right);
  Serial.print("]  ");
  if ( abs(note - first_right) < abs(note - first_left)) {
    return first_right;
  } else {
    return first_left;
  }
}

void set_base_leds(int base) {
  digitalWrite(BASE_LATCH, LOW);
  shiftOut(BASE_DATA, BASE_CLOCK, LSBFIRST, bases_leds[base]);
  digitalWrite(BASE_LATCH, HIGH);
}

void set_scale_leds(int scale) {
  digitalWrite(SCALE_LATCH, LOW);
  shiftOut(SCALE_DATA, SCALE_CLOCK, LSBFIRST, scales[scale]);
  digitalWrite(SCALE_LATCH, HIGH);
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(DAC1, OUTPUT);
  pinMode(BASE_CLOCK, OUTPUT);
  pinMode(BASE_LATCH, OUTPUT);
  pinMode(BASE_DATA, OUTPUT);
  pinMode(SCALE_CLOCK, OUTPUT);
  pinMode(SCALE_LATCH, OUTPUT);
  pinMode(SCALE_DATA, OUTPUT);
  digitalWrite(DAC1,HIGH);
  SPI.begin();

}

void loop() {
  int scale = get_scale();
  // set_scale_leds(scale);
  int base = get_base();
  set_base_leds(base);
  float input = get_input();
  int note = get_nearest_note(input, base, scale);
  Serial.print("Nearest note for base=");
  Serial.print(base_names[base]);
  Serial.print(", scale=");
  Serial.print(scale_names[scale]);
  Serial.print(", input=");
  Serial.print(input);
  Serial.print(":   ");
  Serial.print(note);
  Serial.print(" (");
  Serial.print(base_names[note%12]);
  Serial.println(")");
  delay(1000);
}

void commandNote(int noteMsg) {
 
  unsigned int mV = (unsigned int) ((float) noteMsg * NOTE_SF + 0.5); 
  setVoltage(0, 1, mV);  // DAC1, channel 0, gain = 2X
}

void setVoltage(bool channel, bool gain, unsigned int mV)
{
  unsigned int command = channel ? 0x9000 : 0x1000;

  command |= gain ? 0x0000 : 0x2000;
  command |= (mV & 0x0FFF);
  
  SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
  digitalWrite(DAC1,LOW);
  SPI.transfer(command>>8);
  SPI.transfer(command&0xFF);
  digitalWrite(DAC1,HIGH);
  SPI.endTransaction();
}
