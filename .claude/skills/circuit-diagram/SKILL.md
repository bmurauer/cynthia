---
name: circuit-diagram
description: Draw circuit schematics and block diagrams for this synth project as circuitikz/TikZ, rendered to SVG. Use whenever a circuit, sub-circuit, test setup or signal flow should be visualized (instead of ASCII art or schemdraw), e.g. explaining a module, planning a breadboard test, or documenting a design in a README.
---

# Circuit diagrams with circuitikz

The user finds ASCII schematics hard to read. Any time a circuit is worth
showing, draw it with circuitikz, render it to SVG, check the render visually,
and only then show or reference it.

## Where things go
- Source: `<module>/drawings/tikz/<name>.tex` (one standalone document per drawing)
- Render: `tools/render_tikz.sh <file.tex|folder>` → `<name>.svg` next to the source,
  intermediate files are cleaned up. Commit `.tex` and `.svg`.
- Embed in the module README with `![...](drawings/tikz/<name>.svg)`.

## Content conventions
- **Use the real reference designators, values and IC pin numbers** from the
  module's netlist: `tools/netlist.py <module> --ref U5 R17` (see its `--help`).
  Never invent designators when a netlist exists; mention when a drawing shows
  a proposal that is not in the schematic yet.
- Pin numbers: small grey labels next to the pin (`pin` style in the template).
- Net names that leave the drawing (e.g. `CAP_BUF`, `S`) as plain text at a wire end.
- Short notes (formulas, warnings like "never connect pin 16 to GND directly")
  as `\scriptsize` text nodes near the relevant part.
- Rails are nominal ±12 V (real rails are lower; don't annotate that).

## Toolchain facts (this machine)
- pdflatex + circuitikz 1.8.5 + standalone are installed; **siunitx is not**:
  write units by hand (`56\,k$\Omega$`, `100\,nF`, `1\,M$\Omega$`).
- No OTA symbol: draw an LM13700 as `op amp` labelled "LM13700 (OTA)" and take
  I_ABC from the `down` anchor.
- White background is mandatory (dark-mode previews): `show background rectangle`
  with `background rectangle/.style={fill=white}` (in the template).

## Template
```latex
% <what this shows>. Reference designators follow <module>.net.
% Render: tools/render_tikz.sh <module>/drawings/tikz/<name>.tex
\documentclass[border=8pt]{standalone}
\usepackage[RPvoltages]{circuitikz}
\usetikzlibrary{backgrounds,calc}
\ctikzset{resistors/scale=0.8, capacitors/scale=0.8, diodes/scale=0.7}
\begin{document}
\begin{circuitikz}[font=\sffamily\small, show background rectangle,
                   background rectangle/.style={fill=white}]
  \tikzset{pin/.style={font=\sffamily\scriptsize, text=gray}}
  \node[op amp, noinv input up] (u) at (0,0) {\scriptsize U4B};
  \node[pin, above left] at (u.+) {5};
  \draw (u.+) -- ++(-1,0) to[R, l=R9 10\,k$\Omega$, -*] ++(-2,0) coordinate (n);
  \draw (n) to[C, l_=C11 100\,nF] ++(0,-2) node[ground]{};
\end{circuitikz}
\end{document}
```
Useful parts: `R`, `vR` (pot/rheostat), `C`, `D`, `short, -*` (junction dot),
`-o` (open terminal), `node[ground]`, `node[vcc]{+12\,V}`, `op amp`
(`noinv input up` puts + on top; anchors `+ - out up down`), `spdt`
(anchors `in`, `out 1`, `out 2`; `xscale=-1` to mirror), `npn`/`pnp`
(anchors `B C E`).

## Layout rules (learned the hard way)
- Lay out on an explicit grid: main signal path left→right on one row, supply
  and feedback paths above/below; logic in a separate row underneath.
- A `coordinate`/node must be defined before a later line uses it; put notes
  that refer to late coordinates at the end of the file.
- Keep at least ~2.5 units between parallel vertical branches, otherwise labels
  collide. Choose `l=` vs `l_=` to put a label on the free side.
- Route long chains (e.g. a resistor string to ground) sideways rather than
  through another branch's area.

## Verify before showing
Render, convert to PNG and look at it:
```
tools/render_tikz.sh <file.tex>
rsvg-convert -w 1500 <file.svg> -o <scratchpad>/check.png   # then Read the PNG
```
Fix overlapping labels, wires crossing components, and dangling or unintended
junctions before presenting the drawing. Iterate until it is clean.
