# Module 04 — Steiner-Parker VCF: schematic review

Reference design: `Steiner-improved-sch.gif` (yusynth, "Steiner improved").
Reviewed against a netlist extracted from `04_diode_ladder_vcf.kicad_sch`.

**Verdict:** the *core* of the filter is a faithful port — ladder, mode switch, output
amplifier, resonance buffer and the CV current source all match the reference node for
node. There are four errors that will stop the module working, plus a handful of
smaller things worth deciding on. The PCB has no tracks and no imported netlist yet, so
everything below is cheap to fix now.

Note the reference designators were renumbered during the port, so nothing lines up by
name. The mapping used throughout is at the end of this document.

---

## What is correct

Verified against the reference, node by node:

- **Diode ladder** — 3 diodes, tap, 2, tap, 2, tap, 3, with the three 1.5 nF taps, the
  680 nF band-pass coupling cap and both 1.8 kΩ end resistors. Diode orientation is
  consistent and points the same way as the reference (cathodes toward Q1).
- **Mode switch** — both poles, all four positions. `SW1` pole 2 gives
  LP→bias node, BP→ladder rail, HP→R12 node, AP→R12 node; pole 1 adds the bias node in
  the AP position only. This is exactly the reference's `sw1a` / `SW1b` wiring.
- **Ladder output amp** (`U1C`) — 680 nF into the + input, 2.2 M to ground, 22 k/47 k
  around the − input for a gain of 3.14.
- **Output stage** (`U1B`) — 47 µF, 10 k, 220 k feedback, 1 k series to the jack.
- **Resonance buffer** (`U1D`) — pins 13/14 strapped as a unity buffer, 220 k into the
  + input, 1 M to ground, 1 k into the ladder rail. Identical to the reference `IC1b`.
- **CV current source** — differential pair, 220 Ω + 2 k trim at the base, 3k9 + 82 Ω
  tail to −12 V with 47 µF decoupling.
- **Electrolytic polarity** — every polarised cap is oriented correctly, with the one
  exception noted in issue 5.
- **Power header and reverse-protection diodes** — identical topology to modules 01 and
  02, so consistent with the rest of the repo.
- `RV5` (2 k resonance-range trim) is an addition over the reference. Good idea.

---

## Blocking issues

### 1. Ground is split into two unconnected nets

The schematic mixes `power:GND` (11 symbols) and `power:GNDREF` (12 symbols). KiCad
treats these as **different nets** — they will never be joined on the PCB.

| net | what is on it |
|---|---|
| `GND` | header pins 3–8, both input jacks, decoupling caps, `R7`, `R12`, `R15`, `U1` pin 3 |
| `GNDREF` | output jack sleeve, `Q2` base, all pot bottoms, `R17`, `R19`, `R21`, `RV3`, `C10`, `U1` pin 5 |

So the ladder's ground reference, the differential pair's reference and the output jack
sit on an island with no connection to the power header. Modules 01, 02 and 03 use
`GND` exclusively.

**Fix:** replace every `GNDREF` with `GND`.

### 2. `R3` (10 k) is tied to +12 V at the input amp's summing node

`R3` pin 1 has a `+12V` power symbol (`#PWR08`) wired directly to it at (36.83, 69.85).
With `R2` = 10 k in, `R6` = 10 k feedback and `U1` pin 3 grounded, `U1A` computes
`−(Vin + 12 V)` and sits pinned against the negative rail. No audio gets past the input
stage.

In the reference this resistor (`R4`) is the second input of a two-input mixer, fed
through `C4` from a second source.

**Fix:** either remove `R3`, or make it a real second input (jack → 10 µF → 10 k).

### 3. `R10` and `R11` are 180 k; they should be ≈390 Ω and 1 k

These feed the ladder's bias node from +12 V (`R10` → decoupled by `C15` 47 µF → `R11` →
`RV4` trim → the two 1.8 k end resistors). The reference uses **390 Ω** and **1 kΩ**.

At 180 k + 180 k the network can deliver about 33 µA, while the differential pair below
is asking for roughly 2.8 mA. The ladder collapses toward the negative rail and the
filter does nothing.

All four of `R7`, `R10`, `R11`, `R15` currently read 180 k, which looks like a value that
got pasted across the group — `R7` and `R15` are genuinely 180 k, `R10` and `R11` are not.

**Fix:** `R10` = 390 Ω, `R11` = 1 k.

### 4. Missing 47 k series resistor between `U1A` and the mode switch

The reference puts `R6` = 47 kΩ between the input amp's output and the two switch
commons. Here `U1` pin 1 connects straight to `SW1` pins 1 and 6.

