#include <Arduino.h>
#include <MIDI.h>
#include <SPI.h>

// Pin mapping, matching player_drum_velocity.net:
//
//   PB7  (pin 1)   FAST_MIDI   - bus RX, USART1
//   PA0  (pin 7)   learn       - SW1 to GND, internal pull-up
//   PA1  (pin 8)   LED         - D1 anode, cathode via R9 to GND (active high)
//   PA2  (pin 9)   gate 1      - R1 -> Q1 base -> GATE 1 (J3)
//   PA3  (pin 10)  gate 2      - R3 -> Q2 base -> GATE 2 (J4)
//   PA4  (pin 11)  DAC ~CS     - MCP4822 pin 2
//   PA5  (pin 12)  DAC SCK     - MCP4822 pin 3, SPI1
//   PA7  (pin 14)  DAC SDI     - MCP4822 pin 4, SPI1
//
// DAC A -> U1A buffer -> VELOCITY 1 (J5), DAC B -> U1B buffer -> VELOCITY 2 (J6).
const uint32_t kPinLearn = PA0;
const uint32_t kPinLed = PA1;
const uint32_t kPinGate1 = PA2;
const uint32_t kPinGate2 = PA3;
const uint32_t kPinDacCs = PA4;

// Q1/Q2 invert: a high gate pin pulls its jack low. On the breadboard the
// LEDs sit directly on the gate pins instead, so they want the opposite. Set
// per environment in platformio.ini.
#ifndef GATE_INVERTED
#error "GATE_INVERTED must be set by the build environment (see platformio.ini)"
#endif
constexpr bool kGateInverted = GATE_INVERTED;

// Player bus: USART1 RX on PB7 (TSSOP-20 pin 1, FAST_MIDI on the players).
// Uart is the STM32duino 4.x name for the concrete HardwareSerial class.
// The core refuses to open a UART without a TX pin, so USART1's PB6 is
// claimed as well. It sits on pin 20, which is unconnected on both player
// boards, so nothing is ever driven by it.
Uart busSerial(PB7, PB6);

// The bus carries plain MIDI bytes, just 8x faster than a MIDI cable - see
// kBusBaud in the conductor firmware and the README's "Baud rate".
struct BusSerialSettings : public midi::DefaultSerialSettings {
  static const long BaudRate = 250000;
};
using BusTransport = midi::SerialMIDI<Uart, BusSerialSettings>;
BusTransport busTransport(busSerial);
midi::MidiInterface<BusTransport> busMidi(busTransport);

// MCP4822 command words: bit 15 selects channel A/B, bit 13 (~GA) set means
// 1x gain, bit 12 (~SHDN) set means output enabled. At 1x gain one LSB is
// 0.5mV (0-2.0475V), the only usable range on a 3.3V supply - 2x gain would
// ask for 4.096V and clip at the rail.
constexpr uint16_t kDacChannelA = 0x3000; // velocity 1, into U1A (buffer)
constexpr uint16_t kDacChannelB = 0xB000; // velocity 2, into U1B (buffer)

// Drum gates don't simply follow the note like the melodic player's:
// - Pads and sequencers often send the note-off within a millisecond or two,
//   too short for some envelopes to register, so a gate is held open for at
//   least kMinGateMs even if the note-off arrives earlier.
// - A note-on while the gate is still open (a fast roll) would otherwise
//   leave it high and the connected module would not retrigger, so the gate
//   is dropped for kRetriggerGapUs first.
constexpr uint32_t kMinGateMs = 10;
constexpr uint32_t kRetriggerGapUs = 1000;

// Learned settings live in the last flash page, which platformio.ini keeps
// out of the linker's flash region. One 64-bit word is programmed per save;
// an erased page reads as all 0xFF, which fails the magic check.
//
// The magic differs from the melodic player's ("CYN1"), so a chip that is
// reflashed from one player type to the other rejects the old settings
// instead of misreading them. Bump the digit if the layout ever changes.
constexpr uint32_t kSettingsPage = 15;
constexpr uint32_t kSettingsAddress = FLASH_BASE + kSettingsPage * FLASH_PAGE_SIZE;
constexpr uint32_t kSettingsMagic = 0x43594431; // "CYD1"

struct Settings {
  uint32_t magic;
  uint8_t channel; // 1-16, shared by both outputs
  uint8_t notes[2]; // output 1, output 2
  uint8_t reserved;
};
static_assert(sizeof(Settings) == 8, "Settings must fit one flash doubleword");

// Until something is learned, listen on all channels for the General MIDI
// kick and snare, so a fresh module does something useful straight away.
constexpr uint8_t kDefaultNotes[2] = {36, 38};

constexpr uint32_t kDebounceMs = 20;

// LED patterns while learning, repeating every kLedPatternMs: one blink while
// waiting for the first note, two ("blink-blink, pause") for the second. After
// saving, the LED stays on for kLedSavedMs as confirmation.
constexpr uint32_t kLedPatternMs = 800;
constexpr uint32_t kLedBlinkMs = 120;
constexpr uint32_t kLedSavedMs = 500;

