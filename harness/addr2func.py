"""Map addresses inside halo_rewrite.dll to the function (and object) they fall in, using halo_rewrite.map.
Usage: python harness/addr2func.py 3021db74 300edcdf ..."""
import os, re, sys, bisect
MAP = os.path.join(os.path.dirname(os.path.abspath(__file__)), "build", "halo_rewrite.map")
rows = []
for l in open(MAP, errors="replace"):
    m = re.match(r"\s*0001:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})\s+(?:f\s+)?(?:i\s+)?(\S+)\s*$", l, re.I)
    if m: rows.append((int(m.group(2), 16), m.group(1), m.group(3)))
rows.sort(); starts = [r[0] for r in rows]
for a in sys.argv[1:]:
    v = int(a, 16); i = bisect.bisect_right(starts, v) - 1
    print(f"{v:08x}  {rows[i][1]}+0x{v - rows[i][0]:x}  ({rows[i][2]})" if i >= 0 else f"{v:08x}  ?")
