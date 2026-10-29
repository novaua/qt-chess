"""App icon and installer images, drawn from the board's white knight (pics/white/n.png).

    python scripts/make_icons.py        (needs Pillow: pip install pillow)

Writes:
  ChessGui/ChessGui/chess.ico            exe, taskbar and installer icon (16-256 px)
  ChessGui/ChessGui/pics/app-icon.png    the window icon and the icon on Welcome and About (256 px)
  installer/art/wizard-*.bmp             Inno Setup WizardImageFile (left banner), 100-250 % DPI
  installer/art/wizard-small-*.bmp       Inno Setup WizardSmallImageFile (top right), 100-250 % DPI
"""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFilter

ROOT = Path(__file__).resolve().parent.parent
GUI = ROOT / 'ChessGui' / 'ChessGui'
ART = ROOT / 'installer' / 'art'

# Board colours (Theme.qml): squareDark #2F4F4F, squareLight #ECF0F1.
GREEN_TOP, GREEN_BOTTOM = (0x4A, 0x75, 0x75), (0x1F, 0x36, 0x36)
LIGHT_SQUARE = (0xEC, 0xF0, 0xF1)


def gradient(w, h):
    img = Image.new('RGBA', (w, h))
    draw = ImageDraw.Draw(img)
    for y in range(h):
        t = y / max(1, h - 1)
        draw.line([(0, y), (w, y)], fill=tuple(int(a + (b - a) * t) for a, b in zip(GREEN_TOP, GREEN_BOTTOM)) + (255,))
    return img


def squares(img, size, alpha, offset=(0, 0)):
    """Faint light squares in a checker pattern over the green: a hint of the board."""
    layer = Image.new('RGBA', img.size, (0, 0, 0, 0))
    draw = ImageDraw.Draw(layer)
    for row in range(-1, img.height // size + 2):
        for col in range(-1, img.width // size + 2):
            if (row + col) % 2:
                x, y = col * size + offset[0], row * size + offset[1]
                draw.rectangle((x, y, x + size - 1, y + size - 1), fill=LIGHT_SQUARE + (alpha,))
    return Image.alpha_composite(img, layer)


def with_knight(img, knight, width, center, shadow=True):
    k = knight.resize((width, round(knight.height * width / knight.width)), Image.LANCZOS)
    x, y = center[0] - k.width // 2, center[1] - k.height // 2
    if shadow:
        alpha = Image.new('L', img.size, 0)
        alpha.paste(k.getchannel('A'), (x + width // 50, y + width // 28))
        drop = Image.new('RGBA', img.size, (0, 0, 0, 0))
        drop.putalpha(alpha.point(lambda a: int(a * 0.45)))
        img = Image.alpha_composite(img, drop.filter(ImageFilter.GaussianBlur(max(1, width // 40))))
    layer = Image.new('RGBA', img.size, (0, 0, 0, 0))
    layer.paste(k, (x, y), k)
    return Image.alpha_composite(img, layer)


def app_icon(knight, n=1024):
    """A rounded tile in the board's green with a 2x2 board corner, the knight in front."""
    margin, radius = n * 40 // 1024, n * 200 // 1024
    tile = squares(gradient(n, n), n // 2, 40, offset=(n // 2, 0))
    mask = Image.new('L', (n, n), 0)
    ImageDraw.Draw(mask).rounded_rectangle((margin, margin, n - margin - 1, n - margin - 1), radius, fill=255)
    icon = Image.new('RGBA', (n, n), (0, 0, 0, 0))
    icon.paste(tile, (0, 0), mask)
    edge = Image.new('RGBA', (n, n), (0, 0, 0, 0))
    ImageDraw.Draw(edge).rounded_rectangle((margin, margin, n - margin - 1, n - margin - 1), radius,
                                           outline=(255, 255, 255, 40), width=max(1, n // 170))
    icon = Image.alpha_composite(icon, edge)
    return with_knight(icon, knight, n * 66 // 100, (n // 2 + n // 100, n // 2 + n // 128))


def banner(knight, w, h):
    """The left banner of the installer's first and last page: the board's green, the knight low."""
    img = squares(gradient(w, h), w // 2, 22)
    return with_knight(img, knight, w * 70 // 100, (w // 2, h - h * 30 // 100))


def to_bmp(img, path, background=(255, 255, 255)):
    flat = Image.new('RGB', img.size, background)
    flat.paste(img, (0, 0), img)
    flat.save(path)


def main():
    knight = Image.open(GUI / 'pics' / 'white' / 'n.png').convert('RGBA')
    icon = app_icon(knight)

    sizes = [16, 20, 24, 32, 40, 48, 64, 256]
    icon.resize((256, 256), Image.LANCZOS).save(GUI / 'chess.ico', sizes=[(s, s) for s in sizes])
    icon.resize((256, 256), Image.LANCZOS).save(GUI / 'pics' / 'app-icon.png')

    ART.mkdir(parents=True, exist_ok=True)
    # Inno Setup's sizes for 100, 125, 150, 175, 200 and 250 % (it picks the closest).
    for scale, (w, h) in zip((100, 125, 150, 175, 200, 250),
                             ((164, 314), (192, 386), (246, 459), (273, 556), (328, 628), (410, 797))):
        to_bmp(banner(knight, w, h), ART / f'wizard-{scale}.bmp')
    for scale, (w, h) in zip((100, 125, 150, 175, 200, 250),
                             ((55, 58), (64, 68), (83, 80), (92, 97), (110, 106), (138, 140))):
        side = min(w, h)
        small = Image.new('RGBA', (w, h), (0, 0, 0, 0))
        small.paste(icon.resize((side, side), Image.LANCZOS), ((w - side) // 2, (h - side) // 2))
        to_bmp(small, ART / f'wizard-small-{scale}.bmp')


if __name__ == '__main__':
    main()
