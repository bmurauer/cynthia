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
- SPI: `SCLK`, `SYNC`, `LDAC` (all bussed in parallel to all cards), plus a
  per-hop `DATA_IN`/`DATA_OUT` pair (**not** a shared net - see below)
- I²C: `SDA`, `SCL` (true multi-drop bus, shared by all cards, used for EEPROM
  access)

**Two separate control signals, not one `LATCH`**: loading data and applying it to
the outputs are distinct steps, and they need distinct wires:

- `SYNC` frames the SPI transfer. It is held low for the *entire* bus-wide
  daisy-chain message and released at the end, at which point every DAC decodes
  whatever 24 bits ended up in its own register. Only DAC-bearing cards connect
  it; a gate-only card leaves it unconnected (the 74HC595 has no `SYNC` equivalent
  and shifts on every clock regardless).
- `LDAC` commits loaded values to the analog/gate outputs, simultaneously across
  every chip on the whole bus. Every card connects it. See "Output commit" below.

**On the SPI data line specifically**: this is a true shift-register-style chain,
not addressed SPI with individual chip-selects. `SCLK`, `SYNC` and `LDAC` are
common to every card, but `DATA` is not - each card's DAC/driver hardware shifts
`DATA_IN` through its own register and re-outputs whatever fell off the far end on
`DATA_OUT`, feeding the next card. This is done entirely in hardware (no card-side
MCU), which means the DAC/driver ICs used need to genuinely support daisy-chain
operation (a real SDO-style shift-out pin) - or, for cards/channels without a
suitable off-the-shelf daisy-chainable part, a discrete shift register (e.g.
74HC595) ahead of the DAC/gate transistor. Every expansion card needs two bus
headers (in and out), wired straight through for power/`SCLK`/`SYNC`/`LDAC`/I²C,
but *not* straight through for the data pin.

**Output commit (`LDAC`) is shared between DACs and gate shift registers.** The
74HC595 already has exactly the two-stage architecture that `LDAC` exists to
serve, so the two roles map one-to-one and share a single bus wire:

| DAC | 74HC595 |
|---|---|
| input register (holds pending value) | shift register |
| DAC register (drives output) | storage register |
| `LDAC` (commit) | `RCLK` (commit) |

The polarities work out, and give the correct ordering for free. `LDAC` is
active-low (pulsing low transfers all input registers to their DAC registers at
once); `RCLK` is rising-edge-triggered. So on a single shared pulse:

- **falling edge** -> DAC outputs update, CV starts slewing
- **rising edge** -> 74HC595 commits, gates open

That is the right order for note-on - pitch CV before gate, avoiding the classic
"VCO briefly sounds the previous note" blip. Better still, **the pulse width
becomes the CV settling budget**: these DACs settle in ~5-10us, so a ~10us `LDAC`
low pulse means the gate physically cannot open until the pitch CV has settled.
10us is free next to the ~1ms it takes the triggering MIDI message to arrive.
Holding `LDAC` low that long is safe as long as no SPI writes happen during it,
which they don't - the shift phase is already complete by then.

Connector recommendation: keyed, shrouded 2.54mm IDC box headers, matching the
connector family already used for the eurorack power bus - familiar tooling, and
the keying matters here since a reversed connector would put ±12V on logic pins.
Ground should be interleaved with signal lines in the ribbon pinout to reduce
coupling into audio. With 7 signals plus interleaved grounds that lands on 16
conductors, i.e. a standard 2x8 header:

```
+12V, GND, -12V, GND, DATA, GND, SCLK, GND, SYNC, GND, LDAC, GND, SDA, GND, SCL, GND
```

Provisioning and calibration of a card's EEPROM happens in two stages from an
external computer via the core's REPL CLI: (1) provisioning (type, MIDI channel,
notes), and (2) calibration (DAC trim), only where needed.

