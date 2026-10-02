// sound_class_set_gain_by_name  (Ghidra: sound_class_set_gain_by_name, already named)
// address 0x545390, size 140 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md summary "Sets the gain and countdown timer for a sound
//   class identified by name, e.g. the projectile impact/detonation classes."; sound_class_names
//   (types/sound.h, 0x0069f3a8) walked 51 entries (k_maximum_sound_classes); strstr is the
//   CRT strstr, per src/interface/console_printf_verbose.c's own header note ("_strstr
//   (strstr)") and src/game/cheat_spawn_warthog.c's haystack/needle usage -- called here as
//   strstr(class_name, name), so this matches every class whose name *contains* `name` as a
//   substring, not only an exact match. Writes sound_class_gain.target_gain/fade_ticks
//   (0x00/0x08, types/sound.h), clamping gain to [0, 1] and ticks to >= 0.
// register convention: plain __cdecl with three recognized stack parameters (name, gain, ticks).
// blam-cc: stack -> (name, gain, ticks)
// Phase-4 review: 0x00746140 is a pointer to the class gain table (the code loads it with
// `mov reg, [0x746140]` and indexes from there); the draft declared the table itself there.

#include "tags.h"
#include "memory.h"
#include "sound.h"
#include <string.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern char *sound_class_names[k_maximum_sound_classes]; // 0x0069f3a8
extern sound_class_gain *sound_class_gains; // 0x00746140 holds a pointer to the 51-entry table (mov eax, [0x746140])

// blam-cc: stack -> (name, gain, ticks)
// Sets target_gain (clamped [0, 1]) and fade_ticks (clamped >= 0) on every sound class whose name
// contains `name` as a substring.
void sound_class_set_gain_by_name(char *name, float gain, int16_t ticks)
{
    int32_t i;

    for (i = 0; i < k_maximum_sound_classes; i++) {
        if (sound_class_names[i][0] != '\0' && strstr(sound_class_names[i], name) != 0) {
            float clamped_gain;
            int16_t clamped_ticks;

            if (gain < 0.0f) {
                clamped_gain = 0.0f;
            } else if (gain > 1.0f) {
                clamped_gain = 1.0f;
            } else {
                clamped_gain = gain;
            }

            clamped_ticks = (ticks < 0) ? 0 : ticks;

            sound_class_gains[i].target_gain = clamped_gain;
            sound_class_gains[i].fade_ticks = clamped_ticks;
        }
    }
}

#if 0
Original Ghidra decompilation (0x545390):

void sound_class_set_gain_by_name(undefined4 param_1,float param_2,short param_3)

{
  float fVar1;
  int iVar2;
  short sVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  undefined **ppuVar7;

  iVar6 = 0;
  ppuVar7 = &PTR_s_projectile_impact_0069f3a8;
  iVar5 = 0x33;
  do {
    if ((**ppuVar7 != '\0') &&
       (iVar4 = FUN_00625430(*ppuVar7,param_1), iVar2 = DAT_00746140, iVar4 != 0)) {
      if (0.0 <= param_2) {
        fVar1 = param_2;
        if (1.0 < param_2) {
          fVar1 = 1.0;
        }
      }
      else {
        fVar1 = 0.0;
      }
      *(float *)(iVar6 + DAT_00746140) = fVar1;
      sVar3 = param_3;
      if (param_3 < 0) {
        sVar3 = 0;
      }
      *(short *)(iVar6 + 8 + iVar2) = sVar3;
    }
    ppuVar7 = ppuVar7 + 1;
    iVar6 = iVar6 + 0xc;
    iVar5 = iVar5 + -1;
  } while (iVar5 != 0);
  return;
}

Disassembly (0x545390..0x54541c, capstone; phase-4 review):

0x545390: push ebx
0x545391: push ebp
0x545392: mov bp, word ptr [esp + 0x14]
0x545397: push esi
0x545398: push edi
0x545399: xor esi, esi
0x54539b: mov edi, 0x69f3a8
0x5453a0: mov ebx, 0x33
0x5453a5: mov eax, dword ptr [edi]
0x5453a7: cmp byte ptr [eax], 0
0x5453aa: je 0x54540e
0x5453ac: mov ecx, dword ptr [esp + 0x14]
0x5453b0: push ecx
0x5453b1: push eax
0x5453b2: call 0x625430
0x5453b7: add esp, 8
0x5453ba: test eax, eax
0x5453bc: je 0x54540e
0x5453be: fld dword ptr [esp + 0x18]
0x5453c2: mov ecx, dword ptr [0x746140]
0x5453c8: fcomp dword ptr [0x672ac0]
0x5453ce: fnstsw ax
0x5453d0: test ah, 5
0x5453d3: jp 0x5453dd
0x5453d5: fld dword ptr [0x672ac0]
0x5453db: jmp 0x5453fa
0x5453dd: fld dword ptr [esp + 0x18]
0x5453e1: fcomp dword ptr [0x672ac4]
0x5453e7: fnstsw ax
0x5453e9: test ah, 0x41
0x5453ec: jne 0x5453f6
0x5453ee: fld dword ptr [0x672ac4]
0x5453f4: jmp 0x5453fa
0x5453f6: fld dword ptr [esp + 0x18]
0x5453fa: test bp, bp
0x5453fd: fstp dword ptr [esi + ecx]
0x545400: jge 0x545406
0x545402: xor eax, eax
0x545404: jmp 0x545409
0x545406: movsx eax, bp
0x545409: mov word ptr [esi + ecx + 8], ax
0x54540e: add edi, 4
0x545411: add esi, 0xc
0x545414: dec ebx
0x545415: jne 0x5453a5
0x545417: pop edi
0x545418: pop esi
0x545419: pop ebp
0x54541a: pop ebx
0x54541b: ret 
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
