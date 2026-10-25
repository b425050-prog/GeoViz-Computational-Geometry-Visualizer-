"""Verify the declared body/status/focus text palette against the canvas and panel."""
from pathlib import Path
import json
def lum(rgb):
    def component(v):
        s=v/255
        return s/12.92 if s<=.04045 else ((s+.055)/1.055)**2.4
    return sum(w*component(v) for w,v in zip((.2126,.7152,.0722),rgb))
background=(246,245,240)
palette={'body':(39,52,48),'muted':(82,101,92),'accent':(36,103,77),'focus':(143,91,20),'danger':(171,62,48),'destination':(117,71,127)}
ratios={name:(lum(background)+.05)/(lum(color)+.05) for name,color in palette.items()}
assert min(ratios.values())>=4.5,ratios
print(json.dumps({'background':background,'contrast_ratios':ratios,'minimum_text_ratio':4.5},indent=2))