This matters most in HP and AP mode, where the switch drives the `R12` node — a 1 kΩ
resistor to ground with no coupling cap. With the 47 k present the injected signal is
attenuated about 48×; without it the op-amp drives that node directly and the ladder is
overdriven by roughly the same factor. (In LP mode the error is only ~2 dB, because that
path lands on a 180 k pulldown.)

**Fix:** add a 47 k resistor in series between `U1` pin 1 and `SW1` pins 1/6.

---

## Should fix

### 5. `R15` (180 k) is on the wrong side of `C17`

In the reference, the 180 k pulldown (`R8`) sits on the **switch** side of the 2.2 µF
coupling cap, mirroring what `R7` does for `C16`. Here `R15` is on the **ladder-rail**
side instead, so `C17` pin 2 / `SW1` pin 8 is a two-pin net that floats whenever the
switch is not in the BP position.

Two consequences: charge builds up on the floating node and you get a thump when
switching modes, and `C17` is a polarised electrolytic whose bias is now undefined — it
can end up reverse-biased.

The rail side does not need the resistor: it is already DC-referenced through `R18` to
`U1D`'s output.

**Fix:** move `R15` so it goes from `SW1` pin 8 / `C17` pin 2 to ground.

### 6. Ladder diodes have no part number

`D1`–`D10` have Value `D` and footprint `cynthia:D SMD 0805`. Module 03 already uses
`Diode:1N4148WS` in `Diode_SMD:D_SOD-323_HandSoldering`, which is the right part and
package.

These ten diodes *are* the filter — their matching sets how symmetric the ladder is.
Buying them from one reel and, if you feel like it, sorting a handful by forward voltage
is worth the trouble here.

**Fix:** `1N4148WS`, SOD-323, same as module 03.

### 7. `Q1`/`Q2` should be one matched dual, not two singles

The reference explicitly calls for a matched pair (2SC1583, or two hand-matched BC547C)
because this differential pair is what converts the CV into an exponential current — the
two halves need to match and to track each other's temperature.

Two separate `BC848` in individual SOT-23 packages do neither. You already use
`Transistor_BJT:BC847BS` — a matched dual NPN in SOT-363 — for exactly this job in
module 03's VCA cells.

**Fix:** replace `Q1` + `Q2` with a single `BC847BS`.

---

## Worth a decision

8. **No V/oct input.** The reference has a dedicated one (`R31`, 220 k straight into the
   base summing node) alongside the two attenuated CV inputs. This port has a single
   attenuated CV in. If you want the filter to be playable as a sine source, add it.
9. **No input DC blocking.** The reference has 10 µF in series with each input. Since HP
   and AP mode inject into the ladder with no coupling cap, a DC offset from the
   preceding module will shift the ladder's operating point in those modes.
10. **Cutoff sweep is wider than the reference** — `RV2` 100 k + `R5` 100 k here versus a
    47 k pot + 150 k in the reference, which works out to roughly 9–10 octaves instead of
    ~6. Probably an improvement; just check the ends are usable on the bench.
11. **`RV6` is 100 k with unspecified taper**; the reference calls for 50 kΩ reverse-log.
    Taper matters a lot for how resonance feels near self-oscillation.
12. **`R13`/`R14` (3k9 / 82 Ω) are still sized for ±15 V.** On ±12 V the tail current
    drops from ~3.6 mA to ~2.85 mA, costing a little cutoff range at the top. Dropping
    `R13` to ~3 kΩ restores it, if you care.
13. **Specify C0G/NP0 for `C6`, `C8`, `C11` (1.5 nF).** These are the actual filter caps
    and your own component shortlist rules out X7R here. `C9` (680 nF) on a THT film
    footprint is the right call — C0G that large is not practical.
14. **Footprint library inconsistency**: `C1`/`C2` use `Capacitor_THT:CP_Radial_…` while
    every other electrolytic uses `cynthia:CP_Radial_…`. Cosmetic.

---

## Adding CV control of resonance

### Where to intervene

The resonance control in this circuit is nothing more than a variable attenuator. `U1C`'s
output goes through `R22` (3k9) and `RV5` into the top of `RV6`; the wiper is buffered by
`U1D` at unity gain and driven back into the ladder rail through `R18` (1 k). Maximum
feedback works out to about 0.78 of `U1C`'s output.

So: replace the passive attenuator with a VCA and you have voltage control. Putting it
**before** `U1D` rather than after is what keeps this simple — `U1D` goes on holding the
ladder rail at 0 V DC, which the ladder's operating point depends on.

### Recommended: LM13700 OTA ahead of `U1D`

The LM13700 is already the OTA on your component shortlist, and SOIC is the
future-proof package. Roughly one SOIC-16, one SOIC-8, a jack and ten passives.

**Signal path**

