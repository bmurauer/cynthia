#include <Arduino.h>
#include <MIDI.h>
#include <SPI.h>

// Pin mapping, matching player_melodic.net:
//
//   PB7  (pin 1)   FAST_MIDI   - bus RX, USART1
//   PA2  (pin 9)   learn       - SW1 to GND, internal pull-up
//   PA3  (pin 10)  gate        - R1 -> Q1 base; LED + 1k to GND on the breadboard
//   PA4  (pin 11)  DAC ~CS     - MCP4822 pin 2
//   PA5  (pin 12)  DAC SCK     - MCP4822 pin 3, SPI1
//   PA7  (pin 14)  DAC SDI     - MCP4822 pin 4, SPI1
//   PA11 (pin 16)  LED         - D1 anode, cathode via R8 to GND (active high).
//                                Pin 16 can be remapped to PA9; left at the
//                                reset default, PA11.
const uint32_t kPinLearn = PA2;
const uint32_t kPinGate = PA3;
const uint32_t kPinDacCs = PA4;
const uint32_t kPinLed = PA11;

// Q1 inverts: PA3 high pulls the gate jack low. The breadboard LED sits
// directly on PA3 instead, so it wants the opposite. Set per environment in
// platformio.ini.
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
constexpr uint16_t kDacChannelA = 0x3000; // note CV, into U1A
constexpr uint16_t kDacChannelB = 0xB000; // velocity CV, into U1B (buffer)

// U1A amplifies the note CV by 1 + (R7 + RV1) / R3 = 1 + (18k + 0..5k) / 10k,
// i.e. 2.8x..3.3x. The DAC is scaled for RV1 at mid-travel (3.05x), leaving
// about +-8% trim both ways; RV1 is then set so that one octave is exactly 1V
// at the jack.
constexpr uint32_t kNoteGainMilli = 3050;

// DAC codes per semitone: (1000mV / 12) / gain / 0.5mV per code = 54.6448...
// Truncating that to 54 would leave the top note ~48 codes (~70 cents) flat,
// so it is kept as 16.16 fixed point instead - scaled by 65536, with the
// integer part in the upper 16 bits and the fraction in the lower 16:
//
//   54.6448 * 65536 = 3581142.6 -> 3581143
//
// With the gain in thousandths, (1000 / 12) / (gainMilli / 1000) / 0.5 * 65536
// simplifies to 2000000 * 65536 / (12 * gainMilli). Adding half the divisor
// (6 * gainMilli) first makes the integer division round to nearest instead
// of truncating.
//
// All of this is constexpr, so only the finished constant ends up in the
// binary - the M0+ has no FPU, and the soft-float routines cost about 3KB of
// flash here. noteToDacCode() then needs only a multiply, add and shift; see
// there for a worked example. Storing the constant rounded costs at most
// 0.5/65536 code per semitone, under 0.001 code across the whole range.
constexpr uint32_t kDacCodesPerSemitoneQ16 =
    (2000000ULL * 65536 + 6 * kNoteGainMilli) / (12 * kNoteGainMilli);

// Lowest note, output at 0V. 4095 codes / ~54.6 per semitone gives 75 notes
// (about 6.2V at the jack), so the range is C1..D7; notes outside it are
// clamped.
constexpr uint8_t kLowestNote = 24; // C1

// Learned settings live in the last flash page, which platformio.ini keeps
// out of the linker's flash region. One 64-bit word is programmed per save;
// an erased page reads as all 0xFF, which fails the magic check, so a fresh
// chip falls back to listening on all channels until it has learned one.
constexpr uint32_t kSettingsPage = 15;
constexpr uint32_t kSettingsAddress = FLASH_BASE + kSettingsPage * FLASH_PAGE_SIZE;
constexpr uint32_t kSettingsMagic = 0x43594E31; // "CYN1"

struct Settings {
  uint32_t magic;
  uint8_t channel; // 1-16
  uint8_t reserved[3];
};
static_assert(sizeof(Settings) == 8, "Settings must fit one flash doubleword");

constexpr uint32_t kDebounceMs = 20;
constexpr uint32_t kLearnBlinkMs = 125; // LED blink half-period while armed

// Counts held notes, so the gate stays open while a chord (or legato line)
// is held and only closes when the last key is released. The pitch always
// follows the most recent note-on. Note-on with velocity 0 already arrives
// as NoteOff (library default).
uint8_t heldNotes = 0;

