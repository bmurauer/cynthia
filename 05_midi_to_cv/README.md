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

Cards are passive - there is no local microcontroller. All intelligence
(applying calibration, deciding note-to-card assignment, gate timing) lives on the
core; each card carries only:

- SPI-shiftable DAC/driver hardware (see "The Interface/Bus" for why this needs to
  support genuine hardware daisy-chaining) that turns the calibrated codes/gate
  bits the core already computed into CV/gate output. Percussion-without-velocity
  channels are pure digital (gate bits only), so those don't need a DAC at all.
- An I²C EEPROM storing: card type, DAC calibration data, the notes it plays (for
  percussion cards), and the MIDI channel it should respond to. Read (and, for
  provisioning/calibration, written) directly by the core over the shared I²C bus -
  no card-side logic needed to serve it.
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
every card, but `DATA` is not - each card's DAC/driver hardware shifts `DATA_IN`
through its own register and re-outputs whatever fell off the far end on
`DATA_OUT`, feeding the next card. This is done entirely in hardware (no card-side
MCU), which means the DAC/driver ICs used need to genuinely support daisy-chain
operation (a real SDO-style shift-out pin) - or, for cards/channels without a
suitable off-the-shelf daisy-chainable part, a discrete shift register (e.g.
74HC595) ahead of the DAC/gate transistor. Every expansion card needs two bus
headers (in and out), wired straight through for power/`SCLK`/`LATCH`/I²C, but
*not* straight through for the data pin.

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
   connected expansion cards (type, MIDI channel, notes, DAC calibration data).
2. MIDI messages from all input ports are read by one loop. For each message, the
   table is checked for a card that can process it.
3. The core applies that card's calibration to compute the exact DAC codes/gate
   bits needed, assembles a single SPI message covering all expansion cards, and
   sends it down the chain. Closing a gate is just another such update (core-timed
   note-off or envelope logic) - cards have no local timer.

## Open Questions

- **Polyphony / voice allocation**: not yet decided how notes are allocated across
  multiple melodic-voice cards sharing a MIDI channel (round robin, fixed
  assignment, voice stealing, ...).
- **Gate voltage standard**: leaning 0/+5V (more common in commercial modules than
  the 0/+10V used in `kosmo_poly_midi_2_cv`), not yet fixed.
- **DAC/driver IC selection**: expansion cards are passive (no local MCU), so the
  SPI chain is a genuine hardware shift-register cascade. Need DAC/driver ICs per
  card type that natively support daisy-chain (SDO shift-out) operation, or a
  discrete shift register (e.g. 74HC595) ahead of parts that don't.
