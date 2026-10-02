#include <Arduino.h>
#include <MIDI.h>

// Pin mapping, taken from conductor_main.net (board-to-board bus + Pico
// GPIOs), not from the README, which doesn't spell out GPIO numbers:
//
//   GPIO4  MIDI_1 RX  - not a hardware UART RX pin -> SerialPIO
//   GPIO5  MIDI_2 RX  - UART1 RX                   -> Serial2
//   GPIO6  MIDI_3 RX  - not a hardware UART RX pin -> SerialPIO
//   GPIO16 MIDI_TX    - UART0 TX, broadcast to the player bus (J2)
//   GPIO18 RESET      - to the SN74HCT125 buffer, then out to the panel jack
//   GPIO19 RUN        - to the SN74HCT125 buffer, then out to the panel jack
//   GPIO20 CLOCK      - to the SN74HCT125 buffer, then out to the panel jack
constexpr uint8_t kPinMidi1Rx = 4;
constexpr uint8_t kPinMidi2Rx = 5;
constexpr uint8_t kPinMidi3Rx = 6;
constexpr uint8_t kPinMidiTx = 16;
constexpr uint8_t kPinReset = 18;
constexpr uint8_t kPinRun = 19;
constexpr uint8_t kPinClock = 20;

// The designated clock/start/stop jack, fixed at compile time (see the
// README's "Two boards" / "Open Questions" sections - the conductor holds
// no runtime config). Change this and reflash to move the role to a
// different physical jack.
constexpr uint8_t kClockSourceInput = 1; // MIDI_1

constexpr uint32_t kBusBaud = 250000; // 8x 31250, see README "Baud rate"
constexpr uint32_t kClockPulseUs = 1000; // CLOCK trigger width
constexpr uint32_t kResetPulseMs = 5;    // RESET trigger width

// Transport-message-to-jack mapping is an assumption, not spelled out in
// the module README - confirm/adjust if it doesn't match expectations:
//   Start    -> pulse RESET, then RUN high
//   Continue -> RUN high (no reset pulse)
//   Stop     -> RUN low
//   Clock    -> pulse CLOCK once per tick

SerialPIO midiSerial1(NOPIN, kPinMidi1Rx);
SerialPIO midiSerial3(NOPIN, kPinMidi3Rx);

MIDI_CREATE_INSTANCE(SerialPIO, midiSerial1, MIDI1);
MIDI_CREATE_INSTANCE(HardwareSerial, Serial2, MIDI2);
MIDI_CREATE_INSTANCE(SerialPIO, midiSerial3, MIDI3);

#define MidiTxPort Serial1

bool running = false;
uint32_t clockPulseUntilUs = 0;
uint32_t resetPulseUntilMs = 0;

uint32_t messagesIn[3] = {0, 0, 0};
uint32_t messagesForwarded = 0;

void pulseClock() {
  digitalWrite(kPinClock, HIGH);
  clockPulseUntilUs = micros() + kClockPulseUs;
}

void handleTransport(midi::MidiType type) {
  switch (type) {
    case midi::Clock:
      pulseClock();
      break;
    case midi::Start:
      digitalWrite(kPinReset, HIGH);
      resetPulseUntilMs = millis() + kResetPulseMs;
      digitalWrite(kPinRun, HIGH);
      running = true;
      break;
    case midi::Continue:
      digitalWrite(kPinRun, HIGH);
      running = true;
      break;
    case midi::Stop:
      digitalWrite(kPinRun, LOW);
      running = false;
      break;
    default:
      break;
  }
}

// Number of data bytes to forward for a channel voice message, 0 for
// anything we don't forward.
uint8_t dataByteCountFor(midi::MidiType type) {
  switch (type) {
    case midi::NoteOff:
    case midi::NoteOn:
    case midi::AfterTouchPoly:
    case midi::ControlChange:
    case midi::PitchBend:
      return 2;
    case midi::ProgramChange:
    case midi::AfterTouchChannel:
      return 1;
    default:
      return 0;
  }
}

