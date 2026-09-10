"""RipSlice app icon.

A waveform sheared apart along a diagonal tear. The halves slide ALONG the tear
so the bars step out of register across the seam -- that misalignment is what
reads as "ripped"; a perpendicular gap only reads as a line drawn over bars.

No outline is drawn on the tear: two lips that close together merge into a
stripe and stop reading as a rip. The orange-red accent instead marks one bar
as live, echoing the app itself, where the slot currently being recorded is
highlighted orange against the lime waveform.

Palette from audiodestrukt.com (--bg, --accent, --accent2).
"""
from PIL import Image, ImageDraw
import math, sys

S      = 1024
BG     = (0x0d, 0x0d, 0x0d)
LIME   = (0xc8, 0xff, 0x00)
ORANGE = (0xff, 0x3c, 0x00)

ENV   = [0.36, 0.66, 0.48, 1.00, 0.74, 0.90, 0.44, 0.62]
LIVE  = 3                 # index of the orange "recording" bar (the tallest)
N     = len(ENV)
ANGLE = -14.0
GAP   = 0.030             # tear width as a fraction of S
SLIDE = 0.055             # slide along the tear
INSET = 0.155             # keeps bars inside the frame once slid
CAP   = 0.66              # tallest bar vs half-height, leaves vertical margin

def bars(size):
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    left, right = size * INSET, size * (1 - INSET)
    pitch = (right - left) / N
    bw    = pitch * 0.62
    mid   = size / 2
    for i, e in enumerate(ENV):
        cx = left + pitch * (i + 0.5)
        h  = e * size * 0.5 * CAP
        col = ORANGE if i == LIVE else LIME
        d.rounded_rectangle([cx - bw / 2, mid - h, cx + bw / 2, mid + h],
                            radius=bw * 0.26, fill=col + (255,))
    return img

def seam(size, offset):
    t   = math.tan(math.radians(ANGLE))
    rad = math.radians(ANGLE)
    nx, ny = -math.sin(rad), math.cos(rad)
    pts = []
    steps = 7
    for k in range(steps + 1):
        x = size * k / steps
        y = size * 0.5 + (x - size * 0.5) * t
        y += size * 0.013 * math.sin(k * 2.3)      # torn, not machined
        pts.append((x + nx * offset, y + ny * offset))
    return pts

def build():
    base = Image.new("RGB", (S, S), BG)
    wave = bars(S)
    rad = math.radians(ANGLE)
    ux, uy = math.cos(rad), math.sin(rad)
    nx, ny = -math.sin(rad), math.cos(rad)

    for upper in (True, False):
        sgn = -1 if upper else 1
        sx = (ux * SLIDE + nx * (GAP / 2)) * sgn * S
        sy = (uy * SLIDE + ny * (GAP / 2)) * sgn * S

        layer = Image.new("RGBA", (S, S), (0, 0, 0, 0))
        layer.paste(wave, (int(round(sx)), int(round(sy))), wave)

        edge = seam(S, (GAP / 2) * S * sgn)
        far  = [(S * 2, -S * 2), (-S, -S * 2)] if upper else [(S * 2, S * 3), (-S, S * 3)]
        m = Image.new("L", (S, S), 0)
        ImageDraw.Draw(m).polygon(list(edge) + far, fill=255)

        base.paste(layer, (0, 0),
                   Image.composite(layer.split()[3], Image.new("L", (S, S), 0), m))
    return base

out = sys.argv[1]
img = build()
assert img.mode == "RGB"
img.save(out, "PNG")
print("wrote", out, img.size, img.mode)
