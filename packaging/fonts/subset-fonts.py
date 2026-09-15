import argparse
import os
import re
import string
import sys

import fontTools.subset
import fontTools.ttLib
import fontTools.varLib.instancer

SYMBOL_FONT = "MaterialSymbolsRounded.ttf"
TEXT_FONT = "RobotoFlex.ttf"
TEXT_AXES = {"wght", "wdth"}
SCANNED_SUFFIXES = (".qml", ".cpp", ".h", ".mm", ".js")
LIGATURE_ALPHABET = string.ascii_lowercase + string.digits + "_"
LAYOUT_FEATURES = ["liga", "ccmp", "rlig", "calt", "dlig"]
STRING_LITERAL = re.compile(r'"([^"\n]*)"')
GLYPH_TOKEN = re.compile(r"^[a-z][a-z0-9_]+$")


def named_glyphs(root: str, known: set[str]) -> set[str]:
    """Return the icon names quoted as string literals in the sources under a directory."""
    found: set[str] = set()
    for directory, _, filenames in os.walk(root):
        for filename in filenames:
            if not filename.endswith(SCANNED_SUFFIXES):
                continue
            path = os.path.join(directory, filename)
            with open(path, encoding="utf-8", errors="replace") as source:
                text = source.read()
            found |= {
                match.group(1)
                for match in STRING_LITERAL.finditer(text)
                if GLYPH_TOKEN.match(match.group(1)) and match.group(1) in known
            }
    return found


def subset_symbols(source: str, target: str, roots: list[str]) -> int:
    """Write the icon font with only the icons the sources name, and return how many it kept."""
    known = set(fontTools.ttLib.TTFont(source, lazy=True).getGlyphOrder())
    wanted: set[str] = set()
    for root in roots:
        wanted |= named_glyphs(root, known)
    if not wanted:
        raise SystemExit("subset-fonts: no icon names were found in " + ", ".join(roots))

    options = fontTools.subset.Options()
    options.layout_features = LAYOUT_FEATURES
    options.layout_closure = False
    options.glyph_names = True
    options.name_IDs = ["*"]
    options.name_legacy = True
    options.notdef_outline = True

    font = fontTools.subset.load_font(source, options)
    modified = font["head"].modified
    subsetter = fontTools.subset.Subsetter(options=options)
    subsetter.populate(glyphs=sorted(wanted), text=LIGATURE_ALPHABET)
    subsetter.subset(font)
    font["head"].modified = modified
    fontTools.subset.save_font(font, target, options)
    return len(wanted)


def pin_text_axes(source: str, target: str) -> int:
    """Write the text font with every axis the interface never varies pinned, and return how many."""
    font = fontTools.ttLib.TTFont(source, recalcTimestamp=False)
    modified = font["head"].modified
    pinned: dict[str, float] = {
        axis.axisTag: axis.defaultValue
        for axis in font["fvar"].axes
        if axis.axisTag not in TEXT_AXES
    }
    font = fontTools.varLib.instancer.instantiateVariableFont(font, pinned, inplace=True, updateFontNames=False)
    font["head"].modified = modified
    font.save(target)
    return len(pinned)


def report(label: str, source: str, target: str, detail: str) -> None:
    """Print a font's size before and after, and what was done to it."""
    before = os.path.getsize(source) / 1024
    after = os.path.getsize(target) / 1024
    print(f"{label}: {before:.0f} KiB -> {after:.0f} KiB ({detail})")


def main() -> int:
    """Subset the icon font and pin the text font's unused axes into the output directory."""
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--scan", required=True, nargs="+")
    arguments = parser.parse_args()

    os.makedirs(arguments.output, exist_ok=True)

    symbols_source = os.path.join(arguments.source, SYMBOL_FONT)
    symbols_target = os.path.join(arguments.output, SYMBOL_FONT)
    kept = subset_symbols(symbols_source, symbols_target, arguments.scan)
    report(SYMBOL_FONT, symbols_source, symbols_target, f"{kept} icons")

    text_source = os.path.join(arguments.source, TEXT_FONT)
    text_target = os.path.join(arguments.output, TEXT_FONT)
    pinned = pin_text_axes(text_source, text_target)
    report(TEXT_FONT, text_source, text_target, f"{pinned} axes pinned")
    return 0


if __name__ == "__main__":
    sys.exit(main())