// Re-inserts an explicit status byte and writes the whole message in one
// go, per the README's "Merging inputs: expand running status" rules -
// never propagate running status across the merge, never interleave two
// messages' bytes.
void forwardChannelMessage(midi::MidiType type, midi::Channel channel, byte data1, byte data2) {
  uint8_t dataBytes = dataByteCountFor(type);
  if (dataBytes == 0) return;

  byte statusByte = (byte)type | ((channel - 1) & 0x0F);
  MidiTxPort.write(statusByte);
  MidiTxPort.write(data1 & 0x7F);
  if (dataBytes == 2) MidiTxPort.write(data2 & 0x7F);

  messagesForwarded++;
}

template <typename MidiInterfaceType>
void pumpInput(MidiInterfaceType &midi, uint8_t inputNumber) {
  if (!midi.read()) return;

  messagesIn[inputNumber - 1]++;

  midi::MidiType type = midi.getType();

  switch (type) {
    case midi::Clock:
    case midi::Start:
    case midi::Continue:
    case midi::Stop:
      // Consumed here regardless of which jack it came from - only the
      // designated jack's transport messages are acted on, and none of
      // them are ever forwarded onto the player bus.
      if (inputNumber == kClockSourceInput) {
        handleTransport(type);
      }
      return;

    case midi::SystemExclusive:
      // Not forwarded - no player type needs it, and the fixed-length
      // framing below assumes ordinary channel voice messages.
      return;

    default:
      forwardChannelMessage(type, midi.getChannel(), midi.getData1(), midi.getData2());
      return;
  }
}

String replLine;

void printHelp() {
  Serial.println(F("commands: help, status"));
}

void printStatus() {
  Serial.print(F("clock source: MIDI_"));
  Serial.println(kClockSourceInput);
  Serial.print(F("running: "));
  Serial.println(running ? F("yes") : F("no"));
  Serial.print(F("MIDI_1 messages: "));
  Serial.println(messagesIn[0]);
  Serial.print(F("MIDI_2 messages: "));
  Serial.println(messagesIn[1]);
  Serial.print(F("MIDI_3 messages: "));
  Serial.println(messagesIn[2]);
  Serial.print(F("forwarded to bus: "));
  Serial.println(messagesForwarded);
}

void handleReplCommand(const String &line) {
  if (line == "help") {
    printHelp();
  } else if (line == "status") {
    printStatus();
  } else {
    Serial.println(F("unknown command, try 'help'"));
  }
}

void pollRepl() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      Serial.println();
      if (replLine.length() > 0) {
        handleReplCommand(replLine);
        replLine = "";
      }
    } else if (c == '\b' || c == 0x7F) { // backspace / DEL
      if (replLine.length() > 0) {
        replLine.remove(replLine.length() - 1);
        Serial.print(F("\b \b"));
      }
    } else if (replLine.length() < 63) {
      replLine += c;
      Serial.write(c); // echo - a raw USB-CDC link doesn't echo on its own
    }
  }
}

void setup() {
  pinMode(kPinClock, OUTPUT);
  pinMode(kPinRun, OUTPUT);
  pinMode(kPinReset, OUTPUT);
  digitalWrite(kPinClock, LOW);
  digitalWrite(kPinRun, LOW);
  digitalWrite(kPinReset, LOW);

  Serial.begin(115200); // USB CDC REPL; baud is a formality over USB

  MidiTxPort.setTX(kPinMidiTx);
  MidiTxPort.begin(kBusBaud);

  Serial2.setRX(kPinMidi2Rx);

  // MIDI library transports call begin() on their underlying Serial port
  // at the standard 31250 baud themselves.
  MIDI1.begin(MIDI_CHANNEL_OMNI);
  MIDI1.turnThruOff();
  MIDI2.begin(MIDI_CHANNEL_OMNI);
  MIDI2.turnThruOff();
  MIDI3.begin(MIDI_CHANNEL_OMNI);
  MIDI3.turnThruOff();
}

void loop() {
  pumpInput(MIDI1, 1);
  pumpInput(MIDI2, 2);
  pumpInput(MIDI3, 3);

  pollRepl();

  uint32_t nowUs = micros();
  if (clockPulseUntilUs != 0 && (int32_t)(nowUs - clockPulseUntilUs) >= 0) {
    digitalWrite(kPinClock, LOW);
    clockPulseUntilUs = 0;
  }

  uint32_t nowMs = millis();
  if (resetPulseUntilMs != 0 && nowMs >= resetPulseUntilMs) {
    digitalWrite(kPinReset, LOW);
    resetPulseUntilMs = 0;
  }
}
