#05: MIDI to CV

This is a multi-board MIDI-to-CV system: one core module reads MIDI and drives a
chain of expansion cards, each of which turns part of that MIDI data into CV/gate
signals. The number and type of expansion cards is flexible, so the system can be
grown (or reconfigured) after the initial build.

## The Core

- Based on a Raspberry Pi Pico.
- 3-4 DIN MIDI inputs; some wired to hardware UARTs, some read via software serial.
- All input ports are read by the same loop; each incoming MIDI message is checked
  against an in-memory table (see "Workflow" below) to see which expansion card, if
  any, should receive it.
- Sends commands to expansion cards over a daisy-chained SPI bus.
- Exposes a REPL CLI (used from an external computer) for provisioning and
  calibrating expansion cards.

## Expansion Cards

Expansion cards receive their share of an SPI message and turn it into CV/gate
output. Card types identified so far:

- **Melodic voice**: gate, 1V/oct, velocity.
- **Percussion with velocity**: 4 gates, 4 velocity CVs.
- **Percussion without velocity**: 4 gates only.

Each card carries:

- A small local MCU. Besides driving CV/gate outputs, it acts as the SPI relay
  stage in the daisy chain (see "The Interface/Bus"), and owns the I²C EEPROM
  access described below.
- An I²C EEPROM storing: card type, DAC calibration data, the notes it plays (for
  percussion cards), and the MIDI channel it should respond to.
- Local ±12V-to-logic regulation — only raw ±12V is distributed over the bus, each
  card regulates what it needs locally.

Each expansion card has its own eurorack panel (keeping individual PCBs within the
<=100mm height constraint).

**EEPROM addresses are set manually** (solder jumpers), giving up to 8 devices per
bus. Because addressing is manual, the physical bus order and the address order
must match, or cards may misbehave. There is no runtime detection for this - it
will be documented as a build/wiring caveat rather than solved in firmware.

## The Interface/Bus

Core and expansion cards are connected by a ribbon cable bus carrying:

- Power: `+12V`, `-12V` (regulated locally on each card - see above)
- SPI: `SCLK`, `LATCH` (bussed in parallel to all cards - every card shifts and
  latches in lockstep), plus a per-hop `DATA_IN`/`DATA_OUT` pair (**not** a shared
  net - see below)
- I²C: `SDA`, `SCL` (true multi-drop bus, shared by all cards, used for EEPROM
  access)

**On the SPI data line specifically**: this is a true shift-register-style chain,
not addressed SPI with individual chip-selects. `SCLK` and `LATCH` are common to
every card, but `DATA` is not - each card's MCU receives `DATA_IN`, consumes the
slice of the message meant for it, and re-transmits the remainder out `DATA_OUT`
to the next card. So every expansion card needs two bus headers (in and out), wired
straight through for power/`SCLK`/`LATCH`/I²C, but *not* straight through for the
data pin.

Connector recommendation: keyed, shrouded 2.54mm IDC box headers (2x5 or 2x7),
matching the connector family already used for the eurorack power bus - familiar
tooling, and the keying matters here since a reversed connector would put ±12V on
logic pins. Ground should be interleaved with signal lines in the ribbon pinout to
reduce coupling into audio, e.g.:

```
+12V, GND, -12V, GND, DATA, GND, SCLK, GND, LATCH, GND, SDA, GND, SCL, GND
```

Provisioning and calibration of a card's EEPROM happens in two stages from an
external computer via the core's REPL CLI: (1) provisioning (type, MIDI channel,
notes), and (2) calibration (DAC trim), only where needed.

## Workflow

1. On startup, the core scans the I²C bus and builds an in-memory table of
   connected expansion cards (type, MIDI channel, notes).
2. MIDI messages from all input ports are read by one loop. For each message, the
   table is checked for a card that can process it.
3. A single SPI message covering all expansion cards is assembled and sent down the
   chain each time.

## Open Questions

- **Polyphony / voice allocation**: not yet decided how notes are allocated across
  multiple melodic-voice cards sharing a MIDI channel (round robin, fixed
  assignment, voice stealing, ...).
- **Gate voltage standard**: leaning 0/+5V (more common in commercial modules than
  the 0/+10V used in `kosmo_poly_midi_2_cv`), not yet fixed.
- **Per-card MCU vs. discrete daisy-chainable DAC**: current assumption is a small
  MCU per expansion card (needed anyway for EEPROM/calibration), acting as the SPI
  relay stage. Worth a look at DAC ICs with native SPI daisy-chain (SDO) support as
  an alternative if a card type turns out not to need any other local intelligence.
