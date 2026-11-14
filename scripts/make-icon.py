# Regenerates src/TaskVitals.ico from the icon design in design/icon/: CPU and RAM
# bars rising out of a taskbar band, in the default palette. Sizes up to 24 px use
# the small variant drawn on a 16-unit pixel grid; larger sizes use the 64-unit
# master. Requires Pillow 9.1+. Each size is drawn at 8x and downsampled.
from pathlib import Path
from PIL import Image, ImageDraw

BACKGROUND = (22, 25, 31, 255)
BAND = (44, 50, 62, 255)
CPU = (91, 206, 182, 255)
RAM = (190, 176, 255, 255)
SIZES = (16, 20, 24, 32, 40, 48, 64, 128, 256)

# Geometry in grid units, matching taskvitals-symbol-small.svg and -color.svg.
SMALL = dict(grid=16, tile=3, band=11, bar=1, bars=((CPU, 3, 2, 7, 13), (RAM, 9, 6, 13, 13)))
MASTER = dict(grid=64, tile=14, band=44, bar=4, bars=((CPU, 17, 12, 29, 52), (RAM, 35, 26, 47, 52)))


def render(size):
    spec = SMALL if size <= 24 else MASTER
    n = size * 8
    u = n / spec['grid']
    image = Image.new('RGBA', (n, n), (0, 0, 0, 0))
    ImageDraw.Draw(image).rounded_rectangle((0, 0, n - 1, n - 1), radius=spec['tile'] * u, fill=BACKGROUND)
    band = Image.new('RGBA', (n, n), (0, 0, 0, 0))
    ImageDraw.Draw(band).rounded_rectangle((0, 0, n - 1, n - 1), radius=spec['tile'] * u, fill=BAND)
    top = round(spec['band'] * u)
    image.alpha_composite(band.crop((0, top, n, n)), (0, top))
    draw = ImageDraw.Draw(image)
    for color, x0, y0, x1, y1 in spec['bars']:
        draw.rounded_rectangle((x0 * u, y0 * u, x1 * u - 1, y1 * u - 1), radius=spec['bar'] * u,
                               fill=color, corners=(True, True, False, False))
    return image.resize((size, size), Image.LANCZOS)


def main():
    output = Path(__file__).resolve().parent.parent / 'src' / 'TaskVitals.ico'
    images = [render(size) for size in SIZES]
    images[-1].save(output, format='ICO', sizes=[(s, s) for s in SIZES],
                    append_images=images[:-1])
    print(f'Wrote {output}')


if __name__ == '__main__':
    main()
