#!/usr/bin/env python3
"""Build a collective bill of materials for the eurorack modules.

Reads the KiCad netlists (``*.net``) of every numbered module folder -- the
KOSMO modules live in ``kosmo/`` and are deliberately ignored -- groups the
components across all of them and prints the result as markdown.

Usage:
    tools/collective_bom.py                  # all modules, markdown on stdout
    tools/collective_bom.py 01 03            # only these modules
    tools/collective_bom.py -o BOM.md        # write to a file
    tools/collective_bom.py --refs           # add reference designators
    tools/collective_bom.py --boards 2       # quantities for 2 sets of boards

Netlists are exported from KiCad with ``File > Export > Netlist`` (KiCad
format).  A module without a netlist is reported at the end of the document
instead of being silently dropped.
"""

from __future__ import annotations

import argparse
import re
import sys
from collections import defaultdict
from pathlib import Path

# --------------------------------------------------------------------------
# s-expression parsing
# --------------------------------------------------------------------------

_TOKEN = re.compile(r'''\s*(?:(\()|(\))|"((?:[^"\\]|\\.)*)"|([^\s()"]+))''')


def parse_sexp(text: str):
    """Parse an s-expression into nested lists of strings."""
    stack: list[list] = [[]]
    pos = 0
    end = len(text)
    while pos < end:
        match = _TOKEN.match(text, pos)
        if not match:
            break
        pos = match.end()
        open_paren, close_paren, quoted, atom = match.groups()
        if open_paren:
            stack.append([])
        elif close_paren:
            node = stack.pop()
            if not stack:
                raise ValueError("unbalanced parentheses in netlist")
            stack[-1].append(node)
        elif quoted is not None:
            stack[-1].append(re.sub(r'\\(.)', r'\1', quoted))
        else:
            stack[-1].append(atom)
    if len(stack) != 1:
        raise ValueError("unbalanced parentheses in netlist")
    return stack[0][0] if len(stack[0]) == 1 else stack[0]


def child(node, key):
    """First direct child list starting with `key`, or None."""
    for item in node:
        if isinstance(item, list) and item and item[0] == key:
            return item
    return None


def children(node, key):
    return [i for i in node if isinstance(i, list) and i and i[0] == key]


def value_of(node, key, default=""):
    """Value of `(key "value")`, or `default` if missing/empty."""
    found = child(node, key)
    if found and len(found) > 1 and isinstance(found[1], str):
        return found[1]
    return default


# --------------------------------------------------------------------------
# classification
# --------------------------------------------------------------------------

# Components whose "value" is a panel label ("SAW OUT", "Gate/Loop") rather
# than a part specification: they are grouped by package alone.
LABEL_VALUE_PREFIXES = {"J", "SW", "H"}

# Reference designator prefix -> category.
PREFIX_CATEGORY = {
    "U": "ICs",
    "Q": "Transistors",
    "D": "Diodes & LEDs",
    "R": "Resistors",
    "RV": "Potentiometers",
    "TP": "Potentiometers",
    "C": "Capacitors",
    "J": "Jacks & connectors",
    "SW": "Switches",
    "H": "Hardware",
}

# Footprint patterns win over the prefix, because the same prefix is used for
# different part types across the modules (e.g. TP for both trimmers and test
# points).  Order matters, first match wins.
FOOTPRINT_CATEGORY = [
    (r"trimmer", "Trimmers"),
    (r"pot[ie]+ntiometer", "Potentiometers"),
    (r"jack", "Jacks & connectors"),
    (r"header", "Jacks & connectors"),
    (r"switch|push[ _]button", "Switches"),
    (r"^LED_", "Diodes & LEDs"),
    (r"MountingHole|Screw", "Hardware"),
]

CATEGORY_ORDER = [
    "ICs",
    "Transistors",
    "Diodes & LEDs",
    "Resistors",
    "Capacitors",
    "Trimmers",
    "Potentiometers",
    "Switches",
    "Jacks & connectors",
    "Hardware",
    "Other",
]

# Footprints that are not orderable parts.
IGNORED_FOOTPRINTS = [r"^TestPoint:", r"^Fiducial", r"^MountingHole"]

