"""weapon_get_zoom_fov (0x46fe10) is really a difficulty-scaled value lookup: (stack table index, CX difficulty).
Every external binary call site loads CX from the game globals' difficulty, [0x006b0b80] + 0x0e (checked at all 13
sites; the 14th is the recursion inside weapon_get_zoom_fov_resolved). Many C callers declared it with one argument
and dropped the difficulty; projectile_update passed the two in the wrong order. This rewrites each caller's
declaration to the definition's and supplies the difficulty. Usage: python tools/fix_difficulty_value_callers.py"""
import os, re

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CALLERS = ["ai/actor_evaluate_combat_state_transition", "ai/ai_get_difficulty_request", "projectiles/projectile_update",
           "objects/object_damage_apply_line_of_sight", "objects/object_apply_damage", "objects/object_apply_body_damage",
           "objects/object_apply_shield_damage", "units/biped_integrate_movement",
           "units/biped_integrate_movement_with_collision"]
DECL = ("extern real weapon_get_zoom_fov(int16_t zoom_table_index, int16_t magnification);\n"
        "    // 0x46fe10, blam-cc: stack -> zoom_table_index, CX -> magnification (every caller passes the difficulty)\n"
        "extern uint8_t *main_game_globals; // 0x006b0b80 game globals *, +0x0e difficulty\n")
DIFFICULTY = "*(int16_t *)(main_game_globals + 0x0e)"


def main():
    for rel in CALLERS:
        path = os.path.join(ROOT, "src", *rel.split("/")) + ".c"
        if not os.path.exists(path):
            path = [os.path.join(ROOT, "src", d, os.path.basename(rel) + ".c") for d in os.listdir(os.path.join(ROOT, "src"))
                    if os.path.exists(os.path.join(ROOT, "src", d, os.path.basename(rel) + ".c"))][0]
        t = open(path, encoding="utf-8").read()
        cut = t.rfind("#if 0")
        b, tail = (t, "") if cut < 0 else (t[:cut], t[cut:])
        b, n = re.subn(r"^[ \t]*extern\s+(?:real|float)\s+weapon_get_zoom_fov\([^)]*\)\s*;[^\n]*\n", DECL, b, count=1, flags=re.M)
        assert n == 1, path
        head, body = b.split(DECL, 1)
        body, swapped = re.subn(r"weapon_get_zoom_fov\(\*\(int16_t \*\)\((\w+) \+ 0x0e\), (0x[0-9a-f]+|\d+)\)",
                                lambda m: "weapon_get_zoom_fov(%s, *(int16_t *)(%s + 0x0e))" % (m.group(2), m.group(1)), body)
        body, single = re.subn(r"weapon_get_zoom_fov\(\s*(0x[0-9a-f]+|\d+)\s*\)",
                               lambda m: "weapon_get_zoom_fov(%s, %s)" % (m.group(1), DIFFICULTY), body)
        open(path, "w", encoding="utf-8", newline="").write(head + DECL + body + tail)
        print("%-50s single-arg calls fixed %d, swapped %d" % (os.path.relpath(path, ROOT), single, swapped))


if __name__ == "__main__":
    main()
