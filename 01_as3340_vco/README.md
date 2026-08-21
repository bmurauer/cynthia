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
 - I have added a **hard reset input**. The AS3340's hard sync (pin 6) does not reset the
   timing capacitor — it only flips the internal charge/discharge state, so the ramp reverses
   direction from wherever it happens to be (see the "Output Waveforms" figure on page 3 of
   `AS3340.pdf`: the triangle turns around mid-slope and the sawtooth gets a mid-ramp notch).
   That is fine as a sync flavour but useless as an LFO restart. The reset input instead pulls
   the timing cap to 0 V through a transistor, so tri/saw/square all restart from a defined
   phase. It works in both switch positions (820 pF and 10 nF).

## Reset input

`RESET IN` (J11) → D6 (reverse-polarity block) → 100 k pulldown → U1C, a comparator with a
2.09 V threshold (R57/R58, the same 100 k/22 k divider used in module 02) and hysteresis via
R56. Trip points at the jack are roughly **3.2 V rising / 2.3 V falling**, so any normal 5 V or
10 V trigger fires it and noise does not.

The comparator's output edge is differentiated by R52/C10 into the base of Q3, which shorts the
timing cap to ground through R51 (100 Ω). Because the comparator delivers a clean fixed-amplitude
edge, the pulse width no longer depends on the trigger's amplitude or slew. With R52 = 10 k and
C10 = 330 pF the discharge is held for roughly 15 µs — long enough for the 10 nF LFO cap
(5τ ≈ 5 µs) and short enough to be a small fraction of an audio cycle. **Trim C10 in the
150–470 pF range on the bench to land at 10–20 µs.** C10 must be C0G/NP0.

D5 clamps Q3's base against the comparator's negative swing (the BC847's B-E reverse breakdown
is only ~6 V); R53 bleeds leakage and defines the base at rest.

Things to be aware of:

 - Q3's collector capacitance (~1.5–4 pF) is permanently across the timing cap. On 820 pF that
   is a ~0.2–0.5 % constant pitch offset, which the tune trimmer absorbs, plus 1–2 cents of
   tracking error from its voltage dependence. Keep the Q3 → R51 → pin 11 trace short with no
   plane under it, and re-trim TP2 and TP1 after fitting Q3.
 - Collector leakage adds to the exponential current. Irrelevant at audio; at the slow end of
   LFO mode it shows up as a few percent rise/fall asymmetry.
 - Each reset clicks — the pulse output flips and the saw drops to 0. That is inherent to any
   true hard reset.
 - If the tracking penalty ever turns out to matter, moving R51 from the CAP pin to `Net-(SW2-A)`
   (C15's switched terminal) disconnects Q3 entirely in audio mode, at the price of LFO-only reset.

To make room for the reset circuitry, **U1 was changed from a TL072 (SOIC-8) to a TL074
(SOIC-14)** — every op-amp section in the module was already in use. U1A and U1B keep the same
pin numbers, only the power pins move (V+ 8 → 4, V− 4 → 11). U1C is the reset comparator and U1D
is tied off as a grounded follower. The module now uses two TL074s and no TL072.

The **schematic sheet was enlarged from A4 to A3** — the A4 sheet had no free area left that
could hold the new block without overlapping the title block.

## Still to do after these schematic changes

The netlist and the board have **not** been regenerated — neither can be produced outside KiCad:

 1. Open the schematic and run ERC.
 2. `Tools > Update PCB from Schematic` to pull in J11, Q3, D5, D6, C10, R51–R58 and the
    SOIC-8 → SOIC-14 change on U1, then re-layout that area and place J11 on the panel side
    (the region around x 8–37, y 55–90 is free). Board stays 60 x 108 mm.
 3. `File > Export > Netlist` (KiCad format) to refresh `01_as3340_vco.net` — until then it
    still describes the old design and `tools/collective_bom.py` will report the old BOM.
 4. Add the RESET hole to the panel.
