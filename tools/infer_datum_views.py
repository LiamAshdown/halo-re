"""Finds byte-pointer locals initialised from a datum array, e.g.
    uint8_t *a = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
and, when the stride is the size of the array's element struct, rewrites the variable's raw-offset accesses in that
file into field accesses of that struct (tools/type_access.py; the variable keeps its type).
  python tools/infer_datum_views.py [--dry]"""
import collections, glob, os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MAP = {"actor_data": "actor", "prop_data": "prop", "player_data": "player", "encounter_data": "encounter",
       "light_data": "light", "particle_system_data": "particle_system", "decal_data": "decal",
       "game_looping_sound_data": "game_looping_sound", "swarm_data": "swarm",
       "particle_system_particle_data": "particle_system_particle"}
OBJ_INIT = re.compile(r"uint8_t \*(\w+) = \(uint8_t \*\)\(\(object_header \*\)object_data->data\)\[[^]]*\]\.data;")
INIT = re.compile(r"uint8_t \*(\w+) = \(uint8_t \*\)(\w+)->data \+ \([^;]*& 0xffff\) \* (0x[0-9a-f]+);")


def size_of(struct):
    for h in glob.glob(os.path.join(ROOT, "types", "*.h")):
        m = re.search(r"^\} %s;\s*// size (0x[0-9a-f]+)" % re.escape(struct), open(h, encoding="utf-8").read(), re.M)
        if m:
            return int(m.group(1), 16), os.path.basename(h)
    return None, None


def main():
    groups = collections.defaultdict(set)
    for f in glob.glob(os.path.join(ROOT, "src", "*", "*.c")):
        live = open(f, encoding="utf-8").read().split("\n#if 0")[0]
        for var, array, stride in INIT.findall(live):
            if array in MAP:
                groups[(MAP[array], var, int(stride, 16))].add(os.path.relpath(f, ROOT))
    # an object's data through its header: the object view (offsets below 0x1f4, whatever the object's type)
    for f in glob.glob(os.path.join(ROOT, "src", "*", "*.c")):
        live = open(f, encoding="utf-8").read().split("\n#if 0")[0]
        for var in OBJ_INIT.findall(live):
            if var != "unit_object":           # a local named like the view type: left alone
                groups[("object", var, 0x1f4)].add(os.path.relpath(f, ROOT))
    for (struct, var, stride), files in sorted(groups.items()):
        size, header = size_of(struct)
        if size != stride:
            print("skip %s %s: stride 0x%x, struct size %s" % (struct, var, stride, hex(size) if size else "?"))
            continue
        cmd = [sys.executable, os.path.join(ROOT, "tools", "type_access.py"), struct, header, var] + sorted(files) + \
              ["--pre", "memory.h,objects.h,units.h"] + (["--dry"] if "--dry" in sys.argv else [])
        out = subprocess.run(cmd, capture_output=True, text=True, cwd=ROOT).stdout.strip().splitlines()
        print("%-20s %-10s %s" % (struct, var, out[-1] if out else "?"))


if __name__ == "__main__":
    main()