**Message length and framing**: because `DATA` ripples through every chip on
every card in series (not just once per card), the total bus-wide message length
is the sum of every individual chip's register width across the whole bus - a
melodic voice card contributes 24 bits per DAC channel write plus 8 bits for its
gate register; a percussion-without-velocity card contributes only its 74HC595's
8 bits. Data for the chip furthest from the core must be sent first, since
everything sent after it pushes it one hop further down the chain. Because each
chip only keeps whatever is in its own register when the transfer ends,
**every message must carry the full, current state of every gate and CV channel
on the bus** - there is no such thing as a partial/delta update, particularly for
the 74HC595 gate registers, which have no concept of "leave this bit alone."

## Workflow

1. On startup, the core scans the I²C bus and builds an in-memory table of
   connected expansion cards (type, MIDI channel, notes, DAC calibration data).
2. MIDI messages from all input ports are read by one loop. For each message, the
   table is checked for a card that can process it.
3. The core applies that card's calibration to compute the exact DAC codes/gate
   bits needed, assembles a single SPI message covering all expansion cards, and
   sends it down the chain with `SYNC` held low for the whole transfer.
4. The core releases `SYNC` (every DAC decodes its own 24 bits), then pulses
   `LDAC` low for ~10us - CV outputs update on the falling edge and settle, gates
   commit on the rising edge. Closing a gate is just another such cycle
   (core-timed note-off or envelope logic) - cards have no local timer.

## Component Notes

**DACs**: Analog Devices AD5686/AD5684 (quad, 16-/12-bit) and their dual siblings
AD5689/AD5687 (16-/12-bit) - "nanoDAC+" family. These genuinely support hardware
daisy-chain: past the first 24 clock pulses, data ripples straight through the
input shift register and out the `SDO` pin, so `SDO -> next chip's SDIN` with
`SCLK`/`SYNC`/`LDAC` common to all - matches the bus directly. Since the
core applies per-card calibration in software, chip-level absolute accuracy
doesn't matter much - resolution and monotonicity do. 12-bit (AD5684/AD5687) is
plenty for velocity (MIDI velocity is only 7-bit anyway); 16-bit (AD5686/AD5689)
is worth it on 1V/oct if smooth portamento/pitch-bend matters. Candidate mapping:

- Melodic voice (1V/oct + velocity, 2ch): AD5687 or AD5689 (dual)
- Percussion w/ velocity (4ch): AD5684 or AD5686 (quad)

All four share the same daisy-chain protocol and pinout family, so mixing
resolution tiers by card type is fine. Get the **TSSOP-16** package, not the 3x3mm
LFCSP variant - the LFCSP has no external leads and needs hot air/reflow, a bad
first SMD part. Note the `SDO` pin is shared between daisy-chain pass-through and
a software-invoked readback mode - firmware must never issue a readback command
mid-chain. These DACs run off a single 2.7-5.5V logic supply with an internal
2.5V reference (output only swings to ~5V), so each channel still needs a small
downstream op-amp stage to scale/offset into the final CV range - fits alongside
the local ±12V regulation each card already has.

**Gates**: 74HC595 shift register, one per card. Has its own genuine daisy-chain
pin (`SER` in, `Q7'` out), a shift clock (`SRCLK`) that maps onto the bus `SCLK`,
and a storage-register clock (`RCLK`) that maps onto the bus `LDAC` - see "Output
commit" above for why those two roles are genuinely the same signal. SOIC-16,
cheap, 1.27mm pin pitch - a forgiving first SMD part. One chip covers all 4 gates
on a percussion card (4 bits spare); one bit of it covers the melodic voice's
single gate.

**Clock edge mismatch between the two part types** - worth knowing before the
first prototype. The DAC samples `SDIN` on the **falling** SCLK edge and shifts
`SDO` out on the **rising** edge; the 74HC595 samples `SER` on the **rising**
edge and updates `Q7'` on the **rising** edge too. The two chain-boundary
directions are therefore not equally safe:

