from pathlib import Path
from PIL import Image, ImageDraw, ImageFilter
import numpy as np
import csv, json, sys

root = Path(sys.argv[1]) if len(sys.argv)>1 else Path(__file__).resolve().parents[1]/'Saved'/'PortalAudit'
results = {}
rows = []
for name in ['center','oblique','near','subnear','on_plane','pitch','wide']:
    if not (root/f'{name}_portal.png').exists(): continue
    portal = Image.open(root/f'{name}_portal.png').convert('RGB')
    reference = Image.open(root/f'{name}_reference.png').convert('RGB')
    polygon = [tuple(map(float,line.split(','))) for line in (root/f'{name}_polygon.csv').read_text(encoding='utf-8-sig').splitlines()]
    mask = Image.new('L',portal.size)
    ImageDraw.Draw(mask).polygon(polygon,fill=255)
    mask = mask.filter(ImageFilter.MinFilter(25))
    valid = np.asarray(mask)>0
    a,b = np.asarray(portal).astype(float),np.asarray(reference).astype(float)
    error = np.abs(a-b).mean(axis=2)
    results[name] = {'resolution':portal.size,'pixels':int(valid.sum()),'mean_abs_rgb_error_255':float(error[valid].mean()),'p95_rgb_error_255':float(np.percentile(error[valid],95)), 'fraction_error_over_10':float((error[valid]>10).mean())}
    # Physical entry/exit jambs occupy parts of the mathematical aperture at oblique
    # angles. Compare its inner 60% separately, excluding those real occluders.
    points=np.array(polygon); center=points.mean(axis=0)
    inner=Image.new('L',portal.size)
    ImageDraw.Draw(inner).polygon([tuple(p) for p in center+(points-center)*.6],fill=255)
    core=(np.asarray(inner)>0)&valid
    results[name]['core_mean_error_255']=float(error[core].mean())
    results[name]['core_p95_error_255']=float(np.percentile(error[core],95))
    heat = np.zeros_like(a,dtype=np.uint8)
    heat[:,:,0] = np.minimum(error*8,255).astype(np.uint8)
    heat[~valid]=0
    row = Image.new('RGB',(1440,300),(24,24,24))
    draw = ImageDraw.Draw(row)
    for i,(im,label) in enumerate([(portal,'PORTAL'),(reference,'DIRECT REFERENCE'),(Image.fromarray(heat),'DIFFERENCE x8 (interior)')]):
        im.thumbnail((480,270))
        row.paste(im,(i*480,30))
        draw.text((i*480+8,8),f'{name}: {label}',fill='white')
    rows.append(row)
sheet=Image.new('RGB',(1440,300*len(rows)))
for i,row in enumerate(rows): sheet.paste(row,(0,i*300))
sheet.save(root/'comparison.png')
if (root/'frames.csv').exists():
    data=list(csv.DictReader((root/'frames.csv').open(encoding='utf-8-sig')))
    perf={}
    for name in dict.fromkeys(d['scenario'] for d in data):
        frames=np.array([float(d['frame_ms']) for d in data if d['scenario']==name])
        gpu=np.array([float(d['gpu_ms']) for d in data if d['scenario']==name])
        perf[name]={'frames':len(frames),'mean_fps':1000/frames.mean(),'p95_ms':float(np.percentile(frames,95)), 'p99_ms':float(np.percentile(frames,99)),'max_ms':float(frames.max()),'frames_over_16_67ms':int((frames>1000/60).sum()),'frames_over_33_33ms':int((frames>1000/30).sum()),'gpu_mean_ms':float(gpu.mean())}
        if 'captures' in data[0]:
            subset=[d for d in data if d['scenario']==name]
            perf[name]['mean_captures']=float(np.mean([int(d['captures']) for d in subset]))
            perf[name]['max_camera_error_cm']=max(float(d['camera_error_cm']) for d in subset)
            perf[name]['crossings']=sum(int(d['crossed']) for d in subset)
    results['performance']=perf
(root/'analysis.json').write_text(json.dumps(results,indent=2),encoding='utf-8')
print(json.dumps(results,indent=2))
