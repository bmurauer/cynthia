# #06: Output

Stereo output module: takes two eurorack-level signals and provides impedance-balanced 6.35mm line outputs and a
3.5mm headphone output with a volume control.

Status: rev 1 PCBs laid out, not ordered or built yet. The panel will be designed once the PCBs have arrived.

## I/O Specification

Fill in the "Design" columns before building; fill in the "As-Built" columns after assembly and bench testing.

### Inputs

| Ref | Label | Signal Type (Design) | Expected Behavior / Range (Design) | Measured Behavior / Range (As-Built) | Notes |
|-----|-------|-----------------------|--------------------------------------|-----------------------------------------|-------|
| J1 | Input Left | | | | Front board. Grounded when unplugged. AC-coupled, 100k input impedance. |
| J3 | Input Right | | | | Front board. Normalled to J1, so a plug in J1 alone feeds both channels. AC-coupled, 100k input impedance. |

### Outputs

| Ref | Label | Signal Type (Design) | Expected Behavior / Range (Design) | Measured Behavior / Range (As-Built) | Notes |
|-----|-------|-----------------------|--------------------------------------|-----------------------------------------|-------|
| J2 | Line Out Left | | | | Back board. 6.35mm TRS, impedance-balanced. Not affected by the volume pot. |
| J5 | Line Out Right | | | | Back board. 6.35mm TRS, impedance-balanced. Not affected by the volume pot. |
| J4 | Headphones Out | | | | Front board. 3.5mm TRS, tip = left, ring = right. Level set by RV2. |

## Circuit

Each channel runs through the same stages. Left is shown, with right in brackets:

1. **Input:** C3 (C10) and R11 (R12) AC-couple the input. With 1µF and 100k, the corner is about 1.6Hz. U1 buffers the
   result and sends it across both boards as `LEFT` (`RIGHT`).
2. **Line output:** R3 and R4 (R7 and R8) attenuate by 47k / 4.7k, about 1/11 or -20.8dB, so 10Vpp becomes about 0.9Vpp.
   U3 then buffers the signal.
    - The tip is driven through R6 (R9), 100R.
    - The ring goes to GND through R5 (R10), 100R. Both legs therefore see the same impedance, which is what a balanced
      input needs to reject common-mode noise. A TS plug simply grounds the ring, which is harmless.
3. **Headphone output:** R15 (R14), 100k, feeds one gang of the 50k log pot RV2, so the maximum level is 1/3, or about
   -9.5dB. U2 (NJM4556A) buffers the wiper and drives the jack through R2 (R1), 10R. The series resistor keeps the total
   resistance constant, so the pot's taper is unchanged.

RV2's signal goes to pins 3 and 6 and its GND to pins 1 and 4, so that clockwise turns the volume up. This is the same
as the other level pots in the project. The mounting tabs (`MP`) are grounded, because the printed panel doesn't ground
the pot's frame.

## Boards

The module is split into two boards, each its own KiCad project, because the 6.35mm jacks are much taller than the
other panel parts:

- `front_board/` (19 x 63mm): J1, J3, J4, RV2 on the panel side; U1, U2 and their parts on the rear side.
- `back_board/` (19 x 100mm): power header and protection, J2, J5, U3 and its parts.

Both boards are aligned at their top and left edges. They connect through two headers: pin headers on the rear of the
front board, sockets on the front of the back board.

| Pin | Header A (top) | Header B (bottom) |
|-----|----------------|-------------------|
| 1 | GND | +12V |
| 2 | LEFT | GND |
| 3 | GND | GND |
| 4 | RIGHT | GND |
| 5 | GND | -12V |

Only the back board has reverse-polarity protection and bulk capacitors. The front board gets its power through
header B, so the rails should also be measured there during bring-up.

## Mechanics

Heights are measured from the PCB surface to where each part meets the back of the panel:

| Part | Height | Notes |
|------|--------|-------|
| J4 (stereo jack) | 12mm | Sets the panel height for the front board |
| J1, J3 (Thonkiconn) | 10mm + 2mm spacer | Printed spacer; the pins stick out less on the other side |
| RV2 (Alpha RV112BF-40E1) | 7.5mm | The thread ends at 12.5mm, so there is no room for a nut |
| J2, J5 (6.35mm jacks) | about 26mm | Seated on the shoulders of their pins |

**Board spacing:** 26mm - 12mm - 1.6mm PCB = 12.4mm. A standard socket and header stack is about 11mm, so the header
pins sit about 1.4mm short of fully seated. This also gives the stack some room to adjust when the jack nuts are tightened.

**Pot mounting:** RV2 is held only by the PCB, so solder the tabs fully. Plans for the panel:

- Pot hole of 9.5mm, because the bushing reaches 0.5mm into the panel.
- A sleeve of about 4.2mm around that hole on the back of the panel, with a bore of about 9.2mm. It supports the
  bushing sideways and stops the pot from lifting when a knob is pulled.
- RV2 has a D-shaft, so the knob can't be re-indexed. Its pins face downwards so that the knob pointer lines up.