- **595 -> DAC**: `Q7'` changes on rising, DAC samples on falling - half a clock
  period of settling. Solid.
- **DAC -> 595**: `SDO` changes on rising, 595 samples on that *same* rising edge
  - a race.

The second case most likely works, for the same reason ordinary 595-to-595
cascades work: the driving chip's propagation delay exceeds the receiving chip's
hold requirement (74HC595 hold time is a few ns at 5V, DAC `SDO` propagation
delay is tens of ns). But it should be confirmed on the bench with a scope rather
than assumed. **Note this is a hold-time question, not a setup-time one, so
running the bus slower does not help** - both events are referenced to the same
edge and the clock period never enters into it. If it does bite, the fix is an
inverter on the 595's shift clock, or selecting a DAC whose `SDO` clocks out on
the falling edge.

## SPI Timing

Worked example: 3 melodic voice cards + 1 percussion-with-velocity card + 1
percussion-without-velocity card. Using the per-card bit costs above (24 bits per
DAC channel write, 8 bits per 74HC595 gate register):

| Card | Contents | Bits |
|---|---|---|
| Melodic voice x3 | (2x24) + 8 = 56 bits each | 168 |
| Percussion w/ velocity x1 | (4x24) + 8 | 104 |
| Percussion w/o velocity x1 | 8 | 8 |
| **Total** | | **280 bits** |

`SCLK` is shared, so the slowest device on the bus caps the rate - likely the
74HC595 (~25-30MHz typical ceiling) rather than the DACs (50MHz). Shift time for
280 bits:

| SCLK | Time |
|---|---|
| 1MHz (conservative, ribbon-safe) | 280us |
| 4MHz | 70us |
| 10MHz | 28us |
| 25MHz | 11.2us |

Add ~10us for the `LDAC` commit pulse (sized to cover DAC settling - see "Output
commit"), which is independent of card count.

For context, a single 3-byte MIDI message takes ~0.96ms to arrive over a
31.25kbit/s DIN cable - even at a conservative 1MHz `SCLK`, updating this whole
5-card bus (280us + 10us) is roughly 3x faster than the MIDI message that
triggered it. Scaling to the max 8-card bus, all melodic voices (448 bits), is
still under 0.5ms at 1MHz. SPI bus bandwidth is not expected to be a real
constraint at any card count this system is likely to reach.

## Open Questions

- **Polyphony / voice allocation**: not yet decided how notes are allocated across
  multiple melodic-voice cards sharing a MIDI channel (round robin, fixed
  assignment, voice stealing, ...).
- **Gate voltage standard**: leaning 0/+5V (more common in commercial modules than
  the 0/+10V used in `kosmo_poly_midi_2_cv`), not yet fixed.
- **DAC/driver IC selection**: expansion cards are passive (no local MCU), so the
  SPI chain is a genuine hardware shift-register cascade. Need DAC/driver ICs per
  card type that natively support daisy-chain (SDO shift-out) operation, or a
  discrete shift register (e.g. 74HC595) ahead of parts that don't. Candidate
  parts noted under "Component Notes" below.
- **~~Multi-channel-per-chip updates within one MIDI event~~** - resolved, see
  "Output commit" under The Interface/Bus. The AD5686 family's `LDAC` is active
  low and commits *all* channels of *all* chips at once, decoupled from the shift
  phase, so one shift pass plus one `LDAC` pulse covers a whole MIDI event
  including same-chip multi-channel writes. No multi-pass needed, and no need for
  single-channel-per-signal DACs to work around it. The exact `LDAC` timing
  numbers still want a datasheet check when the part is finally chosen.
- **DAC -> 595 clock edge race**: see "Clock edge mismatch" under Component Notes.
  Expected to work, but needs scope confirmation on the first prototype since it
  is a hold-time margin question that cannot be fixed by slowing the bus down.
