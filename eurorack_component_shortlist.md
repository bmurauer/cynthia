# Eurorack Build — Component Ordering Reference

A consolidated shortlist of the recommendations from our discussion. Organized by category, with the recommended option first and notes on what to avoid. "Workhorse" = the default safe choice; alternatives noted where they matter.

---

## Op-amps (all SOIC-8 narrow body, `SOIC-8_3.9x4.9mm_P1.27mm`)

| Role | Recommended part | Notes |
|---|---|---|
| General dual op-amp (VCA, buffers) | **TL072CDR** (TI, SOIC, commercial temp, reel) | JFET input. `TL072ACDR` if you want tighter offset — rarely needed. |
| General quad op-amp | **TL074CDR** | Same logic as TL072. |
| Ground-sensing dual (envelope comparator/integrator, LM324 replacement) | **LM358BDR** (TI, B-grade) | Or plain **LM358DR** for cheapest. Both sense input down to the negative rail — the property you actually rely on. |
| OTA (random-voltage slew stage, VCAs) | **LM13700** SOIC (genuine TI) | DIP is EOL — see note below. |

**Op-amp rules of thumb:**
- Buy **C (commercial) temperature grade** — you'll never need I/M (industrial/military) headroom indoors. Don't pay for it.
- Skip **B spec-grade** on the TL07x unless a stage needs precision DC offset (none here do).
- **AVOID `LMV358` and any "LV" variant** — those are low-voltage (2.7–5.5 V) parts and will be destroyed on your ±12 V rails despite the similar name. This is the one real landmine.
- `LM2904` = automotive-temp LM358 sibling; electrically equivalent, no benefit for indoor use.
- `TL07xH` family = newer, pin-compatible, but slightly worse input noise (≈37 nV/√Hz) — not a strict upgrade.

**LM13700 sourcing (only relevant for the random-voltage / OTA project):**
- Genuine TI **SOIC** is active and the future-proof choice if going SMD.
- Genuine TI **DIP is end-of-life** → scarce and pricey (€5+ at reichelt).
- DIP clones that builders report working fine, from LCSC (~€0.25–0.35): **Xinluda XD13700**, HGSemi LM13700N, Hanschip LM13700PG.
- Modern alternative for new designs: **SSI2164** (currently-supported, different pinout/topology).

---

## Resistors

**Reputable brands:** Vishay/Dale, Yageo, KOA Speer, Panasonic, Susumu.

| Use | Type | Recommendation |
|---|---|---|
| Bulk / general (bias, non-critical feedback, pull-ups, LED current limit) | **Thick film** | Yageo or Vishay generic 1% 0805/1206. Brand barely matters. Buy in bulk per value. |
| Precision CV scaling (1 V/oct trackers, precise dividers) | **Thin film** | Vishay TNPW-series, 1% or tighter, low tempco. |
| Low-noise signal path, temp-critical (VCO expo converters) | **Thin film** | TNPW or KOA precision; ≤25 ppm/K matters here. |

- Default everything to **thick film 0805** and reserve thin-film only for the handful of precision spots your original design called out (often marked "1% metal film" in build docs).
- 0805 is the sweet spot for hand soldering; 1206 also fine.

---

## Capacitors

**MLCC brands:** Murata, TDK, KEMET, Samsung (SEMCO), Yageo.

| Use | Type | Through-hole vs SMD |
|---|---|---|
| Bulk power decoupling (e.g. 50 µF) | Electrolytic | **Through-hole** — no real SMD benefit, THT has better ESR per size, easier to replace as it ages |
| HF bypass at IC power pins (100 nF) | **X7R ceramic** | **SMD** 0805 — place right at the chip power pin |
| Small signal/filter caps (VCF, noise-shaping, gate-path e.g. 47 nF) | **C0G / NP0 ceramic** | **SMD** 0805/1206 — as good as or better than cheap film here |
| Envelope timing cap (≈1 µF) | **Film** (polypropylene) | **Through-hole** — stability/leakage critical; C0G this big is bulky/pricey; **never X7R here** |
| VCO core timing (AS3340) | **Polypropylene film** | **Through-hole** — see dedicated section below |

**Critical caps rule:** never put **X7R/X5R** in a signal, filter, or timing path (voltage coefficient + microphonics). X7R is *only* for power-rail decoupling. The signal-path ceramic is **C0G/NP0 only**, which tops out around 0.47–1 µF.

---

## VCO timing cap (AS3340) — special case

