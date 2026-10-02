// sound_class_set_muted_by_name  (Ghidra: FUN_00545420)
// address 0x545420, size 63 bytes
// name confidence: 0.4   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md summary "Sets or clears a per-sound-class boolean flag
//   for the sound class identified by name."; the write target (0x0069eae0 + 0x28, stride 0x2c)
//   is sound_class_definition.muted exactly (types/sound.h); the name match reuses
//   src/sound/sound_class_set_gain_by_name.c's strstr(class_name, name) substring idiom.
// register convention: "enabled" flag in BL (unaff_BL), name as the one stack parameter Ghidra
//   recognizes directly (param_1).
// blam-cc: BL -> enabled, stack -> name
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found.

#include "tags.h"
#include "memory.h"
#include "sound.h"
#include <string.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern char *sound_class_names[k_maximum_sound_classes]; // 0x0069f3a8
extern sound_class_definition sound_class_definitions[k_maximum_sound_classes]; // 0x0069eae0

// blam-cc: BL -> enabled, stack -> name
// Sets (muted = !enabled) on every sound class whose name contains `name` as a substring.
void sound_class_set_muted_by_name(uint8_t enabled, char *name)
{
    int32_t i;

    for (i = 0; i < k_maximum_sound_classes; i++) {
        if (sound_class_names[i][0] != '\0' && strstr(sound_class_names[i], name) != 0) {
            sound_class_definitions[i].muted = (enabled == 0);
        }
    }
}

#if 0
Original Ghidra decompilation (0x545420):

void FUN_00545420(undefined4 param_1)

{
  int iVar1;
  char unaff_BL;
  int iVar2;
  undefined **ppuVar3;
  undefined1 *puVar4;

  puVar4 = &DAT_0069eb08;
  ppuVar3 = &PTR_s_projectile_impact_0069f3a8;
  iVar2 = 0x33;
  do {
    if (**ppuVar3 != '\0') {
      iVar1 = FUN_00625430(*ppuVar3,param_1);
      if (iVar1 != 0) {
        *puVar4 = unaff_BL == '\0';
      }
    }
    ppuVar3 = ppuVar3 + 1;
    puVar4 = puVar4 + 0x2c;
    iVar2 = iVar2 + -1;
  } while (iVar2 != 0);
  return;
}

Disassembly (0x545420..0x54545f, capstone; phase-4 review):

0x545420: push ebp
0x545421: push esi
0x545422: push edi
0x545423: mov edi, 0x69eb08
0x545428: mov esi, 0x69f3a8
0x54542d: mov ebp, 0x33
0x545432: mov eax, dword ptr [esi]
0x545434: cmp byte ptr [eax], 0
0x545437: je 0x545452
0x545439: mov ecx, dword ptr [esp + 0x10]
0x54543d: push ecx
0x54543e: push eax
0x54543f: call 0x625430
0x545444: add esp, 8
0x545447: test eax, eax
0x545449: je 0x545452
0x54544b: test bl, bl
0x54544d: sete dl
0x545450: mov byte ptr [edi], dl
0x545452: add esi, 4
0x545455: add edi, 0x2c
0x545458: dec ebp
0x545459: jne 0x545432
0x54545b: pop edi
0x54545c: pop esi
0x54545d: pop ebp
0x54545e: ret 
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
