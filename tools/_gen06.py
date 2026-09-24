import json, os, re

src = open(r'C:\Users\Liam-\halo-re\out\slices\06.md', encoding='utf-8', errors='replace').read()
addrs = re.findall(r'(?m)^(0x[0-9a-f]+)', src)
assert len(addrs) == 300, len(addrs)


def module_for(a):
    if 0x4b1e20 <= a <= 0x4b4120:
        return ("interface", 0.6,
                "run 0x4b1e20-0x4b4120: sig__spectate_hud_sig + sig__motion_sensor_update_sig; HUD/radar drawing")
    if 0x4b43c0 <= a <= 0x4b5b20:
        return ("interface", 0.55,
                "run 0x4b43c0-0x4b5b20: strings 'ui\\shell\\main_menu\\settings_select\\player_setup\\player_profile' + '?forward', ui widget/text callees")
    if 0x4b5d70 <= a <= 0x4bab50:
        return ("networking", 0.45,
                "run 0x4b5d70-0x4bab50: GameSpy serverbrowsing API 0x616f80-0x617c10 + keys hostname/mapname/gametype/numplayers; multiplayer server browser (alt: interface)")
    if 0x4baba0 <= a <= 0x4bb640:
        return ("interface", 0.5,
                "run 0x4baba0-0x4bb640: video settings screen - '-vidmode', '%d x %d', '%d Hz', sig__gamma_sig; chain only reachable from ui callback 0x4bb5e0")
    if 0x4bbb50 <= a <= 0x4c62f0:
        return ("items", 0.65,
                "run 0x4bbb50-0x4c62f0: cea-pdb item_update ('ground point'), projectile_detonate ('gravity'), trigger_create_projectiles ('primary/secondary trigger')")
    if 0x4c6340 <= a <= 0x4ca1a0:
        return ("main", 0.6,
                "run 0x4c6340-0x4ca1a0: '-console'/'-exec'/'-connect', console printf, init.txt+timedemo, main loop, load ui map, screenshots; cea-string-hint main")
    if 0x4ca4b0 <= a <= 0x4cda90:
        return ("math", 0.6,
                "run 0x4ca4b0-0x4cda90: math init 0x4cd3f0 ('-noSSE' -> SSE matrix4x3_multiply dispatch) plus matrix4x3_* and periodic_function_evaluate")
    return ("unknown", 0.2, "no run context")


ov = {
    0x4b1e20: ("interface", 0.7, "sig__spectate_hud_sig: HUD"),
    0x4b3920: ("interface", 0.75, "sig__motion_sensor_update_sig: motion sensor/radar HUD"),
    0x4b2f8a: ("interface", 0.4, "unaligned/misdetected function inside HUD run 0x4b1e20-0x4b4120"),
    0x4bab50: ("networking", 0.3, "boundary stub between server-browser run and video-settings run; no direct evidence"),
    0x4bc5c0: ("items", 0.85, "cea-pdb item_update/item_accelerate via string 'ground point'"),
    0x4bd080: ("items", 0.8, "cea-pdb item_* via string 'ground point'"),
    0x4bd5d0: ("items", 0.8, "cea-pdb item_* via string 'ground point'"),
    0x4c0670: ("items", 0.85, "cea-pdb projectile_detonate via string 'gravity'"),
    0x4c4c40: ("items", 0.85, "cea-pdb trigger_create_projectiles via 'primary trigger'/'secondary trigger'"),
    0x4c1530: ("items", 0.8, "weapon update; references '~primary-blur' function-name string"),
    0x4c62f0: ("items", 0.8, "seed weapon_prevents_grenade_throwing (items 0.8) at end of weapons run"),
    0x4c6340: ("main", 0.5, "seed name weapon_get_first_person_animation_time contradicted: body only __stricmp's '-console' from argv; starts the command-line/main run"),
    0x4c6390: ("main", 0.75, "sig__exec_init_sig: '-exec','init.txt','map_name b30'; cea-string-hint main"),
    0x4c6f30: ("main", 0.65, "timedemo: 'timedemo.txt','map_name b30/c10', GetFileVersionInfo; cea-string-hint main"),
    0x4c7610: ("main", 0.7, "main init: 'banned.txt','bungie.bik','gearbox.bik','levels\\b30\\b30'; cea-string-hint main(4)"),
    0x4c7f10: ("main", 0.6, "seed name weapon_stop_reload contradicted: MsgWaitForMultipleObjects/QPC main loop calling checkpoint + timedemo"),
    0x4c8930: ("main", 0.7, "sig__load_ui_map_sig: 'levels\\ui\\ui'"),
    0x4c9a70: ("main", 0.65, "sig__checkpoint_func_sig: 'gave up trying to save','unsafe save'; cea-string-hint main(2)"),
    0x4c9c60: ("main", 0.45, "seed console_response_printf (interface 0.8); kept in the main/console.c run that holds the console printf fns 0x4c67c0-0x4c6920"),
    0x4c9dc0: ("main", 0.45, "seed console_process_command (interface 0.8); same main/console.c run"),
    0x4ca1a0: ("main", 0.65, "screenshot: 'screenshots','%s\\%dscreenshot%d%d.tga'; cea-string-hint main"),
    0x4ca4b0: ("math", 0.5, "allocates n*n geometry tables; reached only via math init chain 0x4cd3f0 -> 0x4cd0e0"),
    0x4caff0: ("math", 0.45, "float plane/polygon routine in the math-init block reached from 0x4cd3f0"),
    0x4cb7a0: ("math", 0.85, "sig__matrix4x3_inverse_sig"),
    0x4cbde0: ("math", 0.85, "sig__matrix4x3_transform_point_sig"),
    0x4cbec0: ("math", 0.85, "sig__matrix4x3_transform_normal_sig"),
    0x4cc0d0: ("math", 0.85, "sig__matrix4x3_multiply_sig"),
    0x4cc9b0: ("math", 0.75, "sig__periodic_function_evaluate_sig"),
    0x4cd3f0: ("math", 0.8, "math initialize: '-noSSE' selects the SSE/3DNow matrix4x3_multiply implementation"),
}

