import io
import os
import struct
import sys

import PIL.Image
import PIL.ImageChops
import reportlab.graphics.renderPM
import svglib.svglib

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SOURCE = os.path.join(ROOT, "assets", "ht-music.svg")
TARGET = os.path.join(ROOT, "assets", "ht-music.ico")
SIZES = [16, 20, 24, 32, 40, 48, 64, 96, 128, 256]
SUPERSAMPLE = 1024
CLEAR_THRESHOLD = 4


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


def scaled(premultiplied: PIL.Image.Image, opacity: PIL.Image.Image, edge: int) -> PIL.Image.Image:
    """Downscale a premultiplied render and return it with straight colour and its alpha channel."""
    small = premultiplied.resize((edge, edge), PIL.Image.Resampling.LANCZOS)
    mask = opacity.resize((edge, edge), PIL.Image.Resampling.LANCZOS).point(
        lambda level: 0 if level < CLEAR_THRESHOLD else level)
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


def encoded(frame: PIL.Image.Image) -> bytes:
    """Return the image data Pillow writes for a single frame of an ICO file."""
    buffer = io.BytesIO()
    frame.save(buffer, format="ICO", sizes=[frame.size])
    single = buffer.getvalue()
    size, offset = struct.unpack_from("<II", single, 6 + 8)
    return single[offset:offset + size]


def write(path: str, frames: list[PIL.Image.Image]) -> None:
    """Write every frame into one ICO file, each with a directory entry of its own."""
    payloads = [encoded(frame) for frame in frames]
    offset = 6 + 16 * len(frames)
    directory = struct.pack("<HHH", 0, 1, len(frames))
    for frame, payload in zip(frames, payloads):
        edge = frame.width % 256
        directory += struct.pack("<BBBBHHII", edge, edge, 0, 0, 1, 32, len(payload), offset)
        offset += len(payload)
    with open(path, "wb") as target:
        target.write(directory)
        for payload in payloads:
            target.write(payload)


def main() -> int:
    """Generate the application icon from the brand mark and check every size came out right."""
    premultiplied, opacity = render(SOURCE, SUPERSAMPLE)
    frames = [scaled(premultiplied, opacity, size) for size in SIZES]
    write(TARGET, frames)

    written = sorted(PIL.Image.open(TARGET).ico.sizes())
    if written != sorted((size, size) for size in SIZES):
        raise SystemExit("icon is missing sizes: %s" % written)
    for frame in frames:
        if frame.getpixel((0, 0))[3] != 0:
            raise SystemExit("icon corner is not transparent at %d" % frame.width)
    print("%s  %d bytes  %d sizes" % (TARGET, os.path.getsize(TARGET), len(written)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
