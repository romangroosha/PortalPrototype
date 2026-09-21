"""Reject the black portal regression using rendered pixels inside each aperture."""
from pathlib import Path
from PIL import Image, ImageStat

root = Path(__file__).resolve().parents[1] / 'Saved' / 'PuzzleAudit'
for index in range(5):
    path = root / f'distance_{index}.png'
    im = Image.open(path).convert('RGB')
    assert im.size == (1280, 720), f'Unexpected viewport size: {im.size}'
    # Inside even the smallest (9.9 m) portal, excluding the central crosshair.
    region = im.crop((600, 300, 620, 400)).convert('L')
    stats = ImageStat.Stat(region)
    lit = sum(v > 12 for v in region.getdata()) / (region.width * region.height)
    assert lit > .65 and stats.mean[0] > 20, f'Black/missing portal at view {index}: lit={lit:.3f}, mean={stats.mean[0]:.2f}'
    print(f'PORTAL_PIXELS PASS view={index} lit={lit:.3f} mean={stats.mean[0]:.2f}')