- `U1C` pin 8 → 1 µF film → 100 k → OTA A + input (pin 3).
- 220 Ω from pin 3 to ground, 220 Ω from pin 4 (− input) to ground.
  That is ≈1/455 attenuation, so a ±6 V swing at `U1C` arrives as about ±13 mV — inside
  the OTA's linear region, which means you can leave the linearising diodes (pin 2)
  unused.
- OTA A output (pin 5) → load resistor to ground → straight into `U1D` pin 12.
  Use 22 k fixed plus a 50 k trim; this is where you set the self-oscillation point, so
  `RV5` can be repurposed here.
- `U1D` stays exactly as it is: pins 13/14 strapped, `R18` 1 k into the ladder rail.
- `RV6`, `C14` and `R23` come out of the signal path.
- Power: pin 11 → +12 V, pin 6 → −12 V, 100 nF on each.

Sanity check on gain: transconductance is about `I_ABC / 52 mV`, so 500 µA gives
9.6 mA/V. Into 39 kΩ that is a voltage gain of ~375, times the 1/455 input attenuator
≈ 0.82 — which lands right on the ~0.78 the passive version delivers at full CW. Plenty
of headroom to reach and pass self-oscillation.

**Control path**

- `RESONANCE` pot (keep `RV6` on the panel) wired ground → +12 V, wiper → 100 k → summing
  node.
- `RES CV` jack (switched, normalled to ground) → 100 k → the same summing node. Add a
  second 100 k pot in front of it if you want per-patch depth.
- Summing node → a TL072 section as an inverting summer (100 k feedback), then a second
  section to fix polarity and set the offset.
- Output → 33 k → LM13700 pin 1.

One wrinkle: pin 1 sits about one V_BE above the negative rail (≈ −11.3 V), and a TL072
cannot pull quite that low, so the OTA never fully shuts off — you would be left with
roughly 25 µA of residual bias, i.e. a little resonance always present. Either accept
that (it is often what you want anyway), or drive pin 1 from a PNP current source:
+12 V → 12 k → emitter of a `BC857BS`, collector → pin 1, base ← the control op-amp.
The current source also gives cleaner scaling.

### Alternatives

- **Reuse the APM cell from module 03.** No new part types at all — `BC847BS`, `TL072`
  and `1N4148WS` are all already in your BOM, and you have already validated the block.
  Costs about twenty more passives and the board area, versus one extra IC.
- **Vactrol.** Cheapest, a handful of parts, no extra rails. But slow (millisecond-scale
  lag), poor unit-to-unit repeatability, and vactrols are getting hard to buy. Fine if
  you only want slow sweeps.
- **JFET as a voltage-controlled resistor** shunting the wiper. Fewest parts of all, but
  limited range and it adds distortion — which on a Steiner may well be a feature.

### Caveat

On a Steiner the resonance and cutoff interact: the self-oscillation threshold moves with
the ladder bias current. Expect CV'd resonance to feel somewhat level-dependent across
the cutoff range. That is inherent to the topology, not to this modification.

---

## Reference → KiCad designator map

| Reference | This schematic | | Reference | This schematic |
|---|---|---|---|---|
| `IC1a` input amp | `U1A` | | `IC1c` ladder amp | `U1C` |
| `IC1d` output amp | `U1B` | | `IC1b` res. buffer | `U1D` |
| `R3`, `R4`, `R5` | `R2`, `R3`, `R6` | | `R6` 47 k | **missing** |
| `R7`, `R8` 180 k | `R7`, `R15` | | `C5`, `C6` 2.2 µ | `C16`, `C17` |
| `R9`, `R13` 1.8 k | `R8`, `R16` | | `T2` 1 k trim | `RV4` |
| `R10` 390, `R11` 1 k | `R10`, `R11` | | `C7` 47 µ | `C15` |
| `C9`, `C10`, `C11` 1.5 n | `C6`, `C8`, `C11` | | `C12` 680 n | `C9` |
| `R14` 2.2 M | `R17` | | `R16` 22 k, `R17` 47 k | `R19`, `R20` |
| `C13` 47 µ | `C13` | | `R19` 10 k, `R20` 220 k | `R24`, `R25` |
| `R21` 1 k | `R26` | | `R18` 3.9 k | `R22` |
| `P5` 50 k | `RV6` 100 k | | `C14` 10 µ | `C14` |
| `R22` 220 k, `R23` 1 M | `R23`, `R21` | | `R15` 1 k | `R18` |
| `R28` 220, `T1` 2 k | `R9`, `RV3` | | `R29` 3.9 k, `R30` 82 | `R13`, `R14` |
| `C8` 47 µ | `C10` | | `R26` 47 k, `P3`, `R27` | `R1`, `RV2`, `R5` |
| `R24`/`R25` 220 k | `R4` (one CV only) | | `R31` V/oct | **missing** |