# KiCad footprint -> the package you actually order.  Different modules use
# different footprint libraries for the same physical part (a 0805 resistor is
# a 0805 resistor), so normalising here is what makes the collective counts
# meaningful.  Order matters, first match wins.
PACKAGE_RULES = [
    (r"0805", "0805"),
    (r"1206", "1206"),
    (r"0603", "0603"),
    (r"SOIC-(\d+)", lambda m: f"SOIC-{m.group(1)}"),
    (r"SOT-23-(\d+)", lambda m: f"SOT-23-{m.group(1)}"),
    (r"SOT-23", "SOT-23"),
    (r"SOT-363|SC-70-6", "SOT-363 (SC-70-6)"),
    (r"SOD-323", "SOD-323"),
    (r"D_SMA", "SMA"),
    (r"DIP-(\d+)", lambda m: f"DIP-{m.group(1)}"),
    (r"CP_Radial_D(?P<d>[\d.]+)mm_P(?P<p>[\d.]+)mm",
     lambda m: f"THT electrolytic, {m.group('d')} mm ø, {m.group('p')} mm pitch"),
    (r"C_Disc_D(?P<d>[\d.]+)mm_W[\d.]+mm_P(?P<p>[\d.]+)mm",
     lambda m: f"THT disc, {m.group('p')} mm pitch"),
    (r"C_Rect_L(?P<l>[\d.]+)mm_W(?P<w>[\d.]+)mm_P(?P<p>[\d.]+)mm",
     lambda m: f"THT film box, {m.group('p')} mm pitch"),
    (r"LED_D(?P<d>[\d.]+)mm", lambda m: f"THT LED, {m.group('d')} mm"),
    (r"potientiometer|potentiometer", "Panel potentiometer"),
    (r"trimmer", "Trimmer"),
    (r"3\.5mm jack mono switched", "3.5 mm mono jack, switched"),
    (r"Eurorack Header", "2x5 shrouded pin header, 2.54 mm"),
    (r"switch_spdt", "SPDT toggle switch"),
    (r"switch_dpdt", "DPDT toggle switch"),
    (r"switch_linear_2x4", "2x4 linear switch"),
    (r"push button", "Momentary push button"),
]


def strip_library(footprint: str) -> str:
    return footprint.split(":", 1)[1] if ":" in footprint else footprint


def package_of(footprint: str) -> str:
    """Normalise a KiCad footprint to the package you order."""
    if not footprint:
        return "(no footprint)"
    for pattern, replacement in PACKAGE_RULES:
        match = re.search(pattern, footprint, re.IGNORECASE)
        if match:
            return replacement(match) if callable(replacement) else replacement
    return re.sub(r"[_\s]+", " ", strip_library(footprint)).strip()


def prefix_of(reference: str) -> str:
    match = re.match(r"([A-Za-z#]+)", reference)
    return match.group(1).upper() if match else ""


def category_of(reference: str, footprint: str) -> str:
    for pattern, category in FOOTPRINT_CATEGORY:
        if re.search(pattern, strip_library(footprint), re.IGNORECASE):
            return category
    return PREFIX_CATEGORY.get(prefix_of(reference), "Other")


def is_ignored(footprint: str) -> bool:
    return any(re.search(p, footprint, re.IGNORECASE) for p in IGNORED_FOOTPRINTS)


# --------------------------------------------------------------------------
# sorting
# --------------------------------------------------------------------------

_SI = {"p": 1e-12, "n": 1e-9, "u": 1e-6, "µ": 1e-6, "m": 1e-3,
       "r": 1.0, "k": 1e3, "meg": 1e6, "g": 1e9}

_ENG = re.compile(
    r"^(?P<int>\d+)(?:[.,](?P<frac>\d+))?\s*"
    r"(?P<unit>meg|[pnuµmrkg])?(?P<rest>\d*)\s*[FfRrΩ]?$"
)


def numeric_value(value: str):
    """Turn '4k7', '8.2k', '820pF', '100R', '1M2' into a sortable float."""
    text = value.strip().replace("Ω", "").strip()
    if not text:
        return None
    # 1M / 1M2 -> mega (in schematics 'M' means mega, not milli)
    text = re.sub(r"(?<=\d)M(?=\d|$)", "meg", text)
    match = _ENG.match(text)
    if not match:
        return None
    digits = match.group("int")
    frac = match.group("frac") or match.group("rest") or ""
    unit = (match.group("unit") or "r").lower()
    try:
        number = float(f"{digits}.{frac}") if frac else float(digits)
    except ValueError:
        return None
    return number * _SI.get(unit, 1.0)


def row_sort_key(row: "BomRow"):
    number = numeric_value(row.value)
    return (row.package, 0, number, "") if number is not None else (row.package, 1, 0.0, row.value.lower())


def natural_refs(references: list[str]) -> list[str]:
    def key(reference: str):
        match = re.match(r"([A-Za-z#]+)(\d*)", reference)
        return (match.group(1), int(match.group(2) or 0)) if match else (reference, 0)
    return sorted(references, key=key)


