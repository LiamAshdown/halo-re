// sound_set_effects_gain  (Ghidra: sound_set_effects_gain, already named)
// address 0x5487b0, size 2876 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md summary "Sets the effects sound gain (DAT_007252b0),
//   refreshing per-sound_class mute flags for every effects-related class name."; identical
//   structure to src/sound/sound_set_music_gain.c and src/sound/sound_set_master_gain.c (crossing
//   detection, strstr-based class mute/unmute, sound_update_active_instances refresh), just
//   against a fixed list of 25 class-name substrings instead of one. The list is IDENTICAL
//   between the mute pass (crossing to silent) and the unmute pass (crossing to audible) except
//   at position 23: the mute pass matches "sound_class_ambient_computer" (singular) while the
//   unmute pass matches "sound_class_ambient_computers" (plural) -- both literals are present in
//   this function's own strings-referenced list, so this asymmetry is preserved exactly rather
//   than normalized to one spelling.
// register convention: plain __cdecl, gain as the recognized stack parameter (param_1).
// blam-cc: stack -> gain
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found.

#include "tags.h"
#include "memory.h"
#include "sound.h"
#include <string.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern float sound_effects_gain; // 0x007252b0
extern char *sound_class_names[k_maximum_sound_classes]; // 0x0069f3a8
extern sound_class_definition sound_class_definitions[k_maximum_sound_classes]; // 0x0069eae0

extern void sound_update_active_instances(void); // 0x54c900

static const char *const k_effects_class_substrings_mute[25] = {
    "sound_class_projectile_impact", "sound_class_projectile_detonation", "weapon_fire",
    "sound_class_weapon_ready", "sound_class_weapon_reload", "sound_class_weapon_empty",
    "sound_class_weapon_charge", "sound_class_weapon_overheat", "sound_class_weapon_idle",
    "sound_class_object_impacts", "sound_class_particle_impacts", "sound_class_slow_impacts",
    "sound_class_footstep", "sound_class_vehicle_impact", "sound_class_vehicle_engine",
    "sound_class_device_door", "sound_class_device_force_field", "sound_class_device_machinery",
    "sound_class_device_nature", "sound_class_device_computers", "sound_class_ambient_nature",
    "sound_class_ambient_machinery", "sound_class_ambient_computer", "sound_class_player_hurt",
    "sound_class_game_event"
};

static const char *const k_effects_class_substrings_unmute[25] = {
    "sound_class_projectile_impact", "sound_class_projectile_detonation", "weapon_fire",
    "sound_class_weapon_ready", "sound_class_weapon_reload", "sound_class_weapon_empty",
    "sound_class_weapon_charge", "sound_class_weapon_overheat", "sound_class_weapon_idle",
    "sound_class_object_impacts", "sound_class_particle_impacts", "sound_class_slow_impacts",
    "sound_class_footstep", "sound_class_vehicle_impact", "sound_class_vehicle_engine",
    "sound_class_device_door", "sound_class_device_force_field", "sound_class_device_machinery",
    "sound_class_device_nature", "sound_class_device_computers", "sound_class_ambient_nature",
    "sound_class_ambient_machinery", "sound_class_ambient_computers", "sound_class_player_hurt",
    "sound_class_game_event"
};

static void sound_set_effects_class_muted(const char *const *substrings, int32_t count, uint8_t muted)
{
    int32_t s;

    for (s = 0; s < count; s++) {
        int32_t i;
        for (i = 0; i < k_maximum_sound_classes; i++) {
            if (sound_class_names[i][0] != '\0' && strstr(sound_class_names[i], substrings[s]) != 0) {
                sound_class_definitions[i].muted = muted;
            }
        }
    }
}

// blam-cc: stack -> gain
// Sets the effects-class gain slider. Crossing from audible to silent mutes every
// effects-related sound class (see file header for the 25-entry list) and zeroes the gain;
// crossing from silent to audible unmutes them and clamps the gain to at most 1.0; otherwise the
// gain is set directly. Any actual change refreshes every active playing instance's gain.
void sound_set_effects_gain(float gain)
{
    if (gain == sound_effects_gain) {
        return;
    }

    if (sound_effects_gain > 0.0f && gain <= 0.0f) {
        sound_set_effects_class_muted(k_effects_class_substrings_mute, 25, 1);
        sound_effects_gain = 0.0f;
        sound_update_active_instances();
        return;
    }

    if (sound_effects_gain != 0.0f || gain <= 0.0f) {
        sound_effects_gain = gain;
        sound_update_active_instances();
        return;
    }

    sound_set_effects_class_muted(k_effects_class_substrings_unmute, 25, 0);

    sound_effects_gain = (gain < 1.0f) ? gain : 1.0f;
    sound_update_active_instances();
}

#if 0
Original Ghidra decompilation (0x5487b0) -- structure only (778-line pack file has the full
25-substring x 2-pass listing; see out/phase2/sound/00.md or tools/pack.py 0x5487b0 for the
verbatim decompile). Each pass repeats this shape 25 times with a different literal each time:

void sound_set_effects_gain(float param_1)

