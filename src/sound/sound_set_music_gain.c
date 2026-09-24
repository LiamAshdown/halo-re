// sound_set_music_gain  (Ghidra: sound_set_music_gain, already named)
// address 0x548680, size 300 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md summary "Sets the music sound-class gain
//   (DAT_007252a8), toggling a per-class mute flag for the 'music' sound_class when the gain
//   crosses zero."; reuses the same strstr(class_name, "music") substring match as
//   src/sound/sound_class_set_muted_by_name.c and src/sound/sound_class_set_gain_by_name.c, and
//   sound_update_active_instances (0x54c900, already named).
// register convention: plain __cdecl, gain as the recognized stack parameter (param_1).
// blam-cc: stack -> gain
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found.

#include "tags.h"
#include "memory.h"
#include "sound.h"
#include <string.h>

extern float sound_music_gain; // 0x007252a8
extern char *sound_class_names[k_maximum_sound_classes]; // 0x0069f3a8
extern sound_class_definition sound_class_definitions[k_maximum_sound_classes]; // 0x0069eae0

extern void sound_update_active_instances(void); // 0x54c900

// blam-cc: stack -> gain
// Sets the music-class gain slider. Crossing from audible to silent mutes every "music"-named
// sound class and zeroes the gain; crossing from silent to audible unmutes them and clamps the
// gain to at most 1.0; otherwise the gain is set directly. Any actual change refreshes every
// active playing instance's gain.
void sound_set_music_gain(float gain)
{
    int32_t i;

    if (gain == sound_music_gain) {
        return;
    }

    if (sound_music_gain > 0.0f && gain <= 0.0f) {
        for (i = 0; i < k_maximum_sound_classes; i++) {
            if (sound_class_names[i][0] != '\0' && strstr(sound_class_names[i], "music") != 0) {
                sound_class_definitions[i].muted = 1;
            }
        }
        sound_music_gain = 0.0f;
        sound_update_active_instances();
        return;
    }

    if (sound_music_gain != 0.0f || gain <= 0.0f) {
        sound_music_gain = gain;
        sound_update_active_instances();
        return;
    }

    for (i = 0; i < k_maximum_sound_classes; i++) {
        if (sound_class_names[i][0] != '\0' && strstr(sound_class_names[i], "music") != 0) {
            sound_class_definitions[i].muted = 0;
        }
    }

    sound_music_gain = (gain < 1.0f) ? gain : 1.0f;
    sound_update_active_instances();
}

#if 0
Original Ghidra decompilation (0x548680):

void sound_set_music_gain(float param_1)

{
  int iVar1;
  int iVar2;
  undefined **ppuVar3;
  undefined1 *puVar4;

  if (param_1 == DAT_007252a8) {
    return;
  }
  if ((0.0 < DAT_007252a8) && (param_1 < 0.0 != (param_1 == 0.0))) {
    puVar4 = &DAT_0069eb08;
    ppuVar3 = &PTR_s_projectile_impact_0069f3a8;
    iVar2 = 0x33;
    do {
      if (**ppuVar3 != '\0') {
        iVar1 = FUN_00625430(*ppuVar3,"music");
        if (iVar1 != 0) {
          *puVar4 = 1;
        }
      }
      ppuVar3 = ppuVar3 + 1;
      puVar4 = puVar4 + 0x2c;
      iVar2 = iVar2 + -1;
    } while (iVar2 != 0);
    DAT_007252a8 = 0.0;
    sound_update_active_instances();
    return;
  }
  if ((DAT_007252a8 != 0.0) || (param_1 <= 0.0)) {
    DAT_007252a8 = param_1;
    sound_update_active_instances();
    return;
  }
  puVar4 = &DAT_0069eb08;
  ppuVar3 = &PTR_s_projectile_impact_0069f3a8;
  iVar2 = 0x33;
  do {
    if (**ppuVar3 != '\0') {
      iVar1 = FUN_00625430(*ppuVar3,"music");
      if (iVar1 != 0) {
        *puVar4 = 0;
      }
    }
    ppuVar3 = ppuVar3 + 1;
    puVar4 = puVar4 + 0x2c;
    iVar2 = iVar2 + -1;
  } while (iVar2 != 0);
  if (param_1 < 1.0) {
    DAT_007252a8 = param_1;
    sound_update_active_instances();
    return;
  }
  DAT_007252a8 = 1.0;
  sound_update_active_instances();
  return;
}

