#!/usr/bin/env python3
"""Print a KiCad netlist (``*.net``) in a compact, readable form.

One line per component, then one line per net listing every pin on it.
Pin functions from the symbol (``U5.14(+)``) are shown when available, so
op-amp/OTA pins can be read without opening the schematic.

Usage:
    tools/netlist.py 02                      # module by number (or folder name)
    tools/netlist.py path/to/board.net       # any netlist file
    tools/netlist.py 02 --ref U5 R17         # only nets touching these parts
    tools/netlist.py 02 --net BASE I_ABC     # only nets whose name contains these
    tools/netlist.py 02 --nets-only          # skip the component list
    tools/netlist.py 02 --connectors         # board-to-board header pins and their nets
    tools/netlist.py 02 --board main         # split module: only main_board.net

A module split into several boards (one KiCad project per board in subfolders)
prints every board's netlist in turn; reference designators are per board.

Short component descriptions (<= 30 chars, e.g. a pot's role "ATTACK") are
shown as comments; long ones are library boilerplate and hidden.

Netlists are exported from KiCad with ``File > Export > Netlist`` (KiCad
format).
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

from collective_bom import child, children, parse_sexp, value_of

ROOT = Path(__file__).resolve().parent.parent


def find_netlists(target: str) -> list[Path]:
    """A .net file, or every netlist of a module (one per board for split modules)."""
    path = Path(target)
    if path.is_file():
        return [path]
    for folder in sorted(p for p in ROOT.iterdir() if p.is_dir()):
        if folder.name == target or folder.name.startswith(f"{target}_"):
            preferred = folder / f"{folder.name}.net"
            if preferred.exists():
                return [preferred]
            found = sorted(p for p in folder.rglob("*.net")
                           if not any(part.endswith("-backups") for part in p.parts))
            if found:
                return found
            sys.exit(f"no netlist in {folder} - export one from KiCad first")
    sys.exit(f"no module or file matching '{target}'")


def natural_key(text: str):
    return [int(t) if t.isdigit() else t for t in re.split(r"(\d+)", text)]


def short_function(function: str) -> str:
    """KiCad appends the pin number ("+_14", "C1_6"); drop it."""
    return re.sub(r"_\d+$", "", function)


def print_netlist(path: Path, args) -> None:
    export = parse_sexp(path.read_text(encoding="utf-8"))
    print(f"# {path.relative_to(ROOT) if path.is_relative_to(ROOT) else path}")
    comps = children(child(export, "components") or [], "comp")
    nets = children(child(export, "nets") or [], "net")

    if args.connectors:
        # pin -> net for every connector (value "Conn_..."), to compare mating board-to-board headers
        pin_nets: dict[str, dict[str, str]] = {}
        for net in nets:
            for node in children(net, "node"):
                pin_nets.setdefault(value_of(node, "ref"), {})[value_of(node, "pin")] = value_of(net, "name")
        for comp in comps:
            ref, value = value_of(comp, "ref"), value_of(comp, "value")
            if value.startswith("Conn_"):
                pins = pin_nets.get(ref, {})
                print(f"{ref} ({value}): " + "  ".join(f"{p}={pins[p]}" for p in sorted(pins, key=natural_key)))
        print()
        return

    if not (args.nets_only or args.net):
        rows = []
        for comp in comps:
            ref = value_of(comp, "ref")
            if args.ref and ref not in args.ref:
                continue
            footprint = value_of(comp, "footprint").split(":")[-1]
            rows.append((ref, value_of(comp, "value"), footprint, value_of(comp, "description")))
        for ref, value, footprint, description in sorted(rows, key=lambda r: natural_key(r[0])):
            line = f"{ref:6} {value:14} {footprint or '(no footprint)'}"
            # short descriptions are the user's role notes (e.g. "ATTACK"), long ones are library boilerplate
            print(line + (f"  # {description}" if description and len(description) <= 30 else ""))
        print()

    for net in nets:
        name = value_of(net, "name")
        pins = []
        for node in children(net, "node"):
            ref, pin = value_of(node, "ref"), value_of(node, "pin")
            function = short_function(value_of(node, "pinfunction"))
            pins.append((ref, f"{ref}.{pin}" + (f"({function})" if function and function != pin else "")))
        if args.ref and not any(ref in args.ref for ref, _ in pins):
            continue
        if args.net and not any(n.lower() in name.lower() for n in args.net):
            continue
        name = re.sub(r"^unconnected-\((.*)\)$", r"NC \1", name)
        print(f"{name}: " + " ".join(label for _, label in sorted(pins, key=lambda p: natural_key(p[1]))))
    print()


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("target", help="module number/folder name, or a .net file")
    parser.add_argument("--board", nargs="+", default=[], help="split modules: only netlists whose file name contains these")
    parser.add_argument("--ref", nargs="+", default=[], help="only nets touching these references")
    parser.add_argument("--net", nargs="+", default=[], help="only nets whose name contains these")
    parser.add_argument("--nets-only", action="store_true", help="skip the component list")
    parser.add_argument("--connectors", action="store_true", help="only list connector (Conn_*) pins and their nets")
    args = parser.parse_args()

    paths = find_netlists(args.target)
    if args.board:
        paths = [p for p in paths if any(b in p.name for b in args.board)]
    for path in paths:
        print_netlist(path, args)


if __name__ == "__main__":
    main()