{
  int iVar1;
  int iVar2;
  undefined **ppuVar3;
  undefined1 *puVar4;

  if (param_1 == DAT_007252b0) {
    return;
  }
  if ((0.0 < DAT_007252b0) && (param_1 < 0.0 != (param_1 == 0.0))) {
    puVar4 = &DAT_0069eb08;
    ppuVar3 = &PTR_s_projectile_impact_0069f3a8;
    iVar2 = 0x33;
    do {
      if (**ppuVar3 != '\0') {
        iVar1 = FUN_00625430(*ppuVar3,"sound_class_projectile_impact");
        if (iVar1 != 0) {
          *puVar4 = 1;
        }
      }
      ppuVar3 = ppuVar3 + 1;
      puVar4 = puVar4 + 0x2c;
      iVar2 = iVar2 + -1;
    } while (iVar2 != 0);
    /* ... repeated for sound_class_projectile_detonation, weapon_fire, sound_class_weapon_ready,
       sound_class_weapon_reload, sound_class_weapon_empty, sound_class_weapon_charge,
       sound_class_weapon_overheat, sound_class_weapon_idle, sound_class_object_impacts,
       sound_class_particle_impacts, sound_class_slow_impacts, sound_class_footstep,
       sound_class_vehicle_impact, sound_class_vehicle_engine, sound_class_device_door,
       sound_class_device_force_field, sound_class_device_machinery, sound_class_device_nature,
       sound_class_device_computers, sound_class_ambient_nature, sound_class_ambient_machinery,
       sound_class_ambient_computer, sound_class_player_hurt, sound_class_game_event -- 25 total,
       each writing *puVar4 = 1 */
    DAT_007252b0 = 0.0;
    sound_update_active_instances();
    return;
  }
  if ((DAT_007252b0 != 0.0) || (param_1 <= 0.0)) {
    DAT_007252b0 = param_1;
    sound_update_active_instances();
    return;
  }
  /* the same 25-class walk again, writing *puVar4 = 0, with "sound_class_ambient_computers"
     (plural) in place of "sound_class_ambient_computer" at that one position -- see file header */
  if (param_1 < 1.0) {
    DAT_007252b0 = param_1;
    sound_update_active_instances();
    return;
  }
  DAT_007252b0 = 1.0;
  sound_update_active_instances();
  return;
}

Disassembly (0x5487b0..0x5492ec, capstone; phase-4 review):