out = []
for s in addrs:
    a = int(s, 16)
    m, c, e = ov.get(a) or module_for(a)
    out.append({"addr": s, "module": m, "confidence": c, "evidence": e})

notes = (
    "Seven contiguous runs. cea-pdb string anchors fix 0x4bbb50-0x4c62f0 as items "
    "(item_update/item_accelerate via 'ground point', projectile_detonate via 'gravity', "
    "trigger_create_projectiles via 'primary trigger'/'secondary trigger'). 0x4cd3f0 is math_initialize "
    "('-noSSE' picking an SSE matrix4x3_multiply) and it calls 0x4cc8d0 and 0x4cd0e0 -> 0x4ca4b0, which pins "
    "the whole 0x4ca4b0-0x4cda90 tail as math. Four 0.8 seeds were overridden as contradicted by local evidence: "
    "0x4c6340 (named weapon_get_first_person_animation_time but only compares argv against '-console'), "
    "0x4c7f10 (named weapon_stop_reload but is the MsgWaitForMultipleObjects main loop calling the checkpoint and "
    "timedemo functions), and the 11- and 4-byte console stubs 0x4c9c60/0x4c9dc0, which sit inside the main/console.c "
    "run that also contains the console printf helpers at 0x4c67c0-0x4c6920. Lowest-confidence call is 0x4b5d70-0x4bab50: "
    "it is unambiguously the multiplayer server browser (GameSpy serverbrowsing SDK at 0x616f80-0x617c10 plus keys "
    "hostname/mapname/gametype/numplayers/fraglimit/gamevariant) but it also drives the "
    "ui\\shell\\main_menu\\multiplayer_type_select\\join_game widgets, so 'interface' is a plausible alternative to "
    "'networking'. Similarly 0x4baba0-0x4bb640 (resolution list, refresh rate, gamma) could be rasterizer; it was called "
    "interface because the whole chain is reachable only from the video-settings ui callback at 0x4bb5e0."
)

res = {
    "slice": 6,
    "assignments": out,
    "boundaries": [
        {"addr": "0x4b43c0", "note": "HUD/motion-sensor code ends; UI player-profile/text-widget file begins"},
        {"addr": "0x4b5d70", "note": "UI widget file ends; GameSpy server-browser / join-game file begins (mutex + 0x617xxx gamespy calls)"},
        {"addr": "0x4baba0", "note": "server browser ends; video settings (resolution/refresh/gamma) begins ('-vidmode')"},
        {"addr": "0x4bbb50", "note": "video settings ends; items run begins (object globals 0x8603b0/0x87bc14, 'ground point')"},
        {"addr": "0x4c6340", "note": "items/weapons run ends; main.c/console.c run begins ('-console','-exec'); two items seeds here are stale"},
        {"addr": "0x4ca4b0", "note": "main run ends; math library run begins (reachable only from math init 0x4cd3f0)"},
        {"addr": "0x4cda90", "note": "slice ends mid-math run; slice 07 continues with the same small vector/matrix helpers"},
    ],
    "notes": notes,
}

os.makedirs(r'C:\Users\Liam-\halo-re\out\phase1', exist_ok=True)
with open(r'C:\Users\Liam-\halo-re\out\phase1\assign_06.json', 'w', encoding='utf-8') as f:
    json.dump(res, f, indent=1)

from collections import Counter
print(Counter(x['module'] for x in out))
print(len(out), len(set(x['addr'] for x in out)))