# --------------------------------------------------------------------------
# model
# --------------------------------------------------------------------------

class Module:
    def __init__(self, path: Path, netlist: Path | None):
        self.path = path
        self.netlist = netlist
        self.name = path.name
        self.title = ""
        match = re.match(r"(\d+)", self.name)
        self.key = match.group(1) if match else self.name

    @property
    def label(self) -> str:
        return self.title or self.name


class BomRow:
    def __init__(self, category: str, value: str, package: str):
        self.category = category
        self.value = value
        self.package = package
        self.per_module: dict[str, int] = defaultdict(int)
        self.refs: dict[str, list[str]] = defaultdict(list)
        self.footprints: set[str] = set()

    def add(self, module_key: str, reference: str, footprint: str):
        self.per_module[module_key] += 1
        self.refs[module_key].append(reference)
        if footprint:
            self.footprints.add(footprint)

    @property
    def total(self) -> int:
        return sum(self.per_module.values())


# --------------------------------------------------------------------------
# collecting
# --------------------------------------------------------------------------

def find_modules(root: Path, wanted: list[str]) -> list[Module]:
    """Numbered module folders (the KOSMO ones have no numeric prefix)."""
    modules = []
    for path in sorted(p for p in root.iterdir() if p.is_dir()):
        if not re.match(r"\d+_", path.name):
            continue
        if wanted and not any(w == path.name or path.name.startswith(f"{w}_") for w in wanted):
            continue
        preferred = path / f"{path.name}.net"
        netlists = [preferred] if preferred.exists() else sorted(path.glob("*.net"))
        modules.append(Module(path, netlists[0] if netlists else None))
    return modules


def read_components(module: Module) -> list[tuple[str, str, str, str]]:
    """Return (ref, value, footprint, description) for every BOM component."""
    export = parse_sexp(module.netlist.read_text(encoding="utf-8"))

    design = child(export, "design")
    if design:
        sheet = child(design, "sheet")
        block = child(sheet, "title_block") if sheet else None
        if block:
            module.title = value_of(block, "title")

    section = child(export, "components")
    if not section:
        return []

    components = []
    for comp in children(section, "comp"):
        reference = value_of(comp, "ref")
        if not reference or reference.startswith("#"):  # power/ground symbols
            continue
        properties = {value_of(p, "name").lower() for p in children(comp, "property")}
        if {"dnp", "exclude_from_bom"} & properties:
            continue
        footprint = value_of(comp, "footprint")
        if is_ignored(footprint):
            continue
        components.append((reference, value_of(comp, "value"), footprint,
                           value_of(comp, "description")))
    return components


def build_rows(modules: list[Module]) -> tuple[dict[str, list[BomRow]], list[Module]]:
    rows: dict[tuple, BomRow] = {}
    missing = []
    for module in modules:
        if module.netlist is None:
            missing.append(module)
            continue
        for reference, value, footprint, _description in read_components(module):
            category = category_of(reference, footprint)
            package = package_of(footprint)
            # For jacks, switches and headers the value is a panel label.
            grouped_value = "" if prefix_of(reference) in LABEL_VALUE_PREFIXES else value.strip()
            if grouped_value in ("~", "*"):
                grouped_value = ""
            key = (category, grouped_value, package)
            row = rows.get(key)
            if row is None:
                row = rows[key] = BomRow(category, grouped_value, package)
            row.add(module.key, reference, footprint)

    by_category: dict[str, list[BomRow]] = defaultdict(list)
    for row in rows.values():
        by_category[row.category].append(row)
    for category_rows in by_category.values():
        category_rows.sort(key=row_sort_key)
    return by_category, missing


# --------------------------------------------------------------------------
# markdown rendering
# --------------------------------------------------------------------------

def escape(text: str) -> str:
    return text.replace("|", "\\|")


def table(header: list[str], body: list[list[str]], align: list[str] | None = None) -> list[str]:
    align = align or ["---"] * len(header)
    lines = ["| " + " | ".join(header) + " |", "|" + "|".join(align) + "|"]
    lines += ["| " + " | ".join(row) + " |" for row in body]
    return lines


