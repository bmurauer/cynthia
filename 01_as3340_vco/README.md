#01: AS3340 VCO

This design is based on the [design from Eddy Bergman](https://www.eddybergman.com/2020/01/synthesizer-build-part-18-really-good.html), which is based on a DigiSound 80 VCO.

I have made the following modifications to it:

 - I have included a switch that changes the capacitor to a larger one, allowing for mad slow LFO action.
 - the output stages are modified so that the VCO roughly produces -5V/+5V outputs (instead of 0/+10V).
 - the design includes a sine wave output. This is based on Eddy Bergmans Triangle to Sine converter.
 - the triangle wave has a dedicated centering trimmer, as I thought that centering the signal is essential for the sine wave conversion - I think if you measure your resistors a bit, you should be able to do without the trimmer.
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

**Trimmer mapping** (per this board's own KiCad component descriptions, "Fine Tune A/B/C"): `TP2` = Eddy's "Trimpot A", `TP3` = Eddy's "Trimpot B", `TP1` = Eddy's "Trimpot C" (High Frequency Tracking/Linearity). `TP4` is this build's own addition (the triangle-centering trim mentioned above) and isn't part of Eddy's original tuning procedure.

0. **One-time HF trim**: leave `TP1` (Trimpot C, HF Tracking/Linearity) centered. Per Eddy: it's "not really effective for the lower octaves" and its effect is minimal ("only a 1Hz change in high octaves") - it's a single-turn, set-and-forget trim, not part of the iterative tuning below.
1. **Setup**: disengage the Coarse/Octaves control, set the Fine-Tune pot to its middle position. Turn `TP2` and `TP3` all the way to one side until they click at the end stop. Then set `TP2` to about 3/4 of its travel (~22 of 30 turns) and `TP3` to about 1/4 of its travel (~7 turns).
2. **Iterate**: press key C5 and adjust `TP2` until C5 reads in tune on a tuner. Then press key C2 and adjust `TP3` until C2 reads in tune.
3. **Repeat** step 2 (C5/`TP2`, then C2/`TP3`) - each pass should need smaller corrections than the last, converging within about 15 minutes.
4. Aim for tuning accuracy to within about 1/10 Hz; no need for more than one decimal digit of precision.
5. Recommended tool: "Universal Tuner" by Dmitry Pogrebnyak (free, Android). Airyware Tuner is mentioned as a paid alternative.

## I/O Specification

Fill in the "Design" columns before building; fill in the "As-Built" columns after assembly and bench testing.

### Inputs

| Ref | Label | Signal Type (Design) | Expected Behavior / Range (Design) | Measured Behavior / Range (As-Built) | Notes |
|-----|-------|-----------------------|--------------------------------------|-----------------------------------------|-------|
| J3 | SYNC IN | audio or CV | absolute voltage is not important (AC coupled), depending on SW1, rising or falling edges are detected. | | |
| J4 | FM IN | CV | +/-12V eurorack CV, AC-coupled | | AC-coupled (C5+R6=1M) into VLFI. No clamp diodes. Static DC blocked by C5 in steady state; worst-case transient current is negligible. Decision: no clamp diodes added - all signal sources in this system are self-built/trusted, never exceed +/-12V. Revisit if this module is ever exposed to external/untrusted gear. |
| J5 | PWM CV IN | CV | +12V: constant DC output, -12V: constant DC output, 0V: 50% duty cycle square wave | | Verified against schematic (U1A/U1B math, assuming RV3 PWM-depth pot at max and RV1 manual-PWM pot centered): VPWM = 0.2136*(Vcc+V_J5). -12V => 0.000V exactly (0% duty), 0V => 2.563V (~51% duty), +12V => 5.126V (saturates >100%). Matches spec at full CV depth; at partial depth or with manual offset applied, +/-12V won't reach the saturation points. |
| J6 | 1V/O IN | CV | +/-12V eurorack CV, DC-coupled | | DC-coupled via R3=100k directly into VFCI (chip abs-max +/-6V, VFCI is a current summing node). No clamp diodes (unlike SYNC IN). Worst case -12V => ~120uA, within safe range - oscillator just bottoms out, no damage expected. Combined with coarse/fine tune pots at full CCW: ~-160 to -220uA total into VFCI, still ~200x below the chip's 40mA/pin abs max. Decision: no clamp diodes added - all signal sources in this system are self-built/trusted, never exceed +/-12V. Revisit if this module is ever exposed to external/untrusted gear. |

### Outputs

| Ref | Label | Signal Type (Design) | Expected Behavior / Range (Design) | Measured Behavior / Range (As-Built) | Notes |
|-----|-------|-----------------------|--------------------------------------|-----------------------------------------|-------|
| J7 | SQUARE OUT | audio or CV | +/-5V | | |
| J8 | SAW OUT | audio or CV | +/-5V | | |
| J9 | TRI OUT | audio or CV| +/-5V| | |
| J10 | SINE OUT | audio or CV| +/-5V| | |

