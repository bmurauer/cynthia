#01: AS3340 VCO

This design is based on the [design from Eddy Bergman](https://www.eddybergman.com/2020/01/synthesizer-build-part-18-really-good.html), which is based on a DigiSound 80 VCO.

I have made the following modifications to it:

 - I have included a switch (MODE) that adds a larger capacitor, allowing for mad slow LFO action. The 820pF timing capacitor (C17) is soldered permanently next to the AS3340; in LFO mode the switch adds the 1uF film capacitor (C14) in parallel. This keeps the timing node short in VCO mode instead of routing it through the board-to-board header and the switch twice.
 - the output stages are modified so that the VCO roughly produces -5V/+5V outputs (instead of 0/+10V).
 - the design includes a sine wave output. This is based on Eddy Bergmans Triangle to Sine converter.
 - the sine converter has a distortion trimmer (RV9) and a symmetry trimmer (RV10), and uses a matched transistor pair (BCM847BS, Q2) so the trimmed symmetry doesn't drift as the module warms up. There is no separate triangle-centering trimmer; small DC offsets on the triangle are compensated by RV10.
 - the PWM input stage uses one less op amp. For the PWM signal, the polarity of the CV signal is irrelevant, as (1) there is no fixed notion what "positive PWM" looks like in the output wave, and (2) the two different resulting options sound the same to the human ear. If you have a certain setup for LFOs in mind where this detail is important, note that the signal may be inverted before being put into the AS3340 chip.
 - the design does not include a zener diode, as the square wave is centered and scaled like the other stages.
 - I removed the second 1V/O CV input, as I personally don't to use it. The FM input is plenty for me.


The SYNC MODE switch is a 4-position switch that selects one of the following SYNC modes:
1. Hard + Sync: hard syncs on the rising edge of incoming signal
2. Hard +/- Sync: hard syncs on both rising and falling edge of incoming signal
3. Hard - Sync: hard syncs on falling edge of incoming signal
4. Soft: Soft syncs on both rising and falling edge of incoming signal



## Tuning Procedure

Adapted from Eddy Bergman's "TUNING THE VCO" section ([source](https://www.eddybergman.com/2020/01/synthesizer-build-part-18-really-good.html)), which this design is based on.

**Trimmer mapping** (main board, per the KiCad component descriptions "Trimpot A/B/C"): `RV7` = Eddy's "Trimpot A", `RV8` = Eddy's "Trimpot B", `RV6` = Eddy's "Trimpot C" (High Frequency Tracking/Linearity). `RV9` (distortion) and `RV10` (symmetry) belong to this build's sine converter and aren't part of Eddy's original tuning procedure (see step 6).

0. **One-time HF trim**: leave `RV6` (Trimpot C, HF Tracking/Linearity) centered. Per Eddy: it's "not really effective for the lower octaves" and its effect is minimal ("only a 1Hz change in high octaves") - it's a single-turn, set-and-forget trim, not part of the iterative tuning below.
1. **Setup**: disengage the Coarse/Octaves control, set the Fine-Tune pot to its middle position. Turn `RV7` and `RV8` all the way to one side until they click at the end stop. Then set `RV7` to about 3/4 of its travel (~22 of 30 turns) and `RV8` to about 1/4 of its travel (~7 turns).
2. **Iterate**: press key C5 and adjust `RV7` until C5 reads in tune on a tuner. Then press key C2 and adjust `RV8` until C2 reads in tune.
3. **Repeat** step 2 (C5/`RV7`, then C2/`RV8`) - each pass should need smaller corrections than the last, converging within about 15 minutes.
4. Aim for tuning accuracy to within about 1/10 Hz; no need for more than one decimal digit of precision.
5. Recommended tool: "Universal Tuner" by Dmitry Pogrebnyak (free, Android). Airyware Tuner is mentioned as a paid alternative.
6. **Sine shape** (this build only): with the module warmed up, watch SINE OUT on the scope. Adjust `RV10` (symmetry) until the positive and negative half-waves look identical, then `RV9` (distortion) for the roundest sine without flattened peaks. Repeat once, as the two interact slightly.

## Test Points

All on the main board. `TP8` is a wire loop for the scope's ground clip.

| Ref | Net | Expected | Use |
|-----|-----|----------|-----|
| TP5 | +12V | ~+12V (minus protection diode drop) | Bring-up: rails |
| TP6 | -12V | ~-12V (minus protection diode drop) | Bring-up: rails |
| TP7 | VEE (U3 pin 3) | negative, set by R32=680R from -12V | Bring-up: check before anything else on the AS3340 |
| TP8 | GND | 0V | Ground clip / meter reference |
| TP1 | VPWM (U3 pin 5) | 0..~5V, set by PWM OFFSET/PWM CV | PWM stage |
| TP2 | VP (U3 pin 4) | raw pulse output | Before the square output stage |
| TP3 | VSO (U3 pin 8) | raw saw output | Before the saw output stage |
| TP4 | VTO (U3 pin 10) | raw triangle output | Before the triangle output stage; also the buffered view of the timing capacitor |
| TP9 | U1D output (Q1 emitter) | ~1.6V DC | Reset clamp level: the timing capacitor is pulled to this voltage on reset |

Deliberately no test points on CAP, VFCI, VLFI or VSSI: these are high-impedance nodes, and a scope probe (~15pF) would visibly shift the pitch or change the signal. Use TP4 to observe the cap waveform.

## I/O Specification

Fill in the "Design" columns before building; fill in the "As-Built" columns after assembly and bench testing.

### Inputs

| Ref | Label | Signal Type (Design) | Expected Behavior / Range (Design) | Measured Behavior / Range (As-Built) | Notes |
|-----|-------|-----------------------|--------------------------------------|-----------------------------------------|-------|
| J1 | 1V/O IN | CV | +/-12V eurorack CV, DC-coupled | | DC-coupled via R6=100k directly into VFCI (chip abs-max +/-6V, VFCI is a current summing node). No clamp diodes (unlike SYNC IN). Worst case -12V => ~120uA, within safe range - oscillator just bottoms out, no damage expected. Combined with coarse/fine tune pots at full CCW: ~-160 to -220uA total into VFCI, still ~200x below the chip's 40mA/pin abs max. Decision: no clamp diodes added - all signal sources in this system are self-built/trusted, never exceed +/-12V. Revisit if this module is ever exposed to external/untrusted gear. |
| J2 | SYNC IN | audio or CV | absolute voltage is not important (AC coupled), depending on SW1, rising or falling edges are detected. | | |
| J3 | FM IN | CV | +/-12V eurorack CV, AC-coupled | | Attenuated by control-board RV5 (FM CV LVL), then AC-coupled (main-board C5=100nF + R11=1M) into VLFI. No clamp diodes. Static DC blocked by C5 in steady state; worst-case transient current is negligible. Decision: no clamp diodes added - all signal sources in this system are self-built/trusted, never exceed +/-12V. Revisit if this module is ever exposed to external/untrusted gear. |
| J4 | PWM CV IN | CV | +12V: constant DC output, -12V: constant DC output, 0V: 50% duty cycle square wave | | Verified against schematic (U1A/U1B math, assuming control-board RV4 PWM CV LVL pot at max and RV3 PWM OFFSET pot centered): VPWM = 0.2136*(Vcc+V_J4). -12V => 0.000V exactly (0% duty), 0V => 2.563V (~51% duty), +12V => 5.126V (saturates >100%). Matches spec at full CV depth; at partial depth or with manual offset applied, +/-12V won't reach the saturation points. |
| J5 | RESET IN | | | | |

### Outputs

| Ref | Label | Signal Type (Design) | Expected Behavior / Range (Design) | Measured Behavior / Range (As-Built) | Notes |
|-----|-------|-----------------------|--------------------------------------|-----------------------------------------|-------|
| J8 | SQUARE OUT | audio or CV | +/-5V | | |
| J9 | SAW OUT | audio or CV | +/-5V | | |
| J10 | TRI OUT | audio or CV| +/-5V| | |
| J11 | SINE OUT | audio or CV| +/-5V| | |

