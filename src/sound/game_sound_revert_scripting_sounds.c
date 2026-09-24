// game_sound_revert_scripting_sounds  (Ghidra: chimera__revert; renamed per
//   out/phase4/sound_types_notes.md: "the Chimera hint prefix; the code is the game looping
//   sound back-reference clear (SoundLooping.runtime_scripting_sound)")
// address 0x543a90, size 151 bytes
// name confidence: 0.4   rewrite confidence: 0.85
// evidence: out/phase4/sound_types_notes.md misattribution note; types/sound.h game_looping_sound
//   (definition_index 0x0c) and SoundLooping.runtime_scripting_sound (tag +0x1c, per the header's
//   comment block); the loop body is datum_next's own scan (src/memory/datum_next.c) inlined and
//   interleaved with the per-datum back-reference clear, matching the pattern already used
//   (and explicitly called out as equivalent) in src/effects/contrail_update.c.
// register convention: plain __cdecl, no parameters (Ghidra shows a bare signature and the
//   inlined datum_next call takes its array/-1 implicitly from this function's own locals).
// blam-cc: (no arguments)
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found.

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "sound.h"

extern data_array *game_looping_sound_data; // 0x007461a0
extern tag_instance *tag_instances;         // 0x0087bc14

extern datum_index datum_next(int16_t after_index, data_array *array); // 0x4d0630
    // memory module; blam-cc: DX -> after_index, EDI -> array

// Walks every live object-looping-sound datum and, for each one, clears its definition tag's
// SoundLooping.runtime_scripting_sound back-reference when it still points at this datum --
// invalidating the scripted-sound cache the way the HS "revert" path expects.
void game_sound_revert_scripting_sounds(void)
{
    datum_index index = datum_next(-1, game_looping_sound_data);

    while (index != k_datum_index_none) {
        game_looping_sound *self = &((game_looping_sound *)game_looping_sound_data->data)[(uint16_t)index];
        SoundLooping *definition = (SoundLooping *)tag_instances[self->definition_index & 0xffff].data;

        if (*(uint32_t *)&definition->runtime_scripting_sound == (uint32_t)index) {
            *(uint32_t *)&definition->runtime_scripting_sound = (uint32_t)k_datum_index_none;
        }

        index = datum_next((int16_t)index, game_looping_sound_data);
    }
}

#if 0
Original Ghidra decompilation (0x543a90):

void chimera__revert(void)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  short *psVar4;
  short sVar5;
  int iVar6;

  iVar1 = DAT_007461a0;
  uVar3 = datum_next();
  iVar2 = DAT_0087bc14;
  do {
    do {
      if (uVar3 == 0xffffffff) {
        return;
      }
      iVar6 = *(int *)((*(uint *)((uVar3 & 0xffff) * 0x34 + 0xc + *(int *)(iVar1 + 0x34)) & 0xffff)
                       * 0x20 + 0x14 + iVar2);
      if (*(uint *)(iVar6 + 0x1c) == uVar3) {
        *(undefined4 *)(iVar6 + 0x1c) = 0xffffffff;
      }
      iVar6 = uVar3 + 1;
      uVar3 = 0xffffffff;
      sVar5 = (short)iVar6;
    } while ((sVar5 < 0) || (*(short *)(iVar1 + 0x2e) <= sVar5));
    psVar4 = (short *)((int)sVar5 * (int)*(short *)(iVar1 + 0x22) + *(int *)(iVar1 + 0x34));
    do {
      if (*psVar4 != 0) {
        uVar3 = (int)*psVar4 << 0x10 | (int)(short)iVar6;
        break;
      }
      iVar6 = iVar6 + 1;
      psVar4 = (short *)((int)psVar4 + (int)*(short *)(iVar1 + 0x22));
    } while ((short)iVar6 < *(short *)(iVar1 + 0x2e));
  } while( true );
}

Disassembly (0x543a90..0x543b27, capstone; phase-4 review):

0x543a90: push edi
0x543a91: mov edi, dword ptr [0x7461a0]
0x543a97: or edx, 0xffffffff
0x543a9a: call 0x4d0630
0x543a9f: cmp eax, -1
0x543aa2: je 0x543b25
0x543aa8: push ebx
0x543aa9: push ebp
0x543aaa: mov ebp, dword ptr [0x87bc14]
0x543ab0: push esi
0x543ab1: mov edx, dword ptr [edi + 0x34]
0x543ab4: mov ecx, eax
0x543ab6: and ecx, 0xffff
0x543abc: imul ecx, ecx, 0x34
0x543abf: mov ecx, dword ptr [ecx + edx + 0xc]
0x543ac3: and ecx, 0xffff
0x543ac9: shl ecx, 5
0x543acc: mov ecx, dword ptr [ecx + ebp + 0x14]
0x543ad0: cmp dword ptr [ecx + 0x1c], eax
0x543ad3: jne 0x543adc
0x543ad5: mov dword ptr [ecx + 0x1c], 0xffffffff
0x543adc: lea ecx, [eax + 1]
0x543adf: or esi, 0xffffffff
0x543ae2: test cx, cx
0x543ae5: jl 0x543b1b
0x543ae7: mov bx, word ptr [edi + 0x2e]
0x543aeb: cmp cx, bx
0x543aee: jge 0x543b1b
0x543af0: movsx edx, word ptr [edi + 0x22]
0x543af4: movsx eax, cx
0x543af7: imul eax, edx
0x543afa: add eax, dword ptr [edi + 0x34]
0x543afd: lea ecx, [ecx]
0x543b00: cmp word ptr [eax], 0
0x543b04: jne 0x543b10
0x543b06: inc ecx
0x543b07: add eax, edx
0x543b09: cmp cx, bx
0x543b0c: jl 0x543b00
0x543b0e: jmp 0x543b1b
0x543b10: movsx esi, word ptr [eax]
0x543b13: movsx edx, cx
0x543b16: shl esi, 0x10
0x543b19: or esi, edx
0x543b1b: cmp esi, -1
0x543b1e: mov eax, esi
0x543b20: jne 0x543ab1
0x543b22: pop esi
0x543b23: pop ebp
0x543b24: pop ebx
0x543b25: pop edi
0x543b26: ret 
#endif