bool learning = false;
bool learnBlinkOn = false;
uint32_t learnBlinkChangedMs = 0;

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

// Semitones above kLowestNote times the 16.16 constant gives the DAC code,
// still scaled by 65536. Adding 0x8000 (0.5 in 16.16) and shifting right by
// 16 divides by 65536, rounded to nearest. E.g. C3, 24 semitones above C1:
//
//   24 * 3581143 = 85947432      (1311.475 * 65536)
//   + 0x8000     = 85980200
//   >> 16        = 1311          -> 655.5mV at the DAC, 2V at the jack
//
// The largest product, 127 * 3581143 ~= 455M, fits easily in 32 bits.
uint16_t noteToDacCode(uint8_t note) {
  if (note <= kLowestNote) return 0;
  uint32_t code = ((note - kLowestNote) * kDacCodesPerSemitoneQ16 + 0x8000) >> 16;
  return code > 4095 ? 4095 : code;
}

void setGate(bool open) {
  digitalWrite(kPinGate, open != kGateInverted ? HIGH : LOW);
}

// Returns the saved channel, or MIDI_CHANNEL_OMNI if nothing valid is stored.
uint8_t loadChannel() {
  const Settings *saved = reinterpret_cast<const Settings *>(kSettingsAddress);
  if (saved->magic != kSettingsMagic) return MIDI_CHANNEL_OMNI;
  if (saved->channel < 1 || saved->channel > 16) return MIDI_CHANNEL_OMNI;
  return saved->channel;
}

// Erases the settings page and programs the new settings. The CPU stalls for
// the ~20-40ms page erase, so bus bytes arriving meanwhile are lost - fine,
// since this only happens once per learn.
bool saveChannel(uint8_t channel) {
  if (loadChannel() == channel) return true; // spare the flash a needless erase

  Settings settings = {kSettingsMagic, channel, {0xFF, 0xFF, 0xFF}};
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

// Arming listens on all channels and blinks the LED. The gate is closed and
// stays closed until learning ends, since held notes are forgotten. Pressing
// the button again cancels and restores the old channel.
void startLearning() {
  learning = true;
  heldNotes = 0;
  learnBlinkOn = true;
  learnBlinkChangedMs = millis();
  digitalWrite(kPinLed, HIGH);
  setGate(false);
  busMidi.setInputChannel(MIDI_CHANNEL_OMNI);
}

void stopLearning(uint8_t channel) {
  learning = false;
  heldNotes = 0;
  digitalWrite(kPinLed, LOW);
  setGate(false);
  busMidi.setInputChannel(channel);
}

void updateLearnBlink() {
  uint32_t now = millis();
  if (now - learnBlinkChangedMs < kLearnBlinkMs) return;
  learnBlinkChangedMs = now;
  learnBlinkOn = !learnBlinkOn;
  digitalWrite(kPinLed, learnBlinkOn ? HIGH : LOW);
}

void handleNoteOn(uint8_t note, uint8_t velocity) {
  // Velocity 0-127 -> 0-4064 codes -> 0-2.03V, through the unity buffer.
  writeDac(kDacChannelB, velocity << 5);
  writeDac(kDacChannelA, noteToDacCode(note));
  if (heldNotes < 255) heldNotes++;
  setGate(true);
}

void handleNoteOff() {
  if (heldNotes > 0) heldNotes--;
  if (heldNotes == 0) setGate(false);
}

void setup() {
  pinMode(kPinGate, OUTPUT);
  setGate(false);

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

  busMidi.begin(loadChannel());
  busMidi.turnThruOff();
}

void loop() {
  if (learnButtonPressed()) {
    if (learning) {
      stopLearning(loadChannel());
    } else {
      startLearning();
    }
  }

  if (learning) updateLearnBlink();

  if (!busMidi.read()) return;

  switch (busMidi.getType()) {
    case midi::NoteOn:
      if (learning) {
        // Only the channel is learned; the note itself is not played.
        uint8_t channel = busMidi.getChannel();
        saveChannel(channel);
        stopLearning(channel);
      } else {
        handleNoteOn(busMidi.getData1(), busMidi.getData2());
      }
      break;
    case midi::NoteOff:
      if (!learning) handleNoteOff();
      break;
    default:
      break;
  }
}
