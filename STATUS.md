# Module Status

What is actually on my bench, as opposed to what is in KiCad. Update the table whenever a module moves to a new stage.
Only the eurorack modules are tracked here, not the KOSMO ones.

## Current State

| Module | Stage | Breadboard | Next step | Notes | Updated |
|--------|-------|------------|-----------|-------|---------|
| #01 AS3340 VCO | 5 Ordered | Done | Bring-up when PCBs arrive | Tagged `01-rev1` | 2026-11-24 |
| #02 Loop Env | 5 Ordered | Done (gate and loop mode) | Bring-up when PCBs arrive | Tagged `02-rev1` | 2026-11-24 |
| #03 Quad VCA Mixer | 5 Ordered | Skipped: similar circuits proven in KOSMO modules, simple operation | Bring-up when PCBs arrive: trim CV Scale to unity, null CV Bleed | Tagged `03-rev1`. README I/O table is stale | 2026-11-24 |
| #04 Diode Ladder VCF | 2 Circuit | Not yet | Finish the schematic, then decide what to breadboard | PCB file only holds unrouted footprints | 2026-11-24 |
| #05 MIDI to CV | 8 In use | Done | None | One conductor, one drum velocity player and two melodic players built and tested in the 84HP Eowave case. Rev2 notes in its README | 2026-11-24 |
| #06 Output | 5 Ordered | Skipped: similar circuits proven in KOSMO modules, simple operation | Bring-up when PCBs arrive: test both JP3 positions | Tagged `06-rev1` | 2026-11-24 |

## Stages

Every module goes through these stages in order. A stage is done when its gate is met.

| # | Stage | Gate (done when...) |
|---|-------|---------------------|
| 1 | Concept | One paragraph on what it does, the HP and the panel controls. The "Design" columns of the I/O table are filled in. |
| 2 | Circuit | The schematic is based on a proven reference circuit where possible, and the netlist has been exported. Optionally simulated. |
| 3 | Breadboard | The breadboarded parts work with real parts on real ±12V, and key measurements are in the README. Anything not breadboarded has a recorded reason (see below). **No PCB layout before this gate.** |
| 4 | PCB | Layout passes DRC and schematic parity, it has been reviewed against the "PCB Design and Build Notes" in `README.md`, and the module README matches the netlist. |
| 5 | Ordered | Tagged `<module>-rev<N>` at the moment of ordering. From here on, collect notes for the next revision instead of redesigning. |
| 6 | Bring-up | Assembled, rails measured at the far end of every bus, trimmers calibrated. |
| 7 | Panel | Designed after the PCBs have arrived, then printed and fitted. |
| 8 | In use | The "As-Built" columns are filled in, and the module has been used to make music. Problems go into a list for the next revision, not straight into KiCad. |

## Breadboarding Rules

- A breadboard build doesn't have to cover the whole schematic. Decide which blocks are new or risky (e.g. a new core,
  an unusual topology, a part I haven't used before) and breadboard those. Proven blocks, such as power protection,
  input buffers and output stages already used elsewhere in the project, can be left out.
- Skipping a block, or the whole breadboard stage, is allowed only after discussing it. Write down the reason in the
  "Breadboard" column above, or in the module's README.
- Test the real part where possible. Breakout boards cover SMD ICs up to 20 pins (1.27mm and 0.65mm pitch). THT
  versions of many ICs (AS3340, TL074, LM13700, ...) are left over from the KOSMO build.
