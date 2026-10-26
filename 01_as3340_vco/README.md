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

### Why the reset also sets the charge direction

No hard-sync pulse is injected into pin 6 alongside the reset, because pulling the cap down
leaves the core charging on its own. Reading the block diagram on page 2 of `AS3340.pdf`,
zoomed on the core: both SPDT switch blocks on the cap node share one control line, driven by
the comparator at the bottom left. That comparator's `−` input is the node pin 6 (`VHSI`) sits
on, pulled to −1.3 V through 6.3 kΩ. Its `+` side comes from the buffered cap voltage through
the 14.4 kΩ / 7.2 kΩ network — and the right-hand switch block sits *inside* that network.
That is the hysteresis path: **the state selects which threshold is live.** Charging → the
upper threshold (5 V). Discharging → the lower one (0 V). So:

 - core was **discharging** → live threshold is 0 V, the cap crosses it, the comparator flips
   to charge;
 - core was **already charging** → live threshold is 5 V, and moving *down* crosses nothing,
   so the state simply holds.

Both land in the charge state, so the ramp always restarts upward — the reset is
phase-deterministic, not just level-deterministic.

It reliably crosses in the discharging case because while discharging the core is *sinking*
from the node, so the core and Q3 pull the same way and the node cannot stall above the
threshold. After the flip the core *sources*, and the node settles at V_ce(sat) + I_charge·R51:
about 80 mV at the very top of the frequency range, 10–20 mV at normal audio settings. That sits
above the lower threshold, but the live threshold is now the upper one, so the state stays
latched and does not chatter. The ramp restarts 0.2–1.6 % up the triangle rather than at exactly
zero, which matters for neither sync nor LFO phase.

Saw and pulse follow automatically: the tri-to-saw converter is driven by the triangle and the
same state line, and the pulse output is the PWM comparator against the triangle.

The real failure mode here is **pulse length, not direction**. If the pulse ends before the node
has been dragged down and the core happened to be discharging, it just finishes the descent it
was already making — which is why C10 is sized for ~15 µs against the 10 nF cap's ~5 µs.

**Caveat — this is inferred, not measured.** The switch blocks, the comparator and the pin-6
injection are all drawn explicitly, and the 0 V / 5 V triangle levels come from the datasheet's
spec table. But *which threshold is live in which state* is my reading of the hysteresis
topology; it is standard for a triangle core, yet stated nowhere in the datasheet. See the bench
test below.

### Things to be aware of

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

### Knock-on changes

To make room for the reset circuitry, **U1 was changed from a TL072 (SOIC-8) to a TL074
(SOIC-14)** — every op-amp section in the module was already in use. U1A and U1B keep the same
pin numbers, only the power pins move (V+ 8 → 4, V− 4 → 11). U1C is the reset comparator and U1D
is tied off as a grounded follower. The module now uses two TL074s and no TL072.

The **schematic sheet was enlarged from A4 to A3** — the A4 sheet had no free area left that
could hold the new block without overlapping the title block.

## To verify on the bench

 1. **Does the reset really force the charge direction? (UNVERIFIED — see above.)** Scope TP5
    (VTO), trigger `RESET IN` asynchronously, and deliberately catch resets landing in the
    **falling** phase as well as the **rising** phase. Both must snap toward 0 and head upward.
    The falling-phase case is the one that exercises the comparator flip; if the core ever comes
    out of a reset still heading *down* for a full segment, the reasoning above is wrong.
    Contingency if it is: a cap + resistor from U1C's output into `Net-(D1-A)` — the D1 anode,
    i.e. the positive-only sync path — forces the charge state directly. It has to go there and
    **not** straight to pin 6, because the trailing negative edge on the bipolar path would
    force *discharge*.
 2. Trim C10 (150–470 pF) for a 10–20 µs hold at Q3's base.
 3. Measure 1V/oct tracking before and after fitting Q3, then re-trim TP2 (initial frequency)
    and TP1 (HF tracking). Expect a small constant pitch offset; the scale error itself should
    not degrade.
 4. Check the triangle's rise/fall symmetry at the slowest LFO setting — that is where Q3's
    collector leakage would show up.

## Still to do after these schematic changes

The netlist and the board have **not** been regenerated — neither can be produced outside KiCad:

 1. Open the schematic and run ERC.
 2. `Tools > Update PCB from Schematic` to pull in J11, Q3, D5, D6, C10, R51–R58 and the
    SOIC-8 → SOIC-14 change on U1, then re-layout that area and place J11 on the panel side
    (the region around x 8–37, y 55–90 is free). Board stays 60 x 108 mm.
 3. `File > Export > Netlist` (KiCad format) to refresh `01_as3340_vco.net` — until then it
    still describes the old design and `tools/collective_bom.py` will report the old BOM.
 4. Add the RESET hole to the panel.