def render(modules: list[Module], by_category: dict[str, list[BomRow]],
           missing: list[Module], boards: int, show_refs: bool) -> str:
    present = [m for m in modules if m.netlist is not None]
    out: list[str] = ["# Collective BOM", ""]
    out.append("Parts for the modules listed below, generated by "
               "`tools/collective_bom.py` from the KiCad netlists.")
    if boards != 1:
        out.append(f"Quantities are for **{boards} sets** of boards.")
    out += ["", "## Modules", ""]
    out += table(
        ["#", "Module", "Title", "Parts/board"],
        [[m.key, f"`{m.name}`", escape(m.title or "—"),
          str(sum(r.per_module.get(m.key, 0) for rows in by_category.values() for r in rows))]
         for m in present],
        ["---", "---", "---", "--:"],
    )
    out.append("")

    categories = [c for c in CATEGORY_ORDER if by_category.get(c)]
    categories += sorted(c for c in by_category if c not in CATEGORY_ORDER)

    out += ["## Parts", ""]
    for category in categories:
        rows = by_category[category]
        out += [f"### {category}", ""]
        header = ["Value", "Package"] + [m.key for m in present] + ["Total"]
        align = ["---", "---"] + ["--:"] * len(present) + ["--:"]
        if show_refs:
            header.append("References")
            align.append("---")
        body = []
        for row in rows:
            cells = [escape(row.value) if row.value else "—", escape(row.package)]
            cells += [str(row.per_module.get(m.key, 0) * boards) for m in present]
            cells.append(f"**{row.total * boards}**")
            if show_refs:
                cells.append(", ".join(
                    f"{m.key}: {' '.join(natural_refs(row.refs[m.key]))}"
                    for m in present if row.refs.get(m.key)))
            body.append(cells)
        subtotal = ["**Subtotal**", ""]
        subtotal += [f"**{sum(r.per_module.get(m.key, 0) for r in rows) * boards}**" for m in present]
        subtotal.append(f"**{sum(r.total for r in rows) * boards}**")
        if show_refs:
            subtotal.append("")
        body.append(subtotal)
        out += table(header, body, align)
        out.append("")

    out += ["## Summary", ""]
    summary = [[category, str(len(by_category[category])),
                str(sum(r.total for r in by_category[category]) * boards)]
               for category in categories]
    summary.append(["**Total**",
                    f"**{sum(len(by_category[c]) for c in categories)}**",
                    f"**{sum(r.total for c in categories for r in by_category[c]) * boards}**"])
    out += table(["Category", "Distinct parts", "Quantity"], summary, ["---", "--:", "--:"])
    out.append("")

    grouped: dict[tuple[str, str], set[str]] = defaultdict(set)
    for category, rows in by_category.items():
        for row in rows:
            grouped[(category, row.package)] |= row.footprints
    merged = {k: v for k, v in grouped.items() if len(v) > 1}
    if merged:
        out += ["## Merged footprints", "",
                "The same physical part is drawn with different KiCad footprints "
                "in different modules; these were counted as one part:", ""]
        out += table(["Category", "Package", "KiCad footprints"],
                     [[escape(category), escape(package),
                       ", ".join(f"`{escape(f)}`" for f in sorted(footprints))]
                      for (category, package), footprints in sorted(merged.items())])
        out.append("")

    if missing:
        out += ["## Modules without a netlist", "",
                "These modules are **not** included above. Export a netlist from "
                "KiCad (`File > Export > Netlist`, KiCad format) and re-run the script:", ""]
        out += [f"- `{m.name}`" for m in missing]
        out.append("")

    return "\n".join(out).rstrip() + "\n"


# --------------------------------------------------------------------------

def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description="Collective BOM for the eurorack modules (KOSMO excluded).")
    parser.add_argument("modules", nargs="*",
                        help="restrict to these modules, by number or folder name")
    parser.add_argument("-r", "--root", type=Path,
                        default=Path(__file__).resolve().parent.parent,
                        help="repository root (default: parent of tools/)")
    parser.add_argument("-o", "--output", type=Path,
                        help="write markdown to this file instead of stdout")
    parser.add_argument("-b", "--boards", type=int, default=1,
                        help="multiply all quantities by this many sets of boards")
    parser.add_argument("--refs", action="store_true",
                        help="add a column with the reference designators")
    args = parser.parse_args(argv)

    if args.boards < 1:
        parser.error("--boards must be at least 1")

    modules = find_modules(args.root, args.modules)
    if not modules:
        print(f"no numbered module folders found in {args.root}", file=sys.stderr)
        return 1

    by_category, missing = build_rows(modules)
    for module in missing:
        print(f"warning: no netlist in {module.name}, module skipped", file=sys.stderr)
    if not by_category:
        print("no components found", file=sys.stderr)
        return 1

    markdown = render(modules, by_category, missing, args.boards, args.refs)
    if args.output:
        args.output.write_text(markdown, encoding="utf-8")
        print(f"wrote {args.output}", file=sys.stderr)
    else:
        sys.stdout.write(markdown)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