Disassembly (0x548680..0x5487ac, capstone; phase-4 review):

0x548680: fld dword ptr [0x7252a8]
0x548686: fld dword ptr [esp + 4]
0x54868a: fucompp 
0x54868c: fnstsw ax
0x54868e: test ah, 0x44
0x548691: jnp 0x5487ab
0x548697: fld dword ptr [0x7252a8]
0x54869d: push ebx
0x54869e: fcomp dword ptr [0x672ac0]
0x5486a4: push esi
0x5486a5: push edi
0x5486a6: fnstsw ax
0x5486a8: test ah, 0x41
0x5486ab: jne 0x548707
0x5486ad: fld dword ptr [esp + 0x10]
0x5486b1: fcomp dword ptr [0x672ac0]
0x5486b7: fnstsw ax
0x5486b9: test ah, 0x41
0x5486bc: jp 0x548707
0x5486be: mov edi, 0x69eb08
0x5486c3: mov esi, 0x69f3a8
0x5486c8: mov ebx, 0x33
0x5486cd: lea ecx, [ecx]
0x5486d0: mov eax, dword ptr [esi]
0x5486d2: cmp byte ptr [eax], 0
0x5486d5: je 0x5486ec
0x5486d7: push 0x671770
0x5486dc: push eax
0x5486dd: call 0x625430
0x5486e2: add esp, 8
0x5486e5: test eax, eax
0x5486e7: je 0x5486ec
0x5486e9: mov byte ptr [edi], 1
0x5486ec: add esi, 4
0x5486ef: add edi, 0x2c
0x5486f2: dec ebx
0x5486f3: jne 0x5486d0
0x5486f5: pop edi
0x5486f6: pop esi
0x5486f7: mov dword ptr [0x7252a8], 0
0x548701: pop ebx
0x548702: jmp 0x54c900
0x548707: fld dword ptr [0x672ac0]
0x54870d: fld dword ptr [0x7252a8]
0x548713: fucompp 
0x548715: fnstsw ax
0x548717: test ah, 0x44
0x54871a: jp 0x548799
0x548720: fld dword ptr [esp + 0x10]
0x548724: fcomp dword ptr [0x672ac0]
0x54872a: fnstsw ax
0x54872c: test ah, 0x41
0x54872f: jne 0x548799
0x548731: mov edi, 0x69eb08
0x548736: mov esi, 0x69f3a8
0x54873b: mov ebx, 0x33
0x548740: mov eax, dword ptr [esi]
0x548742: cmp byte ptr [eax], 0
0x548745: je 0x54875c
0x548747: push 0x671770
0x54874c: push eax
0x54874d: call 0x625430
0x548752: add esp, 8
0x548755: test eax, eax
0x548757: je 0x54875c
0x548759: mov byte ptr [edi], 0
0x54875c: add esi, 4
0x54875f: add edi, 0x2c
0x548762: dec ebx
0x548763: jne 0x548740
0x548765: fld dword ptr [esp + 0x10]
0x548769: fcomp dword ptr [0x672ac4]
0x54876f: fnstsw ax
0x548771: test ah, 5
0x548774: jp 0x548787
0x548776: mov eax, dword ptr [esp + 0x10]
0x54877a: pop edi
0x54877b: pop esi
0x54877c: mov dword ptr [0x7252a8], eax
0x548781: pop ebx
0x548782: jmp 0x54c900
0x548787: pop edi
0x548788: pop esi
0x548789: mov dword ptr [0x7252a8], 0x3f800000
0x548793: pop ebx
0x548794: jmp 0x54c900
0x548799: mov ecx, dword ptr [esp + 0x10]
0x54879d: pop edi
0x54879e: pop esi
0x54879f: mov dword ptr [0x7252a8], ecx
0x5487a5: pop ebx
0x5487a6: jmp 0x54c900
0x5487ab: ret 
#endif
