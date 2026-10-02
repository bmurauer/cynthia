#!/bin/sh
# Render standalone TikZ/circuitikz .tex files to SVG (white background is set
# in the .tex itself). Intermediate files are removed; the .svg lands next to
# its .tex.
#
# Usage: tools/render_tikz.sh path/to/a.tex [path/to/b.tex ...]
#        tools/render_tikz.sh path/to/folder        # every .tex in it
set -e
[ $# -gt 0 ] || { echo "usage: $0 file.tex|folder ..." >&2; exit 1; }
for arg in "$@"; do
    if [ -d "$arg" ]; then files="$files $(ls "$arg"/*.tex)"; else files="$files $arg"; fi
done
for f in $files; do
    dir=$(dirname "$f"); base=$(basename "$f" .tex)
    if ! (cd "$dir" && pdflatex -interaction=nonstopmode -halt-on-error "$base.tex" >/dev/null); then
        grep -A3 "^!" "$dir/$base.log" >&2
        rm -f "$dir/$base.aux" "$dir/$base.log" "$dir/$base.pdf"
        exit 1
    fi
    pdftocairo -svg "$dir/$base.pdf" "$dir/$base.svg"
    rm -f "$dir/$base.aux" "$dir/$base.log" "$dir/$base.pdf"
    echo "wrote $dir/$base.svg"
done
