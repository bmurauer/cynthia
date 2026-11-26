# #07: Buffered Multiple

Copies signals without loading their source: two inputs, each copied to four buffered outputs. Input 2 is normalled to
input 1, so with only input 1 patched, all eight outputs carry input 1. Every output has its own op-amp, so a short
or a heavy load on one output doesn't affect the others or the source. There are no panel controls.

The module passes anything from -10V to +10V without clipping or phase reversal. That covers every signal in the
current system: audio (±5V, #01), 1V/oct pitch (0 to 6.2V, #05), envelopes (up to ±9.1V, #02) and gates (0/+5V, and
#02's EOC at about +10V).

Status: rev 1 PCB laid out (28.5 x 100mm, fits a 6HP panel), not ordered yet. See `STATUS.md`.

## I/O Specification

Fill in the "Design" columns before building; fill in the "As-Built" columns after assembly and bench testing.

### Inputs

| Ref | Label | Signal Type (Design) | Expected Behavior / Range (Design) | Measured Behavior / Range (As-Built) | Notes |
|-----|-------|-----------------------|--------------------------------------|-----------------------------------------|-------|
| J1 | INPUT_1 | audio, CV or gate | -10V to +10V, DC-coupled | | Grounded when unplugged. R1 (1M) pulls the input to GND when a cable is plugged in with its other end open. Feeds U1 and, while J2 is unpatched, U2. |
| J2 | INPUT_2 | audio, CV or gate | -10V to +10V, DC-coupled | | Normalled to INPUT_1. R2 (1M) pulldown, as on J1. Feeds U2. |

### Outputs

| Ref | Label | Signal Type (Design) | Expected Behavior / Range (Design) | Measured Behavior / Range (As-Built) | Notes |
|-----|-------|-----------------------|--------------------------------------|-----------------------------------------|-------|
| J3 | OUTPUT_1 | copy of INPUT_1 | unity gain, -10V to +10V | | U1A, R3 |
| J4 | OUTPUT_2 | copy of INPUT_1 | unity gain, -10V to +10V | | U1B, R4 |
| J5 | OUTPUT_3 | copy of INPUT_1 | unity gain, -10V to +10V | | U1C, R5 |
| J6 | OUTPUT_4 | copy of INPUT_1 | unity gain, -10V to +10V | | U1D, R6 |
| J7 | OUTPUT_5 | copy of INPUT_2 | unity gain, -10V to +10V | | U2A, R7 |
| J8 | OUTPUT_6 | copy of INPUT_2 | unity gain, -10V to +10V | | U2B, R8 |
| J9 | OUTPUT_7 | copy of INPUT_2 | unity gain, -10V to +10V | | U2C, R9 |
| J10 | OUTPUT_8 | copy of INPUT_2 | unity gain, -10V to +10V | | U2D, R10 |

## Circuit

Each output is a TL074H section wired as a unity-gain follower. It drives its jack through a 1k resistor, and the
feedback is taken from the **jack side** of that resistor:

- **Exact copies for 1V/oct.** With the 1k outside the loop, a typical 100k input would see about 1% less voltage,
  which is about 12 cents per octave. Inside the loop, the op-amp makes up for the drop, so the output reads the input
  voltage at any load. What's left is the TL074H's offset (±4mV max, about 5 cents). That's a constant shift that the
  VCO's tune knob absorbs, not a scaling error.
- **Short protection still works.** If the jack is shorted to GND, the op-amp saturates and the 1k limits the current
  to about 11mA, well under the TL074H's 26mA short-circuit limit. The same goes for two outputs patched together
  (at least 2k in series). The 1k then dissipates about 120mW, close to the 125mW rating of an 0805, so a short left
  in place for a long time is the one case that stresses it.
- **The 1k isolates the op-amp from cable capacitance.** It moves into the loop, though, so long or capacitive
  loads are worth checking at bring-up (see below). If an output rings, the standard fix is about 10k from the jack to
  the inverting input plus about 100pF from the op-amp output to the inverting input.

The TL074H is the reason for the ±10V range. On the real rails of about ±11.6V, its input range reaches from about
-10.1V to +11.6V, its output swings to within about 0.2V of the rails, and it doesn't reverse phase. The old TL074
die would be guaranteed only to about ±7.6V and could flip on large negative inputs, so the H must be in the BOM.

Power comes in through the usual 1N5819 reverse-polarity diodes and 22µF bulk caps (C1, C2), with a 100nF decoupling
cap per rail at each IC (C3 to C6). TP1, TP2 and TP3 give +12V, GND and -12V.

## Why buffered

None of the current modules strictly need a buffer to share a signal. Their outputs have 1k or less in series (11k for
#05's gates), and their inputs are about 100k, so a passive multiple into four inputs costs about 4% of the level,
and gates stay well above the inputs' 1-2.5V thresholds. The buffered version is for pitch CV into several VCOs,
where it keeps the scaling exact, and for future modules whose impedances aren't known yet.

## Bring-up

1. Measure +12V and -12V at TP1 and TP3, and at the far end of the bus.
2. With nothing patched, all outputs should read 0V (within a few mV).
3. Patch a DC voltage into J1 and check that all eight outputs match it. Then patch a different voltage into J2 and
   check that only J7 to J10 follow it.
4. Check the range: feed about ±10V (or the largest signals the system has, e.g. #02's envelope at max LEVEL, both
   polarities) and check that no output clips or jumps.
5. **Ringing check:** feed a square wave (e.g. #01's SQUARE OUT), load one output with a long patch cable, and look
   at the far end with the scope. Small overshoot is fine; sustained ringing calls for the compensation fix above.