Polystyrene is the classic, but it's heat-fragile and increasingly scarce. **Polypropylene film is the modern equal-or-better substitute.**

| Option | Construction | When |
|---|---|---|
| **Vishay Roederstein KP1830** | Film/**foil** polypropylene | The "correct" timing-grade choice — lowest dielectric absorption, purpose-marketed for oscillator/timing. Worth the premium for a VCO core. |
| **TDK B32621 (MKP)** | **Metallized** polypropylene | Cheaper, still far better than ceramic; fine for most builds. |

- Foil construction (KP1830) is why it costs more — solid foil electrodes vs. sprayed-on metal, giving lower DA and better stability.
- **Get a tight tolerance: ±1 % to ±5 % (F or J suffix)** if you build more than one VCO or want repeatable tuning.

---

## Diodes (all 0805 SMD where applicable)

**Brands:** Vishay, onsemi, Diodes Inc, Nexperia.

| Part | Use |
|---|---|
| **1N4148** | General signal/steering (envelope charge/discharge paths) |
| **1N5819** | Schottky (fast clamp/rectify around VCA core) |
| Noise-source zener (e.g. 1N753A-type) | **Stick to a big-name brand** — breakdown-noise behavior is exactly where a sketchy clone misbehaves |

---

## Transistors

| Part | Use | Notes |
|---|---|---|
| **BC847** (SOT-23) | VCA differential pair, gate-trigger switching | SMD sibling of the BC547 you used in KOSMO. Already migrated in your latest netlist. |

---

## Eurorack jacks (3.5 mm)

Use the **vertical "Thonkiconn" style** so the PCB sits parallel to the panel (the horizontal jacks on Mouser are the wrong mechanical type for your layout).

| Part number(s) | Type |
|---|---|
| **PJ301M-12** / **PJ398SM** / **WQP518MA** | Mono, vertical, non-switched (interchangeable names, same footprint) |
| **PJ301BM** | Mono, **switched** (for normalled/auto-disconnect inputs — most CV/audio inputs want this) |
| PJ366ST / PJ-3410 | Stereo (TRS), if any module needs it |

- **Source from Thonk or Tayda** rather than Mouser — far cheaper and reliably stocked for these synth-specific parts. Same goes for panel pots.
- PCB note: leave a **3 mm clearance hole** (no traces/plane) under the jack barrel center.

---

## Potentiometers

| Value | Use | Mount |
|---|---|---|
| **1 M** | Envelope Attack/Decay timing | Panel, through-hole |
| **100 k trimmer** | VCA offset/CV-depth calibration | Board, through-hole |

Order alongside jacks from Thonk/Tayda.

---

## Power header

- **2×5 (10-pin) Eurorack power header**, 2.54 mm pitch, vertical (`PinHeader_2x05_P2.54mm_Vertical`). Standard Eurorack ±12 V/+5 V bus connector.

---

## Soldering consumables — IMPORTANT correction

| Item | Action |
|---|---|
| **Stannol Lötfett F-SW 21** (current acid flux) | **STOP using on electronics.** It's a zinc-chloride acid flux (DIN type 3.1.1.C) — corrosive, conductive residue. Keep it strictly for non-electronics metalwork. |
| **No-clean flux** (pen or gel) | Order this. Options: MG Chemicals no-clean, Amtech-style no-clean, Chip Quik SMD291 no-clean. Essential for drag-soldering SOICs and fixing bridges. |
| **99 % isopropyl alcohol** | For cleanup/inspection. |
| Rosin-core solder | Your existing wire solder is fine (flux core is already electronics-grade). |
| Solder paste | Optional — only if you move to hotplate/hot-air reflow. Perishable (refrigerate); skip for hand soldering. Get **no-clean** if you do. |

---

## Footprint conventions you've settled on
- Op-amps: `SOIC-8_3.9x4.9mm_P1.27mm` (shared by LM358 and TL072 — nice consistency)
- Passives: `0805 HandSolder`
- BC847: `SOT-23-3`
- Electrolytics, jacks, pots, power header: through-hole

---

## Suggested vendor split
- **Mouser:** ICs (genuine TI/Vishay/onsemi), precision/thin-film resistors, C0G & X7R ceramics, film caps, diodes, transistors, the sensitive stuff that needs traceability.
- **Thonk / Tayda:** Thonkiconn jacks, panel potentiometers, knobs, power headers, panels — cheaper and reliably stocked for synth-specific parts.
- **LCSC:** bulk generic passives, and LM13700 DIP clones if you go that route.
