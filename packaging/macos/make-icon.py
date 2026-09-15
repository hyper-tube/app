import io
import os
import sys

import PIL.Image
import PIL.ImageChops
import PIL.ImageFilter
import reportlab.graphics.renderPM
import svglib.svglib

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SOURCE = os.path.join(ROOT, "assets", "ht-music-macos.svg")
TARGET = os.path.join(ROOT, "assets", "ht-music.icns")
CANVAS = 1024
PLATE = 824
SIZES = [32, 64, 128, 256, 512, 1024]
SUPERSAMPLE = 2048
SHADOW_OFFSET = 12
SHADOW_BLUR = 14
SHADOW_OPACITY = 0.3
OPAQUE_THRESHOLD = 128


def flatten(path: str, edge: int, background: int) -> PIL.Image.Image:
    """Render the SVG as a square image of the given edge over a solid background colour."""
    drawing = svglib.svglib.svg2rlg(path)
    scale = edge / drawing.width
    drawing.width = edge
    drawing.height = edge
    drawing.scale(scale, scale)
    data = reportlab.graphics.renderPM.drawToString(drawing, fmt="PNG", bg=background)
    return PIL.Image.open(io.BytesIO(data)).convert("RGB")


def render(path: str, edge: int) -> tuple[PIL.Image.Image, PIL.Image.Image]:
    """Return the SVG premultiplied over black and its opacity, recovered from a render over white."""
    over_black = flatten(path, edge, 0x000000)
    over_white = flatten(path, edge, 0xFFFFFF)
    opacity = PIL.ImageChops.invert(PIL.ImageChops.difference(over_white, over_black)).convert("L")
    return over_black, opacity


def shadowed(opacity: PIL.Image.Image) -> PIL.Image.Image:
    """Return the opacity of the plate composited over its own soft black drop shadow."""
    factor = opacity.width / CANVAS
    shadow = PIL.Image.new("L", opacity.size, 0)
    shadow.paste(opacity, (0, round(SHADOW_OFFSET * factor)))
    shadow = shadow.filter(PIL.ImageFilter.GaussianBlur(SHADOW_BLUR * factor))
    shadow = shadow.point(lambda level: round(level * SHADOW_OPACITY))
    return PIL.ImageChops.add(opacity, PIL.ImageChops.multiply(shadow, PIL.ImageChops.invert(opacity)))


def scaled(premultiplied: PIL.Image.Image, opacity: PIL.Image.Image, edge: int) -> PIL.Image.Image:
    """Downscale a premultiplied render and return it with straight colour and its alpha channel."""
    small = premultiplied.resize((edge, edge), PIL.Image.Resampling.LANCZOS)
    mask = opacity.resize((edge, edge), PIL.Image.Resampling.LANCZOS)
    straight = PIL.Image.new("RGB", small.size)
    source = small.load()
    cover = mask.load()
    target = straight.load()
    for y in range(edge):
        for x in range(edge):
            alpha = cover[x, y]
            if alpha == 0:
                continue
            red, green, blue = source[x, y]
            target[x, y] = (min(255, red * 255 // alpha),
                            min(255, green * 255 // alpha),
                            min(255, blue * 255 // alpha))
    straight.putalpha(mask)
    return straight


def main() -> int:
    """Generate the macOS application icon from its grid artwork and check it sits on Apple's grid."""
    premultiplied, opacity = render(SOURCE, SUPERSAMPLE)
    frames = [scaled(premultiplied, shadowed(opacity), size) for size in SIZES]
    frames[-1].save(TARGET, format="ICNS", append_images=frames)

    written = PIL.Image.open(TARGET)
    written.load()
    if written.size != (CANVAS, CANVAS):
        raise SystemExit("icon is %s, not %d" % (written.size, CANVAS))
    plate = written.getchannel("A").point(lambda level: 255 if level >= OPAQUE_THRESHOLD else 0).getbbox()
    margin = (CANVAS - PLATE) // 2
    if plate != (margin, margin, CANVAS - margin, CANVAS - margin):
        raise SystemExit("icon plate is %s, not on the %d grid" % (plate, PLATE))
    if written.getpixel((0, 0))[3] != 0:
        raise SystemExit("icon corner is not transparent")
    print("%s  %d bytes  plate %s" % (TARGET, os.path.getsize(TARGET), plate))
    return 0


if __name__ == "__main__":
    sys.exit(main())