struct Output {
  uint32_t pinGate;
  uint16_t dacChannel;
  uint8_t note;
  bool held;              // between note-on and note-off
  bool open;              // gate currently high (logically)
  bool reopenPending;     // in the retrigger gap, reopen at reopenAtUs
  uint32_t reopenAtUs;
  uint32_t openedMs;      // for the minimum gate length
};

Output outputs[2] = {
    {kPinGate1, kDacChannelA, kDefaultNotes[0], false, false, false, 0, 0},
    {kPinGate2, kDacChannelB, kDefaultNotes[1], false, false, false, 0, 0},
};

// Learning goes Idle -> WaitingFirst -> WaitingSecond -> saved, Idle. Nothing
// is saved until both notes are in; pressing the button in either waiting
// state cancels and keeps the old settings entirely.
enum class LearnState { Idle, WaitingFirst, WaitingSecond };
LearnState learnState = LearnState::Idle;
uint8_t learnChannel = 0;
uint8_t learnFirstNote = 0;
uint32_t learnStartedMs = 0;
uint32_t ledSavedUntilMs = 0; // 0 when not showing the save confirmation

bool buttonRaw = false;
bool buttonStable = false;
uint32_t buttonChangedMs = 0;

void writeDac(uint16_t channel, uint16_t code) {
  uint16_t command = channel | (code & 0x0FFF);
  SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
  digitalWrite(kPinDacCs, LOW);
  SPI.transfer(command >> 8);
  SPI.transfer(command & 0xFF);
  digitalWrite(kPinDacCs, HIGH);
  SPI.endTransaction();
}

void writeGatePin(const Output &output, bool open) {
  digitalWrite(output.pinGate, open != kGateInverted ? HIGH : LOW);
}

void openGate(Output &output) {
  output.open = true;
  output.reopenPending = false;
  output.openedMs = millis();
  writeGatePin(output, true);
}

void closeGate(Output &output) {
  output.open = false;
  output.reopenPending = false;
  writeGatePin(output, false);
}

void resetOutputs() {
  for (Output &output : outputs) {
    output.held = false;
    closeGate(output);
  }
}

// Returns false (leaving *settings untouched) if nothing valid is stored.
bool loadSettings(Settings *settings) {
  const Settings *saved = reinterpret_cast<const Settings *>(kSettingsAddress);
  if (saved->magic != kSettingsMagic) return false;
  if (saved->channel < 1 || saved->channel > 16) return false;
  if (saved->notes[0] > 127 || saved->notes[1] > 127) return false;
  *settings = *saved;
  return true;
}

// Erases the settings page and programs the new settings. The CPU stalls for
// the ~20-40ms page erase, so bus bytes arriving meanwhile are lost - fine,
// since this only happens once per learn.
bool saveSettings(const Settings &settings) {
  Settings current;
  if (loadSettings(&current) && memcmp(&current, &settings, sizeof(Settings)) == 0) {
    return true; // spare the flash a needless erase
  }

  uint64_t doubleword;
  memcpy(&doubleword, &settings, sizeof(doubleword));

  FLASH_EraseInitTypeDef erase = {};
  erase.TypeErase = FLASH_TYPEERASE_PAGES;
  erase.Page = kSettingsPage;
  erase.NbPages = 1;
  uint32_t failedPage;

  HAL_FLASH_Unlock();
  FLASH->SR = FLASH_SR_ERRORS; // write-1-to-clear any stale error flags
  bool ok = HAL_FLASHEx_Erase(&erase, &failedPage) == HAL_OK &&
            HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, kSettingsAddress, doubleword) == HAL_OK;
  HAL_FLASH_Lock();
  return ok;
}

// Loads the saved channel and notes (or the defaults) into the outputs and
// the MIDI input filter.
void applySavedSettings() {
  Settings settings;
  if (loadSettings(&settings)) {
    outputs[0].note = settings.notes[0];
    outputs[1].note = settings.notes[1];
    busMidi.setInputChannel(settings.channel);
  } else {
    outputs[0].note = kDefaultNotes[0];
    outputs[1].note = kDefaultNotes[1];
    busMidi.setInputChannel(MIDI_CHANNEL_OMNI);
  }
}

// True once per debounced press (not on release).
bool learnButtonPressed() {
  bool down = digitalRead(kPinLearn) == LOW;
  uint32_t now = millis();
  if (down != buttonRaw) {
    buttonRaw = down;
    buttonChangedMs = now;
  }
  if (buttonRaw != buttonStable && now - buttonChangedMs >= kDebounceMs) {
    buttonStable = buttonRaw;
    return buttonStable;
  }
  return false;
}

