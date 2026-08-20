#05: MIDI to CV

This is a multi-board MIDI-to-CV system: one **conductor** module reads MIDI from
several DIN inputs and broadcasts it to a chain of **players**, each of which is an
independent module that turns the messages it cares about into CV/gate signals. The
number and type of players is flexible, so the system can be grown or reconfigured
after the initial build.

The guiding principle is that the conductor is deliberately dumb. It does not know
what players exist, does not track which one handles what, and holds no state about
them. It parses MIDI and passes it on; every player decides for itself whether a
message is its business.

## The Conductor

- RP2040, in the form of a **Pico module soldered onto a carrier PCB** via its
  castellated edges (see Component Notes for why not a bare chip).
- 3-4 DIN MIDI inputs; some wired to hardware UARTs, some read via software serial.
- Parses every input. Clock, start and stop messages are **handled locally** and are
  not forwarded; everything else is broadcast onto the player bus.
- Clock/start/stop are only honoured from **one designated input jack**, so a second
  device sending clock cannot fight the first.
- Exposes a USB REPL CLI for diagnostics (the Pico module brings its own USB
  connector). This is no longer used to provision players - they configure
  themselves.

## Players

Each player is a self-contained module with its own MCU, its own panel and its own
outputs. It receives the whole broadcast stream and filters it down to the
channel and notes it has learned. Player types identified so far:

- **Melodic voice**: gate, 1V/oct, velocity.
- **Percussion with velocity**: 4 gates, 4 velocity CVs.
- **Percussion without velocity**: 4 gates only (no DAC needed).

Everything is local: note handling, gate timing, velocity scaling, and 1V/oct
calibration all happen on the player's own MCU. Each player carries:

- An **STM32G0** MCU (see Component Notes).
- A DAC on a short private SPI bus, plus an op-amp output stage to scale into the
  final CV range. Percussion-without-velocity players skip the DAC entirely.
- A **MIDI learn button and LED** for configuration.
- Local ±12V-to-3.3V regulation - only raw ±12V is distributed over the bus.
- A small **SWD header** for firmware updates.

Each player has its own eurorack panel, keeping individual PCBs within the <=100mm
height constraint.

### Configuration by MIDI learn

Players are configured in place, with no config bus and no central provisioning.
Press the learn button, the LED blinks, and the next note-on captures its channel
and note number. Percussion players cycle through their four slots on repeated
presses. The result is written to a dedicated page of the STM32G0's internal flash.

Flash erase granularity on the G0 is 2KB, which is coarse, but configuration writes
happen a handful of times in a module's life so wear is a non-issue. One reserved
page is plenty.

This replaces the previous scheme entirely - there is no EEPROM, no I2C bus, no
solder-jumper addressing, and therefore no possibility of the bus order and the
address order disagreeing.

## The Bus

The conductor and players are connected by a ribbon cable carrying power and a
single one-way serial line:

- Power: `+12V`, `-12V` (regulated locally on each player)
- `MIDI_TX`: conductor transmit, fanned out to every player's UART RX

That is the whole bus. Because it is a **one-way broadcast** - one driver, many
high-impedance receivers - there is no arbitration, no addressing, no bus
enumeration and no termination scheme to get wrong. A player is electrically just
another listener; adding one changes nothing for the others.

Suggested pinout on a 2x4 keyed, shrouded 2.54mm IDC box header, with grounds
interleaved between signals and two pins left reserved so the connector does not
have to change if a clock line or a back-channel is ever wanted:

```
+12V, GND, -12V, GND, MIDI_TX, GND, [reserved], [reserved]
```

Keying still matters: a reversed connector would put ±12V onto a logic pin.

Since every signal is now genuinely common to all players, there are two equally
valid wiring styles: two headers per player (in and out, straight through) chained
with short cables between adjacent panels, or a single header per player with one
multi-drop ribbon carrying several IDC connectors crimped along its length - the
same arrangement as a eurorack power bus board. The latter is simpler and was not
possible under the old design, where the data line had to pass *through* each card
rather than past it.

### Merging inputs: expand running status

This is the one piece of conductor firmware that is easy to get subtly wrong.

MIDI uses **running status**: after a status byte, a run of same-type messages may
omit it and send only data bytes. That is fine on a single point-to-point link, but
the conductor is merging 3-4 independent streams onto one wire. If it forwards
bytes verbatim, a running-status run from input A will be interrupted by messages
from input B, and the receiving players will interpret A's orphaned data bytes
against B's status byte. The stream silently corrupts under exactly the conditions
you would want it to work - several controllers playing at once.

