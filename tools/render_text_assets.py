#!/usr/bin/env python3
"""
Genera le scritte del plug-in come immagini PNG usando il font Pricedown.

Il font NON viene incluso nel plug-in (la licenza gratuita Typodermic non
permette di inserirlo nel software): nel plug-in finiscono solo le immagini.

Uso:   python3 tools/render_text_assets.py "percorso/Pricedown Bl.otf"
Output: Assets/*.png  (risoluzione 2x per schermi Retina)
"""
import sys, os
from PIL import Image, ImageDraw, ImageFont, ImageFilter, ImageChops

SCALE = 2
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "Assets")


def hexc(h, a=255):
    h = h.lstrip("#")
    return (int(h[0:2], 16), int(h[2:4], 16), int(h[4:6], 16), a)


def vertical_gradient(size, stops):
    w, h = size
    grad = Image.new("RGBA", (1, h))
    for y in range(h):
        t = y / max(1, h - 1)
        for i in range(len(stops) - 1):
            p0, c0 = stops[i]
            p1, c1 = stops[i + 1]
            if p0 <= t <= p1:
                k = (t - p0) / max(1e-6, p1 - p0)
                grad.putpixel((0, y), tuple(int(c0[j] + (c1[j] - c0[j]) * k) for j in range(4)))
                break
    return grad.resize((w, h))


def render(text, font_path, height_px, fill_stops, stroke_px, stroke_col,
           glow_col=None, glow_radius=0, shadow=(0, 0), tracking=0):
    """height_px e stroke_px sono in pixel logici (verranno moltiplicati per SCALE)."""
    size = int(height_px * SCALE)
    font = ImageFont.truetype(font_path, size)
    sw = int(round(stroke_px * SCALE))
    pad = sw + int(glow_radius * SCALE * 3) + 8 + int(max(abs(shadow[0]), abs(shadow[1])) * SCALE)

    # larghezza con spaziatura tra le lettere
    widths = [font.getlength(ch) for ch in text]
    total_w = int(sum(widths) + tracking * SCALE * (len(text) - 1))
    asc, desc = font.getmetrics()
    W, H = total_w + pad * 2, asc + desc + pad * 2

    def text_mask(stroke):
        m = Image.new("L", (W, H), 0)
        d = ImageDraw.Draw(m)
        x = pad
        for ch, cw in zip(text, widths):
            d.text((x, pad), ch, font=font, fill=255, stroke_width=stroke, stroke_fill=255)
            x += cw + tracking * SCALE
        return m

    fill_mask = text_mask(0)
    outer_mask = text_mask(sw) if sw > 0 else fill_mask

    img = Image.new("RGBA", (W, H), (0, 0, 0, 0))

    if glow_col and glow_radius > 0:
        for radius, alpha in ((glow_radius * 2.2, 0.55), (glow_radius, 0.85)):
            blur = outer_mask.filter(ImageFilter.GaussianBlur(radius * SCALE))
            blur = blur.point(lambda v: int(min(255, v * alpha * 1.6)))
            layer = Image.new("RGBA", (W, H), glow_col[:3] + (0,))
            layer.putalpha(blur)
            img = Image.alpha_composite(img, layer)

    if shadow != (0, 0):
        sh = ImageChops.offset(outer_mask, int(shadow[0] * SCALE), int(shadow[1] * SCALE))
        layer = Image.new("RGBA", (W, H), (0, 0, 0, 0))
        layer.putalpha(sh.point(lambda v: int(v * 0.6)))
        img = Image.alpha_composite(img, layer)

    if sw > 0:
        layer = Image.new("RGBA", (W, H), stroke_col[:3] + (0,))
        layer.putalpha(outer_mask.point(lambda v: int(v * stroke_col[3] / 255)))
        img = Image.alpha_composite(img, layer)

    bbox = fill_mask.getbbox()
    grad_full = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    gh = bbox[3] - bbox[1]
    grad_full.paste(vertical_gradient((W, gh), fill_stops), (0, bbox[1]))
    grad_full.putalpha(ImageChops.multiply(grad_full.getchannel("A"), fill_mask))
    img = Image.alpha_composite(img, grad_full)

    return img.crop(img.getbbox())


def lerp(a, b, k):
    return tuple(int(a[i] + (b[i] - a[i]) * k) for i in range(4))


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)
    font = sys.argv[1]
    os.makedirs(OUT, exist_ok=True)
    plum = hexc("#2a0c26")

    # titolo: gradiente bianco -> pesca -> rosa, contorno prugna, bagliore rosa
    render("Wanted Level", font, 80,
           [(0.0, hexc("#fff6ee")), (0.5, hexc("#ffb48e")), (1.0, hexc("#ff5f9a"))],
           3.0, plum, glow_col=hexc("#c2407f"), glow_radius=7, shadow=(0, 3)
           ).save(os.path.join(OUT, "title.png"))

    render("SUNSHINE STATE DISTORTION", font, 17,
           [(0.0, hexc("#ffe9f1")), (1.0, hexc("#ffc3d8"))],
           1.4, plum, glow_col=hexc("#ff5fae"), glow_radius=3, tracking=2.5
           ).save(os.path.join(OUT, "subtitle.png"))

    names = ["CLEAN RECORD", "PETTY THEFT", "GRAND THEFT", "ARMED & LOUD", "STATEWIDE MANHUNT", "MAXIMUM CHAOS"]
    calm, hot = hexc("#ffd6e6"), hexc("#ff2a4a")
    for i, n in enumerate(names):
        k = max(0.0, min(1.0, (i - 1.5) / 3.0))
        c = lerp(calm, hot, k)
        top = lerp(c, hexc("#ffffff"), 0.35)
        render(n, font, 27, [(0.0, top), (1.0, c)], 2.2, plum, shadow=(1.5, 2)
               ).save(os.path.join(OUT, f"status_{i}.png"))

    for label, fname in (("MIX", "label_mix.png"), ("OUTPUT", "label_output.png")):
        render(label, font, 20, [(0.0, hexc("#ffffff")), (1.0, hexc("#ffc3d8"))], 1.6, plum, shadow=(1, 1.5)
               ).save(os.path.join(OUT, fname))

    print("Immagini create in", os.path.abspath(OUT))


if __name__ == "__main__":
    main()