// Arming listens on all channels for the first note. Gates are closed and
// held notes forgotten, so nothing is left hanging open while learning.
void startLearning() {
  learnState = LearnState::WaitingFirst;
  learnStartedMs = millis();
  ledSavedUntilMs = 0;
  resetOutputs();
  busMidi.setInputChannel(MIDI_CHANNEL_OMNI);
}

void cancelLearning() {
  learnState = LearnState::Idle;
  digitalWrite(kPinLed, LOW);
  applySavedSettings();
}

// Only note-ons advance learning. The note-off of the first learned note (and
// anything else) is ignored. Once the first note is in, the MIDI library's
// input filter is narrowed to its channel, so the second note can only come
// from the same channel. Both steps may learn the same note.
void handleLearnNoteOn(uint8_t channel, uint8_t note) {
  if (learnState == LearnState::WaitingFirst) {
    learnChannel = channel;
    learnFirstNote = note;
    learnState = LearnState::WaitingSecond;
    learnStartedMs = millis(); // restart the pattern so the change is visible
    busMidi.setInputChannel(channel);
    return;
  }

  Settings settings = {kSettingsMagic, learnChannel, {learnFirstNote, note}, 0xFF};
  saveSettings(settings);
  learnState = LearnState::Idle;
  applySavedSettings();

  ledSavedUntilMs = millis() + kLedSavedMs;
  if (ledSavedUntilMs == 0) ledSavedUntilMs = 1; // 0 means "not showing"
  digitalWrite(kPinLed, HIGH);
}

void updateLed() {
  uint32_t now = millis();

  if (learnState == LearnState::Idle) {
    if (ledSavedUntilMs != 0 && (int32_t)(now - ledSavedUntilMs) >= 0) {
      digitalWrite(kPinLed, LOW);
      ledSavedUntilMs = 0;
    }
    return;
  }

  uint32_t t = (now - learnStartedMs) % kLedPatternMs;
  bool on = t < kLedBlinkMs;
  if (learnState == LearnState::WaitingSecond) {
    on = on || (t >= 2 * kLedBlinkMs && t < 3 * kLedBlinkMs);
  }
  digitalWrite(kPinLed, on ? HIGH : LOW);
}

void handleNoteOn(uint8_t note, uint8_t velocity) {
  for (Output &output : outputs) {
    if (output.note != note) continue;

    // Velocity 0-127 -> 0-4064 codes -> 0-2.03V, through the unity buffer.
    writeDac(output.dacChannel, velocity << 5);
    output.held = true;

    if (output.open || output.reopenPending) {
      // Retrigger: drop the gate briefly so the connected module sees a new
      // rising edge. A note-on during an ongoing gap just restarts the gap.
      output.open = false;
      writeGatePin(output, false);
      output.reopenPending = true;
      output.reopenAtUs = micros() + kRetriggerGapUs;
    } else {
      openGate(output);
    }
  }
}

void handleNoteOff(uint8_t note) {
  // The gate itself is closed by updateGates(), once the minimum length is up.
  for (Output &output : outputs) {
    if (output.note == note) output.held = false;
  }
}

void updateGates() {
  uint32_t nowUs = micros();
  uint32_t nowMs = millis();
  for (Output &output : outputs) {
    if (output.reopenPending && (int32_t)(nowUs - output.reopenAtUs) >= 0) {
      openGate(output);
    }
    if (output.open && !output.held && nowMs - output.openedMs >= kMinGateMs) {
      closeGate(output);
    }
  }
}

void setup() {
  for (Output &output : outputs) {
    pinMode(output.pinGate, OUTPUT);
    writeGatePin(output, false);
  }

  pinMode(kPinLed, OUTPUT);
  digitalWrite(kPinLed, LOW);

  pinMode(kPinLearn, INPUT_PULLUP);

  pinMode(kPinDacCs, OUTPUT);
  digitalWrite(kPinDacCs, HIGH);

  // MISO is never read (the MCP4822 has no SDO), but the core wants a pin;
  // PA6 is SPI1's MISO and is unconnected on the player boards.
  SPI.setMOSI(PA7);
  SPI.setMISO(PA6);
  SPI.setSCLK(PA5);
  SPI.begin();

  writeDac(kDacChannelA, 0);
  writeDac(kDacChannelB, 0);

  busMidi.begin(MIDI_CHANNEL_OMNI);
  busMidi.turnThruOff();
  applySavedSettings();
}

void loop() {
  if (learnButtonPressed()) {
    if (learnState == LearnState::Idle) {
      startLearning();
    } else {
      cancelLearning();
    }
  }

  updateLed();
  updateGates();

  if (!busMidi.read()) return;

  switch (busMidi.getType()) {
    case midi::NoteOn:
      if (learnState != LearnState::Idle) {
        handleLearnNoteOn(busMidi.getChannel(), busMidi.getData1());
      } else {
        handleNoteOn(busMidi.getData1(), busMidi.getData2());
      }
      break;
    case midi::NoteOff:
      if (learnState == LearnState::Idle) handleNoteOff(busMidi.getData1());
      break;
    default:
      break;
  }
}