Two rules for the conductor:

1. **Re-insert the status byte on every forwarded message.** Never propagate
   running status across the merge.
2. **Forward whole messages atomically.** Buffer per input and serialise complete
   messages; never interleave the bytes of two messages.

### Baud rate

Keep the MIDI *protocol* on the bus so that stock MIDI parsing libraries work
unchanged on the players, but do **not** keep MIDI's 31250 baud.

Worst case, four inputs receiving simultaneously produce 4 x 31250 = 125000 baud of
aggregate traffic, which a single 31250 baud output cannot carry - the conductor's
queues would grow without bound and notes would arrive late under heavy load.

Recommended: **250000 baud**, which is exactly 8x 31250 (so clock division stays
clean) and leaves 2x headroom over the worst-case aggregate.

## Workflow

1. On startup each player loads its learned channel/note configuration from its own
   flash. The conductor does nothing - it has no table to build and no bus to scan.
2. The conductor reads all MIDI inputs. Clock, start and stop from the designated
   jack are handled locally and consumed.
3. Every other message is re-serialised with an explicit status byte and broadcast
   on `MIDI_TX`.
4. Every player receives every message. Each independently decides whether the
   message matches its learned channel/note, and if so updates its own gate, 1V/oct
   and velocity outputs directly. Gate timing is local, so a note-off is handled by
   the player that owns the note without the conductor being involved.

## Component Notes

**Conductor MCU**: RP2040 as a **Pico module**, not a bare chip. The bare RP2040 is
QFN-56 at 0.4mm pitch and needs external QSPI flash, a crystal and careful
decoupling - widely considered a poor first SMD part, needing a stencil plus hot air
or a hotplate. Soldering a Pico down by its castellated edges is trivial by
comparison, and brings flash, crystal and a USB connector with it. The cost is board
area and vertical space behind the panel.

**Player MCU**: **STM32G030F6P6** in TSSOP-20 (6.4 x 4.4mm) is the candidate -
hand-solderable, around $1.50 in ones, 64MHz Cortex-M0+, 32KB flash, 8KB SRAM. Pin
budget is comfortable for both player types:

| Player type | UART RX | SPI | Gates | Button | LED | Total GPIO |
|---|---|---|---|---|---|---|
| Melodic voice | 1 | 3 | 1 | 1 | 1 | 7 |
| Percussion w/ velocity | 1 | 3 | 4 | 1 | 1 | 10 |

TSSOP-20 leaves roughly 14 usable pins after power, SWD and NRST, so both fit with
room to spare. Larger packages (LQFP-32, UFQFPN-28) exist in the same family if a
future player type needs more.

**DACs - the selection constraint is gone.** The previous design needed the unusual
and expensive class of DAC with a genuine hardware cascade (`SDO`) pin, because the
cards were passive and shared one chain. Each player now drives its own DAC over a
short private SPI bus, so an ordinary per-chip chip-select part is fine and the
whole market is available.

Default recommendation: **MCP4822** - 8-pin SOIC, cheap, dual 12-bit, internal
2.048V reference. There is already working code for this part family in
`kosmo/kosmo_modular_midi_2_cv/code/src/10_modular_midi_2_cv.cpp`: the
`0x1000`/`0x9000` command words and the `NOTE_SF` scaling carry over directly, as
does that file's note that the output "will need to be amplified by 1.77X for the
standard 1V/octave". At 3.3V only gain-of-1 is usable (0-2.048V out), which is
exactly what that code already assumes, with the op-amp stage scaling up.

One tradeoff worth knowing: reference tempco shows up as a gain error on 1V/oct and
scales with output voltage. At 0.833mV per cent, a 20°C warm-up gives roughly 6
cents of drift at 50ppm/°C versus about 0.25 cents at 2ppm/°C. A trimmer corrects
static error but not drift. If pitch stability turns out to matter, the cheap fix is
an external-reference part such as the **MCP4922** plus a precision reference
(REF3025, LM4040) - far cheaper than a precision DAC, and now perfectly allowed
since nothing has to daisy-chain.

**Logic rail**: 3.3V. The STM32G0 runs 1.7-3.6V and the MCP4822 2.7-5.5V, so both
are happy, and the whole digital side matches the Pico natively with no level
shifting on the bus.