0x5487b0: fld dword ptr [0x7252b0]
0x5487b6: fld dword ptr [esp + 4]
0x5487ba: fucompp 
0x5487bc: fnstsw ax
0x5487be: test ah, 0x44
0x5487c1: jnp 0x5492eb
0x5487c7: fld dword ptr [0x7252b0]
0x5487cd: push ebx
0x5487ce: fcomp dword ptr [0x672ac0]
0x5487d4: push esi
0x5487d5: push edi
0x5487d6: fnstsw ax
0x5487d8: test ah, 0x41
0x5487db: jne 0x548d3f
0x5487e1: fld dword ptr [esp + 0x10]
0x5487e5: fcomp dword ptr [0x672ac0]
0x5487eb: fnstsw ax
0x5487ed: test ah, 0x41
0x5487f0: jp 0x548d3f
0x5487f6: mov edi, 0x69eb08
0x5487fb: mov esi, 0x69f3a8
0x548800: mov ebx, 0x33
0x548805: mov eax, dword ptr [esi]
0x548807: cmp byte ptr [eax], 0
0x54880a: je 0x548821
0x54880c: push 0x671c64
0x548811: push eax
0x548812: call 0x625430
0x548817: add esp, 8
0x54881a: test eax, eax
0x54881c: je 0x548821
0x54881e: mov byte ptr [edi], 1
0x548821: add esi, 4
0x548824: add edi, 0x2c
0x548827: dec ebx
0x548828: jne 0x548805
0x54882a: mov edi, 0x69eb08
0x54882f: mov esi, 0x69f3a8
0x548834: mov ebx, 0x33
0x548839: lea esp, [esp]
0x548840: mov eax, dword ptr [esi]
0x548842: cmp byte ptr [eax], 0
0x548845: je 0x54885c
0x548847: push 0x671c40
0x54884c: push eax
0x54884d: call 0x625430
0x548852: add esp, 8
0x548855: test eax, eax
0x548857: je 0x54885c
0x548859: mov byte ptr [edi], 1
0x54885c: add esi, 4
0x54885f: add edi, 0x2c
0x548862: dec ebx
0x548863: jne 0x548840
0x548865: mov edi, 0x69eb08
0x54886a: mov esi, 0x69f3a8
0x54886f: mov ebx, 0x33
0x548874: mov eax, dword ptr [esi]
0x548876: cmp byte ptr [eax], 0
0x548879: je 0x548890
0x54887b: push 0x6718a8
0x548880: push eax
0x548881: call 0x625430
0x548886: add esp, 8
0x548889: test eax, eax
0x54888b: je 0x548890
0x54888d: mov byte ptr [edi], 1
0x548890: add esi, 4
0x548893: add edi, 0x2c
0x548896: dec ebx
0x548897: jne 0x548874
0x548899: mov edi, 0x69eb08
0x54889e: mov esi, 0x69f3a8
0x5488a3: mov ebx, 0x33
0x5488a8: mov eax, dword ptr [esi]
0x5488aa: cmp byte ptr [eax], 0
0x5488ad: je 0x5488c4
0x5488af: push 0x671c24
0x5488b4: push eax
0x5488b5: call 0x625430
0x5488ba: add esp, 8
0x5488bd: test eax, eax
0x5488bf: je 0x5488c4
0x5488c1: mov byte ptr [edi], 1
0x5488c4: add esi, 4
0x5488c7: add edi, 0x2c
0x5488ca: dec ebx
0x5488cb: jne 0x5488a8
0x5488cd: mov edi, 0x69eb08
0x5488d2: mov esi, 0x69f3a8
0x5488d7: mov ebx, 0x33
0x5488dc: lea esp, [esp]
0x5488e0: mov eax, dword ptr [esi]
0x5488e2: cmp byte ptr [eax], 0
0x5488e5: je 0x5488fc
0x5488e7: push 0x671c08
0x5488ec: push eax
0x5488ed: call 0x625430
0x5488f2: add esp, 8
0x5488f5: test eax, eax
0x5488f7: je 0x5488fc
0x5488f9: mov byte ptr [edi], 1
0x5488fc: add esi, 4
0x5488ff: add edi, 0x2c
0x548902: dec ebx
0x548903: jne 0x5488e0
0x548905: mov edi, 0x69eb08
0x54890a: mov esi, 0x69f3a8
0x54890f: mov ebx, 0x33
0x548914: mov eax, dword ptr [esi]
0x548916: cmp byte ptr [eax], 0
0x548919: je 0x548930
0x54891b: push 0x671bec
0x548920: push eax
0x548921: call 0x625430
0x548926: add esp, 8
0x548929: test eax, eax
0x54892b: je 0x548930
0x54892d: mov byte ptr [edi], 1
0x548930: add esi, 4
0x548933: add edi, 0x2c
0x548936: dec ebx
0x548937: jne 0x548914
0x548939: mov edi, 0x69eb08
0x54893e: mov esi, 0x69f3a8
0x548943: mov ebx, 0x33
0x548948: mov eax, dword ptr [esi]
0x54894a: cmp byte ptr [eax], 0
0x54894d: je 0x548964
0x54894f: push 0x671bd0
0x548954: push eax
0x548955: call 0x625430
0x54895a: add esp, 8
0x54895d: test eax, eax
0x54895f: je 0x548964
0x548961: mov byte ptr [edi], 1
0x548964: add esi, 4
0x548967: add edi, 0x2c
0x54896a: dec ebx
0x54896b: jne 0x548948
0x54896d: mov edi, 0x69eb08
0x548972: mov esi, 0x69f3a8
0x548977: mov ebx, 0x33
0x54897c: lea esp, [esp]
0x548980: mov eax, dword ptr [esi]
0x548982: cmp byte ptr [eax], 0
0x548985: je 0x54899c
0x548987: push 0x671bb4
0x54898c: push eax
0x54898d: call 0x625430
0x548992: add esp, 8
0x548995: test eax, eax
0x548997: je 0x54899c
0x548999: mov byte ptr [edi], 1
0x54899c: add esi, 4
0x54899f: add edi, 0x2c
0x5489a2: dec ebx
0x5489a3: jne 0x548980
0x5489a5: mov edi, 0x69eb08
0x5489aa: mov esi, 0x69f3a8
0x5489af: mov ebx, 0x33
0x5489b4: mov eax, dword ptr [esi]
0x5489b6: cmp byte ptr [eax], 0
0x5489b9: je 0x5489d0
0x5489bb: push 0x671b9c
0x5489c0: push eax
0x5489c1: call 0x625430
0x5489c6: add esp, 8
0x5489c9: test eax, eax
0x5489cb: je 0x5489d0
0x5489cd: mov byte ptr [edi], 1
0x5489d0: add esi, 4
0x5489d3: add edi, 0x2c
0x5489d6: dec ebx
0x5489d7: jne 0x5489b4
0x5489d9: mov edi, 0x69eb08
0x5489de: mov esi, 0x69f3a8
0x5489e3: mov ebx, 0x33
0x5489e8: mov eax, dword ptr [esi]
0x5489ea: cmp byte ptr [eax], 0
0x5489ed: je 0x548a04
0x5489ef: push 0x671b80
0x5489f4: push eax
0x5489f5: call 0x625430
0x5489fa: add esp, 8
0x5489fd: test eax, eax
0x5489ff: je 0x548a04
0x548a01: mov byte ptr [edi], 1
0x548a04: add esi, 4
0x548a07: add edi, 0x2c
0x548a0a: dec ebx
0x548a0b: jne 0x5489e8
0x548a0d: mov edi, 0x69eb08
0x548a12: mov esi, 0x69f3a8
0x548a17: mov ebx, 0x33
0x548a1c: lea esp, [esp]
0x548a20: mov eax, dword ptr [esi]
0x548a22: cmp byte ptr [eax], 0
0x548a25: je 0x548a3c
0x548a27: push 0x671b60
0x548a2c: push eax
0x548a2d: call 0x625430
0x548a32: add esp, 8
0x548a35: test eax, eax
0x548a37: je 0x548a3c
0x548a39: mov byte ptr [edi], 1
0x548a3c: add esi, 4
0x548a3f: add edi, 0x2c
0x548a42: dec ebx
0x548a43: jne 0x548a20
0x548a45: mov edi, 0x69eb08
0x548a4a: mov esi, 0x69f3a8
0x548a4f: mov ebx, 0x33
0x548a54: mov eax, dword ptr [esi]
0x548a56: cmp byte ptr [eax], 0
0x548a59: je 0x548a70
0x548a5b: push 0x671b44
0x548a60: push eax
0x548a61: call 0x625430
0x548a66: add esp, 8
0x548a69: test eax, eax
0x548a6b: je 0x548a70
0x548a6d: mov byte ptr [edi], 1
0x548a70: add esi, 4
0x548a73: add edi, 0x2c
0x548a76: dec ebx
0x548a77: jne 0x548a54
0x548a79: mov edi, 0x69eb08
0x548a7e: mov esi, 0x69f3a8
0x548a83: mov ebx, 0x33
0x548a88: mov eax, dword ptr [esi]
0x548a8a: cmp byte ptr [eax], 0
0x548a8d: je 0x548aa4
0x548a8f: push 0x671b2c
0x548a94: push eax
0x548a95: call 0x625430
0x548a9a: add esp, 8
0x548a9d: test eax, eax
0x548a9f: je 0x548aa4
0x548aa1: mov byte ptr [edi], 1
0x548aa4: add esi, 4
0x548aa7: add edi, 0x2c
0x548aaa: dec ebx
0x548aab: jne 0x548a88
0x548aad: mov edi, 0x69eb08
0x548ab2: mov esi, 0x69f3a8
0x548ab7: mov ebx, 0x33
0x548abc: lea esp, [esp]
0x548ac0: mov eax, dword ptr [esi]
0x548ac2: cmp byte ptr [eax], 0
0x548ac5: je 0x548adc
0x548ac7: push 0x671b10
0x548acc: push eax
0x548acd: call 0x625430
0x548ad2: add esp, 8
0x548ad5: test eax, eax
0x548ad7: je 0x548adc
0x548ad9: mov byte ptr [edi], 1
0x548adc: add esi, 4
0x548adf: add edi, 0x2c
0x548ae2: dec ebx
0x548ae3: jne 0x548ac0
0x548ae5: mov edi, 0x69eb08
0x548aea: mov esi, 0x69f3a8
0x548aef: mov ebx, 0x33
0x548af4: mov eax, dword ptr [esi]
0x548af6: cmp byte ptr [eax], 0
0x548af9: je 0x548b10
0x548afb: push 0x671af4
0x548b00: push eax
0x548b01: call 0x625430
0x548b06: add esp, 8
0x548b09: test eax, eax
0x548b0b: je 0x548b10
0x548b0d: mov byte ptr [edi], 1
0x548b10: add esi, 4
0x548b13: add edi, 0x2c
0x548b16: dec ebx
0x548b17: jne 0x548af4
0x548b19: mov edi, 0x69eb08
0x548b1e: mov esi, 0x69f3a8
0x548b23: mov ebx, 0x33
0x548b28: mov eax, dword ptr [esi]
0x548b2a: cmp byte ptr [eax], 0
0x548b2d: je 0x548b44
0x548b2f: push 0x671adc
0x548b34: push eax
0x548b35: call 0x625430
0x548b3a: add esp, 8
0x548b3d: test eax, eax
0x548b3f: je 0x548b44
0x548b41: mov byte ptr [edi], 1
0x548b44: add esi, 4
0x548b47: add edi, 0x2c
0x548b4a: dec ebx
0x548b4b: jne 0x548b28
0x548b4d: mov edi, 0x69eb08
0x548b52: mov esi, 0x69f3a8
0x548b57: mov ebx, 0x33
0x548b5c: lea esp, [esp]
0x548b60: mov eax, dword ptr [esi]
0x548b62: cmp byte ptr [eax], 0
0x548b65: je 0x548b7c
0x548b67: push 0x671abc
0x548b6c: push eax
0x548b6d: call 0x625430
0x548b72: add esp, 8
0x548b75: test eax, eax
0x548b77: je 0x548b7c
0x548b79: mov byte ptr [edi], 1
0x548b7c: add esi, 4
0x548b7f: add edi, 0x2c
0x548b82: dec ebx
0x548b83: jne 0x548b60
0x548b85: mov edi, 0x69eb08
0x548b8a: mov esi, 0x69f3a8
0x548b8f: mov ebx, 0x33
0x548b94: mov eax, dword ptr [esi]
0x548b96: cmp byte ptr [eax], 0
0x548b99: je 0x548bb0
0x548b9b: push 0x671a9c
0x548ba0: push eax
0x548ba1: call 0x625430
0x548ba6: add esp, 8
0x548ba9: test eax, eax
0x548bab: je 0x548bb0
0x548bad: mov byte ptr [edi], 1
0x548bb0: add esi, 4
0x548bb3: add edi, 0x2c
0x548bb6: dec ebx
0x548bb7: jne 0x548b94
0x548bb9: mov edi, 0x69eb08
0x548bbe: mov esi, 0x69f3a8
0x548bc3: mov ebx, 0x33
0x548bc8: mov eax, dword ptr [esi]
0x548bca: cmp byte ptr [eax], 0
0x548bcd: je 0x548be4
0x548bcf: push 0x671a80
0x548bd4: push eax
0x548bd5: call 0x625430
0x548bda: add esp, 8
0x548bdd: test eax, eax
0x548bdf: je 0x548be4
0x548be1: mov byte ptr [edi], 1
0x548be4: add esi, 4
0x548be7: add edi, 0x2c
0x548bea: dec ebx
0x548beb: jne 0x548bc8
0x548bed: mov edi, 0x69eb08
0x548bf2: mov esi, 0x69f3a8
0x548bf7: mov ebx, 0x33
0x548bfc: lea esp, [esp]
0x548c00: mov eax, dword ptr [esi]
0x548c02: cmp byte ptr [eax], 0
0x548c05: je 0x548c1c
0x548c07: push 0x671a60
0x548c0c: push eax
0x548c0d: call 0x625430
0x548c12: add esp, 8
0x548c15: test eax, eax
0x548c17: je 0x548c1c
0x548c19: mov byte ptr [edi], 1
0x548c1c: add esi, 4
0x548c1f: add edi, 0x2c
0x548c22: dec ebx
0x548c23: jne 0x548c00
0x548c25: mov edi, 0x69eb08
0x548c2a: mov esi, 0x69f3a8
0x548c2f: mov ebx, 0x33
0x548c34: mov eax, dword ptr [esi]
0x548c36: cmp byte ptr [eax], 0
0x548c39: je 0x548c50
0x548c3b: push 0x671a44
0x548c40: push eax
0x548c41: call 0x625430
0x548c46: add esp, 8
0x548c49: test eax, eax
0x548c4b: je 0x548c50
0x548c4d: mov byte ptr [edi], 1
0x548c50: add esi, 4
0x548c53: add edi, 0x2c
0x548c56: dec ebx
0x548c57: jne 0x548c34
0x548c59: mov edi, 0x69eb08
0x548c5e: mov esi, 0x69f3a8
0x548c63: mov ebx, 0x33
0x548c68: mov eax, dword ptr [esi]
0x548c6a: cmp byte ptr [eax], 0
0x548c6d: je 0x548c84
0x548c6f: push 0x671a24
0x548c74: push eax
0x548c75: call 0x625430
0x548c7a: add esp, 8
0x548c7d: test eax, eax
0x548c7f: je 0x548c84
0x548c81: mov byte ptr [edi], 1
0x548c84: add esi, 4
0x548c87: add edi, 0x2c
0x548c8a: dec ebx
0x548c8b: jne 0x548c68
0x548c8d: mov edi, 0x69eb08
0x548c92: mov esi, 0x69f3a8
0x548c97: mov ebx, 0x33
0x548c9c: lea esp, [esp]
0x548ca0: mov eax, dword ptr [esi]
0x548ca2: cmp byte ptr [eax], 0
0x548ca5: je 0x548cbc
0x548ca7: push 0x671a04
0x548cac: push eax
0x548cad: call 0x625430
0x548cb2: add esp, 8
0x548cb5: test eax, eax
0x548cb7: je 0x548cbc
0x548cb9: mov byte ptr [edi], 1
0x548cbc: add esi, 4
0x548cbf: add edi, 0x2c
0x548cc2: dec ebx
0x548cc3: jne 0x548ca0
0x548cc5: mov edi, 0x69eb08
0x548cca: mov esi, 0x69f3a8
0x548ccf: mov ebx, 0x33
0x548cd4: mov eax, dword ptr [esi]
0x548cd6: cmp byte ptr [eax], 0
0x548cd9: je 0x548cf0
0x548cdb: push 0x6719ec
0x548ce0: push eax
0x548ce1: call 0x625430
0x548ce6: add esp, 8
0x548ce9: test eax, eax
0x548ceb: je 0x548cf0
0x548ced: mov byte ptr [edi], 1
0x548cf0: add esi, 4
0x548cf3: add edi, 0x2c
0x548cf6: dec ebx
0x548cf7: jne 0x548cd4
0x548cf9: mov edi, 0x69eb08
0x548cfe: mov esi, 0x69f3a8
0x548d03: mov ebx, 0x33
0x548d08: mov eax, dword ptr [esi]
0x548d0a: cmp byte ptr [eax], 0
0x548d0d: je 0x548d24
0x548d0f: push 0x6719d4
0x548d14: push eax
0x548d15: call 0x625430
0x548d1a: add esp, 8
0x548d1d: test eax, eax
0x548d1f: je 0x548d24
0x548d21: mov byte ptr [edi], 1
0x548d24: add esi, 4
0x548d27: add edi, 0x2c
0x548d2a: dec ebx
0x548d2b: jne 0x548d08
0x548d2d: pop edi
0x548d2e: pop esi
0x548d2f: mov dword ptr [0x7252b0], 0
0x548d39: pop ebx
0x548d3a: jmp 0x54c900
0x548d3f: fld dword ptr [0x672ac0]
0x548d45: fld dword ptr [0x7252b0]
0x548d4b: fucompp 
0x548d4d: fnstsw ax
0x548d4f: test ah, 0x44
0x548d52: jp 0x5492d9
0x548d58: fld dword ptr [esp + 0x10]
0x548d5c: fcomp dword ptr [0x672ac0]
0x548d62: fnstsw ax
0x548d64: test ah, 0x41
0x548d67: jne 0x5492d9
0x548d6d: mov edi, 0x69eb08
0x548d72: mov esi, 0x69f3a8
0x548d77: mov ebx, 0x33
0x548d7c: lea esp, [esp]
0x548d80: mov eax, dword ptr [esi]
0x548d82: cmp byte ptr [eax], 0
0x548d85: je 0x548d9c
0x548d87: push 0x671c64
0x548d8c: push eax
0x548d8d: call 0x625430
0x548d92: add esp, 8
0x548d95: test eax, eax
0x548d97: je 0x548d9c
0x548d99: mov byte ptr [edi], 0
0x548d9c: add esi, 4
0x548d9f: add edi, 0x2c
0x548da2: dec ebx
0x548da3: jne 0x548d80
0x548da5: mov edi, 0x69eb08
0x548daa: mov esi, 0x69f3a8
0x548daf: mov ebx, 0x33
0x548db4: mov eax, dword ptr [esi]
0x548db6: cmp byte ptr [eax], 0
0x548db9: je 0x548dd0
0x548dbb: push 0x671c40
0x548dc0: push eax
0x548dc1: call 0x625430
0x548dc6: add esp, 8
0x548dc9: test eax, eax
0x548dcb: je 0x548dd0
0x548dcd: mov byte ptr [edi], 0
0x548dd0: add esi, 4
0x548dd3: add edi, 0x2c
0x548dd6: dec ebx
0x548dd7: jne 0x548db4
0x548dd9: mov edi, 0x69eb08
0x548dde: mov esi, 0x69f3a8
0x548de3: mov ebx, 0x33
0x548de8: mov eax, dword ptr [esi]
0x548dea: cmp byte ptr [eax], 0
0x548ded: je 0x548e04
0x548def: push 0x6718a8
0x548df4: push eax
0x548df5: call 0x625430
0x548dfa: add esp, 8
0x548dfd: test eax, eax
0x548dff: je 0x548e04
0x548e01: mov byte ptr [edi], 0
0x548e04: add esi, 4
0x548e07: add edi, 0x2c
0x548e0a: dec ebx
0x548e0b: jne 0x548de8
0x548e0d: mov edi, 0x69eb08
0x548e12: mov esi, 0x69f3a8
0x548e17: mov ebx, 0x33
0x548e1c: lea esp, [esp]
0x548e20: mov eax, dword ptr [esi]
0x548e22: cmp byte ptr [eax], 0
0x548e25: je 0x548e3c
0x548e27: push 0x671c24
0x548e2c: push eax
0x548e2d: call 0x625430
0x548e32: add esp, 8
0x548e35: test eax, eax
0x548e37: je 0x548e3c
0x548e39: mov byte ptr [edi], 0
0x548e3c: add esi, 4
0x548e3f: add edi, 0x2c
0x548e42: dec ebx
0x548e43: jne 0x548e20
0x548e45: mov edi, 0x69eb08
0x548e4a: mov esi, 0x69f3a8
0x548e4f: mov ebx, 0x33
0x548e54: mov eax, dword ptr [esi]
0x548e56: cmp byte ptr [eax], 0
0x548e59: je 0x548e70
0x548e5b: push 0x671c08
0x548e60: push eax
0x548e61: call 0x625430
0x548e66: add esp, 8
0x548e69: test eax, eax
0x548e6b: je 0x548e70
0x548e6d: mov byte ptr [edi], 0
0x548e70: add esi, 4
0x548e73: add edi, 0x2c
0x548e76: dec ebx
0x548e77: jne 0x548e54
0x548e79: mov edi, 0x69eb08
0x548e7e: mov esi, 0x69f3a8
0x548e83: mov ebx, 0x33
0x548e88: mov eax, dword ptr [esi]
0x548e8a: cmp byte ptr [eax], 0
0x548e8d: je 0x548ea4
0x548e8f: push 0x671bec
0x548e94: push eax
0x548e95: call 0x625430
0x548e9a: add esp, 8
0x548e9d: test eax, eax
0x548e9f: je 0x548ea4
0x548ea1: mov byte ptr [edi], 0
0x548ea4: add esi, 4
0x548ea7: add edi, 0x2c
0x548eaa: dec ebx
0x548eab: jne 0x548e88
0x548ead: mov edi, 0x69eb08
0x548eb2: mov esi, 0x69f3a8
0x548eb7: mov ebx, 0x33
0x548ebc: lea esp, [esp]
0x548ec0: mov eax, dword ptr [esi]
0x548ec2: cmp byte ptr [eax], 0
0x548ec5: je 0x548edc
0x548ec7: push 0x671bd0
0x548ecc: push eax
0x548ecd: call 0x625430
0x548ed2: add esp, 8
0x548ed5: test eax, eax
0x548ed7: je 0x548edc
0x548ed9: mov byte ptr [edi], 0
0x548edc: add esi, 4
0x548edf: add edi, 0x2c
0x548ee2: dec ebx
0x548ee3: jne 0x548ec0
0x548ee5: mov edi, 0x69eb08
0x548eea: mov esi, 0x69f3a8
0x548eef: mov ebx, 0x33
0x548ef4: mov eax, dword ptr [esi]
0x548ef6: cmp byte ptr [eax], 0
0x548ef9: je 0x548f10
0x548efb: push 0x671bb4
0x548f00: push eax
0x548f01: call 0x625430
0x548f06: add esp, 8
0x548f09: test eax, eax
0x548f0b: je 0x548f10
0x548f0d: mov byte ptr [edi], 0
0x548f10: add esi, 4
0x548f13: add edi, 0x2c
0x548f16: dec ebx
0x548f17: jne 0x548ef4
0x548f19: mov edi, 0x69eb08
0x548f1e: mov esi, 0x69f3a8
0x548f23: mov ebx, 0x33
0x548f28: mov eax, dword ptr [esi]
0x548f2a: cmp byte ptr [eax], 0
0x548f2d: je 0x548f44
0x548f2f: push 0x671b9c
0x548f34: push eax
0x548f35: call 0x625430
0x548f3a: add esp, 8
0x548f3d: test eax, eax
0x548f3f: je 0x548f44
0x548f41: mov byte ptr [edi], 0
0x548f44: add esi, 4
0x548f47: add edi, 0x2c
0x548f4a: dec ebx
0x548f4b: jne 0x548f28
0x548f4d: mov edi, 0x69eb08
0x548f52: mov esi, 0x69f3a8
0x548f57: mov ebx, 0x33
0x548f5c: lea esp, [esp]
0x548f60: mov eax, dword ptr [esi]
0x548f62: cmp byte ptr [eax], 0
0x548f65: je 0x548f7c
0x548f67: push 0x671b80
0x548f6c: push eax
0x548f6d: call 0x625430
0x548f72: add esp, 8
0x548f75: test eax, eax
0x548f77: je 0x548f7c
0x548f79: mov byte ptr [edi], 0
0x548f7c: add esi, 4
0x548f7f: add edi, 0x2c
0x548f82: dec ebx
0x548f83: jne 0x548f60
0x548f85: mov edi, 0x69eb08
0x548f8a: mov esi, 0x69f3a8
0x548f8f: mov ebx, 0x33
0x548f94: mov eax, dword ptr [esi]
0x548f96: cmp byte ptr [eax], 0
0x548f99: je 0x548fb0
0x548f9b: push 0x671b60
0x548fa0: push eax
0x548fa1: call 0x625430
0x548fa6: add esp, 8
0x548fa9: test eax, eax
0x548fab: je 0x548fb0
0x548fad: mov byte ptr [edi], 0
0x548fb0: add esi, 4
0x548fb3: add edi, 0x2c
0x548fb6: dec ebx
0x548fb7: jne 0x548f94
0x548fb9: mov edi, 0x69eb08
0x548fbe: mov esi, 0x69f3a8
0x548fc3: mov ebx, 0x33
0x548fc8: mov eax, dword ptr [esi]
0x548fca: cmp byte ptr [eax], 0
0x548fcd: je 0x548fe4
0x548fcf: push 0x671b44
0x548fd4: push eax
0x548fd5: call 0x625430
0x548fda: add esp, 8
0x548fdd: test eax, eax
0x548fdf: je 0x548fe4
0x548fe1: mov byte ptr [edi], 0
0x548fe4: add esi, 4
0x548fe7: add edi, 0x2c
0x548fea: dec ebx
0x548feb: jne 0x548fc8
0x548fed: mov edi, 0x69eb08
0x548ff2: mov esi, 0x69f3a8
0x548ff7: mov ebx, 0x33
0x548ffc: lea esp, [esp]
0x549000: mov eax, dword ptr [esi]
0x549002: cmp byte ptr [eax], 0
0x549005: je 0x54901c
0x549007: push 0x671b2c
0x54900c: push eax
0x54900d: call 0x625430
0x549012: add esp, 8
0x549015: test eax, eax
0x549017: je 0x54901c
0x549019: mov byte ptr [edi], 0
0x54901c: add esi, 4
0x54901f: add edi, 0x2c
0x549022: dec ebx
0x549023: jne 0x549000
0x549025: mov edi, 0x69eb08
0x54902a: mov esi, 0x69f3a8
0x54902f: mov ebx, 0x33
0x549034: mov eax, dword ptr [esi]
0x549036: cmp byte ptr [eax], 0
0x549039: je 0x549050
0x54903b: push 0x671b10
0x549040: push eax
0x549041: call 0x625430
0x549046: add esp, 8
0x549049: test eax, eax
0x54904b: je 0x549050
0x54904d: mov byte ptr [edi], 0
0x549050: add esi, 4
0x549053: add edi, 0x2c
0x549056: dec ebx
0x549057: jne 0x549034
0x549059: mov edi, 0x69eb08
0x54905e: mov esi, 0x69f3a8
0x549063: mov ebx, 0x33
0x549068: mov eax, dword ptr [esi]
0x54906a: cmp byte ptr [eax], 0
0x54906d: je 0x549084
0x54906f: push 0x671af4
0x549074: push eax
0x549075: call 0x625430
0x54907a: add esp, 8
0x54907d: test eax, eax
0x54907f: je 0x549084
0x549081: mov byte ptr [edi], 0
0x549084: add esi, 4
0x549087: add edi, 0x2c
0x54908a: dec ebx
0x54908b: jne 0x549068
0x54908d: mov edi, 0x69eb08
0x549092: mov esi, 0x69f3a8
0x549097: mov ebx, 0x33
0x54909c: lea esp, [esp]
0x5490a0: mov eax, dword ptr [esi]
0x5490a2: cmp byte ptr [eax], 0
0x5490a5: je 0x5490bc
0x5490a7: push 0x671adc
0x5490ac: push eax
0x5490ad: call 0x625430
0x5490b2: add esp, 8
0x5490b5: test eax, eax
0x5490b7: je 0x5490bc
0x5490b9: mov byte ptr [edi], 0
0x5490bc: add esi, 4
0x5490bf: add edi, 0x2c
0x5490c2: dec ebx
0x5490c3: jne 0x5490a0
0x5490c5: mov edi, 0x69eb08
0x5490ca: mov esi, 0x69f3a8
0x5490cf: mov ebx, 0x33
0x5490d4: mov eax, dword ptr [esi]
0x5490d6: cmp byte ptr [eax], 0
0x5490d9: je 0x5490f0
0x5490db: push 0x671abc
0x5490e0: push eax
0x5490e1: call 0x625430
0x5490e6: add esp, 8
0x5490e9: test eax, eax
0x5490eb: je 0x5490f0
0x5490ed: mov byte ptr [edi], 0
0x5490f0: add esi, 4
0x5490f3: add edi, 0x2c
0x5490f6: dec ebx
0x5490f7: jne 0x5490d4
0x5490f9: mov edi, 0x69eb08
0x5490fe: mov esi, 0x69f3a8
0x549103: mov ebx, 0x33
0x549108: mov eax, dword ptr [esi]
0x54910a: cmp byte ptr [eax], 0
0x54910d: je 0x549124
0x54910f: push 0x671a9c
0x549114: push eax
0x549115: call 0x625430
0x54911a: add esp, 8
0x54911d: test eax, eax
0x54911f: je 0x549124
0x549121: mov byte ptr [edi], 0
0x549124: add esi, 4
0x549127: add edi, 0x2c
0x54912a: dec ebx
0x54912b: jne 0x549108
0x54912d: mov edi, 0x69eb08
0x549132: mov esi, 0x69f3a8
0x549137: mov ebx, 0x33
0x54913c: lea esp, [esp]
0x549140: mov eax, dword ptr [esi]
0x549142: cmp byte ptr [eax], 0
0x549145: je 0x54915c
0x549147: push 0x671a80
0x54914c: push eax
0x54914d: call 0x625430
0x549152: add esp, 8
0x549155: test eax, eax
0x549157: je 0x54915c
0x549159: mov byte ptr [edi], 0
0x54915c: add esi, 4
0x54915f: add edi, 0x2c
0x549162: dec ebx
0x549163: jne 0x549140
0x549165: mov edi, 0x69eb08
0x54916a: mov esi, 0x69f3a8
0x54916f: mov ebx, 0x33
0x549174: mov eax, dword ptr [esi]
0x549176: cmp byte ptr [eax], 0
0x549179: je 0x549190
0x54917b: push 0x671a60
0x549180: push eax
0x549181: call 0x625430
0x549186: add esp, 8
0x549189: test eax, eax
0x54918b: je 0x549190
0x54918d: mov byte ptr [edi], 0
0x549190: add esi, 4
0x549193: add edi, 0x2c
0x549196: dec ebx
0x549197: jne 0x549174
0x549199: mov edi, 0x69eb08
0x54919e: mov esi, 0x69f3a8
0x5491a3: mov ebx, 0x33
0x5491a8: mov eax, dword ptr [esi]
0x5491aa: cmp byte ptr [eax], 0
0x5491ad: je 0x5491c4
0x5491af: push 0x671a44
0x5491b4: push eax
0x5491b5: call 0x625430
0x5491ba: add esp, 8
0x5491bd: test eax, eax
0x5491bf: je 0x5491c4
0x5491c1: mov byte ptr [edi], 0
0x5491c4: add esi, 4
0x5491c7: add edi, 0x2c
0x5491ca: dec ebx
0x5491cb: jne 0x5491a8
0x5491cd: mov edi, 0x69eb08
0x5491d2: mov esi, 0x69f3a8
0x5491d7: mov ebx, 0x33
0x5491dc: lea esp, [esp]
0x5491e0: mov eax, dword ptr [esi]
0x5491e2: cmp byte ptr [eax], 0
0x5491e5: je 0x5491fc
0x5491e7: push 0x671a24
0x5491ec: push eax
0x5491ed: call 0x625430
0x5491f2: add esp, 8
0x5491f5: test eax, eax
0x5491f7: je 0x5491fc
0x5491f9: mov byte ptr [edi], 0
0x5491fc: add esi, 4
0x5491ff: add edi, 0x2c
0x549202: dec ebx
0x549203: jne 0x5491e0
0x549205: mov edi, 0x69eb08
0x54920a: mov esi, 0x69f3a8
0x54920f: mov ebx, 0x33
0x549214: mov eax, dword ptr [esi]
0x549216: cmp byte ptr [eax], 0
0x549219: je 0x549230
0x54921b: push 0x6719b4
0x549220: push eax
0x549221: call 0x625430
0x549226: add esp, 8
0x549229: test eax, eax
0x54922b: je 0x549230
0x54922d: mov byte ptr [edi], 0
0x549230: add esi, 4
0x549233: add edi, 0x2c
0x549236: dec ebx
0x549237: jne 0x549214
0x549239: mov edi, 0x69eb08
0x54923e: mov esi, 0x69f3a8
0x549243: mov ebx, 0x33
0x549248: mov eax, dword ptr [esi]
0x54924a: cmp byte ptr [eax], 0
0x54924d: je 0x549264
0x54924f: push 0x6719ec
0x549254: push eax
0x549255: call 0x625430
0x54925a: add esp, 8
0x54925d: test eax, eax
0x54925f: je 0x549264
0x549261: mov byte ptr [edi], 0
0x549264: add esi, 4
0x549267: add edi, 0x2c
0x54926a: dec ebx
0x54926b: jne 0x549248
0x54926d: mov edi, 0x69eb08
0x549272: mov esi, 0x69f3a8
0x549277: mov ebx, 0x33
0x54927c: lea esp, [esp]
0x549280: mov eax, dword ptr [esi]
0x549282: cmp byte ptr [eax], 0
0x549285: je 0x54929c
0x549287: push 0x6719d4
0x54928c: push eax
0x54928d: call 0x625430
0x549292: add esp, 8
0x549295: test eax, eax
0x549297: je 0x54929c
0x549299: mov byte ptr [edi], 0
0x54929c: add esi, 4
0x54929f: add edi, 0x2c
0x5492a2: dec ebx
0x5492a3: jne 0x549280
0x5492a5: fld dword ptr [esp + 0x10]
0x5492a9: fcomp dword ptr [0x672ac4]
0x5492af: fnstsw ax
0x5492b1: test ah, 5
0x5492b4: jp 0x5492c7
0x5492b6: mov eax, dword ptr [esp + 0x10]
0x5492ba: pop edi
0x5492bb: pop esi
0x5492bc: mov dword ptr [0x7252b0], eax
0x5492c1: pop ebx
0x5492c2: jmp 0x54c900
0x5492c7: pop edi
0x5492c8: pop esi
0x5492c9: mov dword ptr [0x7252b0], 0x3f800000
0x5492d3: pop ebx
0x5492d4: jmp 0x54c900
0x5492d9: mov ecx, dword ptr [esp + 0x10]
0x5492dd: pop edi
0x5492de: pop esi
0x5492df: mov dword ptr [0x7252b0], ecx
0x5492e5: pop ebx
0x5492e6: jmp 0x54c900
0x5492eb: ret 
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
