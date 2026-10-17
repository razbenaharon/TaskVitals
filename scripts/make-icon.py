# Regenerates src/TaskVitals.ico: CPU and RAM bars in the default palette.
# Requires Pillow. Each size is drawn at 8x and downsampled for clean edges.
from pathlib import Path
from PIL import Image, ImageDraw

BACKGROUND = (22, 25, 31, 255)
BORDER = (54, 62, 77, 255)
TRACK = (44, 50, 62, 255)
CPU = (91, 206, 182, 255)
RAM = (190, 176, 255, 255)
SIZES = (16, 20, 24, 32, 40, 48, 64, 128, 256)


def render(size):
    scale = 8
    n = size * scale
    image = Image.new('RGBA', (n, n), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    small = size <= 24
    inset = 0 if small else n * 0.03
    draw.rounded_rectangle((inset, inset, n - inset - 1, n - inset - 1), radius=n * 0.22,
                           fill=BACKGROUND, outline=None if small else BORDER,
                           width=max(scale, int(n * 0.012)))
    width = n * (0.24 if small else 0.22)
    gap = n * (0.12 if small else 0.13)
    left = (n - 2 * width - gap) / 2
    top, bottom = n * 0.20, n * 0.80
    for i, (color, level) in enumerate(((CPU, 0.74), (RAM, 0.44))):
        x = left + i * (width + gap)
        radius = width * 0.3
        draw.rounded_rectangle((x, top, x + width, bottom), radius=radius, fill=TRACK)
        draw.rounded_rectangle((x, bottom - (bottom - top) * level, x + width, bottom),
                               radius=radius, fill=color)
    return image.resize((size, size), Image.LANCZOS)


def main():
    output = Path(__file__).resolve().parent.parent / 'src' / 'TaskVitals.ico'
    images = [render(size) for size in SIZES]
    images[-1].save(output, format='ICO', sizes=[(s, s) for s in SIZES],
                    append_images=images[:-1])
    print(f'Wrote {output}')


if __name__ == '__main__':
    main()