Gates therefore emerge at 0-3.3V and need one small output stage to reach the gate
standard. Either a **74HCT244 on a 5V rail** (TTL thresholds accept 3.3V input
directly, non-inverting, 8 gates per chip) or **one 2N7002 plus a pull-up per gate**
(no extra rail, but inverting, so firmware flips the bit).

Suggested rails, cascading rather than dropping 12V straight to 3.3V:

```
+12V ---> 5V LDO ---> 3.3V LDO ---> STM32G0, DAC
             `----------------------> 74HCT244 gate buffer (if used)
±12V ------------------------------> op-amp output stages
```

The 3.3V regulator then drops only 1.7V instead of 8.7V. A direct 12V->3.3V LDO at
~25mA burns over 200mW and runs warm in a small package - avoidable for the cost of
one extra regulator.

**Firmware updates**: because the bus is one-way and broadcast, the STM32 ROM
bootloader cannot be aimed at a single player - every player would answer at once.
Give each player a small SWD header and use a cheap ST-Link. The conductor updates
over its Pico USB connector as usual.

## Latency and Bandwidth

A 3-byte MIDI message takes ~0.96ms to arrive over a 31.25kbit/s DIN cable. That
inbound time dominates everything downstream:

| Stage | Time |
|---|---|
| Inbound DIN message (3 bytes @ 31250) | ~0.96ms |
| Conductor parse and re-serialise | microseconds |
| Broadcast on bus (3 bytes @ 250000) | ~0.12ms |
| Player parse + DAC write | microseconds |

Total added latency over a direct MIDI connection is well under a fifth of the time
the message spent on the input cable. Bus bandwidth is not a constraint at any
player count this system will reach; the 250000 baud recommendation exists to
absorb *aggregate* load from several simultaneously active inputs, not to reduce
per-message latency.

## Superseded Design

An earlier version of this module used a smart core plus **passive** expansion
cards: the core applied all calibration and drove a daisy-chained SPI bus of DACs
and 74HC595 shift registers, with a second I2C bus of per-card EEPROMs holding
configuration. It was abandoned on cost and complexity grounds. Recorded here so
the idea is not re-proposed:

- The SPI chain required DACs with a genuine cascade `SDO` pin - an expensive part
  class, and it specifically excluded cheap commodity parts like the MCP4822.
- Because every chip latched simultaneously, each message had to carry the full
  state of every gate and CV channel on the entire bus; partial updates were
  impossible.
- It produced a string of subtle hazards: `LDAC`-versus-`RCLK` commit ordering, a
  DAC-to-595 clock-edge hold-time race that no amount of slowing the bus could fix,
  I2C pull-up loading across 8 cards, the 24Cxx capacity/address-pin trap, and a
  requirement that physical card order match EEPROM address order with no way to
  detect a mismatch.
- The bus needed 16 conductors; the replacement needs 6.

The full reasoning is in git history (see `d6e827c` and earlier) if any of it is
ever needed again.

## Open Questions

- **Polyphony / voice allocation - the main open design question.** Central
  allocation disappeared along with the smart core. Players now filter
  independently, so two melodic players sharing a MIDI channel both play the *same*
  note in unison rather than dividing polyphony between them. Options:
  - **(a) One MIDI channel per melodic player** - simplest, recommended. Voice
    allocation becomes the sequencer's or DAW's job, which is what they are good at.
  - **(b) Deterministic distributed allocation** - every player runs identical
    allocation logic over the identical broadcast stream, and each knows its own
    index N of M, so they reach the same answer without communicating. Elegant and
    free, but desyncs if a player resets or is added mid-performance.
  - **(c) Conductor rewrites channels before broadcasting** - works, but puts state
    back into the conductor, which is what this overhaul removed.
- **1V/oct calibration method.** Central calibration died with the core, and there
  is no REPL path or back-channel to a player. Recommendation is a per-output analog
  trimmer, matching existing practice in `kosmo_poly_midi_2_cv`. A software
  calibration in flash is possible, but with no measurement path a player cannot
  calibrate itself, so a human adjusting a trimmer is the pragmatic answer.
- **Gate voltage standard**: leaning 0/+5V (more common in commercial modules than
  the 0/+10V used in `kosmo_poly_midi_2_cv`), not yet fixed.
- **Conductor clock outputs**: assumed that "handles clock/start/stop" means the
  conductor carries its own clock / run / reset output jacks on its panel. Needs
  confirming - it also decides how much panel space the conductor needs.
