#03: Quad VCA Mixer

## I/O Specification

Fill in the "Design" columns before building; fill in the "As-Built" columns after assembly and bench testing.

Jack labels in the netlist are generic ("CV in" / "signal in" without channel numbers). Based on net tracing, J2–J5 appear to form channels 1–2 and J6–J9 channels 3–4 — double-check this grouping against the schematic before relying on it.

### Inputs

| Ref | Label | Signal Type (Design) | Expected Behavior / Range (Design) | Measured Behavior / Range (As-Built) | Notes |
|-----|-------|-----------------------|--------------------------------------|-----------------------------------------|-------|
| J2 | CV in | | | | channel 1 (inferred) |
| J3 | CV in | | | | channel 2 (inferred) |
| J4 | signal in | | | | channel 1 (inferred) |
| J5 | signal in | | | | channel 2 (inferred) |
| J6 | CV in | | | | channel 3 (inferred) |
| J7 | CV in | | | | channel 4 (inferred) |
| J8 | signal in | | | | channel 3 (inferred) |
| J9 | signal in | | | | channel 4 (inferred) |

### Outputs

| Ref | Label | Signal Type (Design) | Expected Behavior / Range (Design) | Measured Behavior / Range (As-Built) | Notes |
|-----|-------|-----------------------|--------------------------------------|-----------------------------------------|-------|
| J1 | Signal out | | | | mixed output of all 4 channels |
