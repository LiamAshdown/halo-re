// sound_class_update_gain_fade  (Ghidra: sound_class_update_gain_fade, already named)
// address 0x545330, size 88 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md summary "Advances the per-sound-class gain fade timers
//   by the given number of ticks, interpolating or snapping each class's gain toward its target.";
//   types/sound.h sound_class_gain (target_gain 0x00, current_gain 0x04, fade_ticks 0x08) and its
//   own comment "0x545330 walks 51 entries at stride 0xc".
// register convention: plain __cdecl, ticks as the recognized stack parameter (param_1).
// blam-cc: stack -> ticks
// Phase-4 review: 0x00746140 is a pointer to the class gain table (the code loads it with
// `mov reg, [0x746140]` and indexes from there); the draft declared the table itself there.

#include "tags.h"
#include "memory.h"
#include "sound.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern sound_class_gain *sound_class_gains; // 0x00746140 holds a pointer to the 51-entry table (mov eax, [0x746140])

// blam-cc: stack -> ticks
// Advances every sound class's gain fade by `ticks`: interpolates current_gain toward
// target_gain proportionally while fade_ticks remains, or snaps directly to target_gain once the
// fade completes.
void sound_class_update_gain_fade(int32_t ticks)
{
    int32_t i;

    if (ticks <= 0) {
        return;
    }

    for (i = 0; i < k_maximum_sound_classes; i++) {
        sound_class_gain *gain = &sound_class_gains[i];

        if (ticks < gain->fade_ticks) {
            int16_t original_ticks = gain->fade_ticks;
            gain->fade_ticks = (int16_t)(original_ticks - ticks);
            gain->current_gain = (gain->target_gain - gain->current_gain) *
                ((float)ticks / (float)original_ticks) + gain->current_gain;
        } else {
            gain->current_gain = gain->target_gain;
            gain->fade_ticks = 0;
        }
    }
}

#if 0
Original Ghidra decompilation (0x545330):

void sound_class_update_gain_fade(int param_1)

{
  short sVar1;
  short *psVar2;
  int iVar3;

  if (0 < param_1) {
    psVar2 = (short *)(DAT_00746140 + 8);
    iVar3 = 0x33;
    do {
      sVar1 = *psVar2;
      if (param_1 < sVar1) {
        *psVar2 = sVar1 - (short)param_1;
        *(float *)(psVar2 + -2) =
             (*(float *)(psVar2 + -4) - *(float *)(psVar2 + -2)) *
             ((float)param_1 / (float)(int)sVar1) + *(float *)(psVar2 + -2);
      }
      else {
        *(undefined4 *)(psVar2 + -2) = *(undefined4 *)(psVar2 + -4);
        *psVar2 = 0;
      }
      psVar2 = psVar2 + 6;
      iVar3 = iVar3 + -1;
    } while (iVar3 != 0);
  }
  return;
}

Disassembly (0x545330..0x545388, capstone; phase-4 review):

0x545330: push ecx
0x545331: push esi
0x545332: mov esi, dword ptr [esp + 0xc]
0x545336: test esi, esi
0x545338: jle 0x545385
0x54533a: mov eax, dword ptr [0x746140]
0x54533f: push edi
0x545340: add eax, 8
0x545343: mov edi, 0x33
0x545348: mov cx, word ptr [eax]
0x54534b: movsx edx, cx
0x54534e: cmp edx, esi
0x545350: mov dword ptr [esp + 8], edx
0x545354: jle 0x545373
0x545356: fild dword ptr [esp + 0x10]
0x54535a: sub ecx, esi
0x54535c: mov word ptr [eax], cx
0x54535f: fidiv dword ptr [esp + 8]
0x545363: fld dword ptr [eax - 8]
0x545366: fsub dword ptr [eax - 4]
0x545369: fmulp st(1)
0x54536b: fadd dword ptr [eax - 4]
0x54536e: fstp dword ptr [eax - 4]
0x545371: jmp 0x54537e
0x545373: mov ecx, dword ptr [eax - 8]
0x545376: mov dword ptr [eax - 4], ecx
0x545379: mov word ptr [eax], 0
0x54537e: add eax, 0xc
0x545381: dec edi
0x545382: jne 0x545348
0x545384: pop edi
0x545385: pop esi
0x545386: pop ecx
0x545387: ret 
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
