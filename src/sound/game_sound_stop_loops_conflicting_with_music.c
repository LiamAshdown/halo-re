// game_sound_stop_loops_conflicting_with_music  (Ghidra: FUN_00544c70)
// address 0x544c70, size 223 bytes
// name confidence: 0.35   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md summary "Sweeps unattached looping-sound datums and
//   detaches any conflicting class-0x20 looping sound currently bound to the associated object."
//   -- "object" there means "not object-bound" (object_index == k_datum_index_none, i.e. a
//   script/background loop); this walks every such datum via datum_next (the same inlined-scan
//   idiom as src/sound/game_sound_revert_scripting_sounds.c), and for each whose definition has a
//   music-class loop track (src/sound/sound_looping_definition_has_music_loop.c), detaches its
//   SoundLooping.runtime_scripting_sound back-reference exactly like
//   src/sound/sound_looping_stop.c, additionally setting
//   _game_looping_sound_stopped_by_music_bit (0x04) on the detached datum -- matching
//   types/sound.h's own flag comment: "_game_looping_sound_stopped_by_music_bit = 0x04,
//   // 0x544c70, the definition stops_music".
// register convention: plain __cdecl, no parameters.
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
extern uint32_t sound_looping_definition_has_music_loop(datum_index looping_definition); // 0x544c10

// For every live, not-object-bound game_looping_sound datum whose definition has a music-class
// loop track, detaches its definition's currently scripted instance and marks that instance
// stopped-by-music.
void game_sound_stop_loops_conflicting_with_music(void)
{
    datum_index index = datum_next(-1, game_looping_sound_data);

    while (index != k_datum_index_none) {
        game_looping_sound *self = &((game_looping_sound *)game_looping_sound_data->data)[(uint16_t)index];

        if (self->object_index == k_datum_index_none) {
            datum_index looping_definition = self->definition_index;
            uint32_t has_music_loop = sound_looping_definition_has_music_loop(looping_definition);

            if (has_music_loop != 0 && looping_definition != k_datum_index_none) {
                SoundLooping *definition = (SoundLooping *)tag_instances[looping_definition & 0xffff].data;
                datum_index scripted_sound = *(datum_index *)&definition->runtime_scripting_sound;

                if (scripted_sound != k_datum_index_none) {
                    game_looping_sound *scripted = &((game_looping_sound *)game_looping_sound_data->data)[(uint16_t)scripted_sound];

                    scripted->flags &= ~_game_looping_sound_scripted_bit;
                    scripted->flags |= _game_looping_sound_stop_requested_bit;
                    *(uint32_t *)&definition->runtime_scripting_sound = (uint32_t)k_datum_index_none;
                    scripted->flags |= _game_looping_sound_stopped_by_music_bit;
                }
            }
        }

        index = datum_next((int16_t)index, game_looping_sound_data);
    }
}

#if 0
Original Ghidra decompilation (0x544c70):

void FUN_00544c70(void)

{
  uint *puVar1;
  uint uVar2;
  int iVar3;
  char cVar4;
  uint uVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  short *psVar9;
  short sVar10;

  iVar3 = DAT_007461a0;
  uVar5 = datum_next();
  do {
    do {
      if (uVar5 == 0xffffffff) {
        return;
      }
      iVar6 = (uVar5 & 0xffff) * 0x34;
      if (*(int *)(iVar6 + 0x10 + *(int *)(iVar3 + 0x34)) == -1) {
        uVar2 = *(uint *)(iVar6 + *(int *)(iVar3 + 0x34) + 0xc);
        cVar4 = FUN_00544c10();
        if ((cVar4 != '\0') && (uVar2 != 0xffffffff)) {
          iVar6 = *(int *)((uVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
          if (*(uint *)(iVar6 + 0x1c) != 0xffffffff) {
            iVar7 = (*(uint *)(iVar6 + 0x1c) & 0xffff) * 0x34;
            iVar8 = iVar7 + *(int *)(iVar3 + 0x34);
            *(uint *)(iVar8 + 4) = *(uint *)(iVar7 + 4 + *(int *)(iVar3 + 0x34)) & 0xffffffef;
            puVar1 = (uint *)((*(uint *)(iVar6 + 0x1c) & 0xffff) * 0x34 + *(int *)(iVar3 + 0x34) + 4
                             );
            *puVar1 = *puVar1 | 2;
            *(undefined4 *)(iVar6 + 0x1c) = 0xffffffff;
            puVar1 = (uint *)(iVar8 + 4);
            *puVar1 = *puVar1 | 4;
          }
        }
      }
      iVar6 = uVar5 + 1;
      sVar10 = (short)iVar6;
      uVar5 = 0xffffffff;
    } while ((sVar10 < 0) || (*(short *)(iVar3 + 0x2e) <= sVar10));
    psVar9 = (short *)((int)sVar10 * (int)*(short *)(iVar3 + 0x22) + *(int *)(iVar3 + 0x34));
    do {
      if (*psVar9 != 0) {
        uVar5 = (int)*psVar9 << 0x10 | (int)(short)iVar6;
        break;
      }
      iVar6 = iVar6 + 1;
      psVar9 = (short *)((int)psVar9 + (int)*(short *)(iVar3 + 0x22));
    } while ((short)iVar6 < *(short *)(iVar3 + 0x2e));
  } while( true );
}

Disassembly (0x544c70..0x544d4f, capstone; phase-4 review):

0x544c70: push ebx
0x544c71: push ebp
0x544c72: push edi
0x544c73: mov edi, dword ptr [0x7461a0]
0x544c79: or ebp, 0xffffffff
0x544c7c: mov edx, ebp
0x544c7e: call 0x4d0630
0x544c83: mov ebx, eax
0x544c85: cmp ebx, ebp
0x544c87: je 0x544d4b
0x544c8d: push esi
0x544c8e: mov edi, edi
0x544c90: mov edx, dword ptr [edi + 0x34]
0x544c93: mov eax, ebx
0x544c95: and eax, 0xffff
0x544c9a: imul eax, eax, 0x34
0x544c9d: mov ecx, dword ptr [eax + edx + 0x10]
0x544ca1: add eax, edx
0x544ca3: cmp ecx, ebp
0x544ca5: jne 0x544d05
0x544ca7: mov esi, dword ptr [eax + 0xc]
0x544caa: mov eax, esi
0x544cac: call 0x544c10
0x544cb1: test al, al
0x544cb3: je 0x544d05
0x544cb5: cmp esi, ebp
0x544cb7: je 0x544d05
0x544cb9: mov eax, dword ptr [0x87bc14]
0x544cbe: and esi, 0xffff
0x544cc4: shl esi, 5
0x544cc7: mov edx, dword ptr [esi + eax + 0x14]
0x544ccb: mov eax, dword ptr [edx + 0x1c]
0x544cce: cmp eax, ebp
0x544cd0: je 0x544d05
0x544cd2: mov esi, dword ptr [edi + 0x34]
0x544cd5: and eax, 0xffff
0x544cda: imul eax, eax, 0x34
0x544cdd: mov ecx, dword ptr [eax + esi + 4]
0x544ce1: add eax, esi
0x544ce3: and ecx, 0xffffffef
0x544ce6: mov dword ptr [eax + 4], ecx
0x544ce9: mov ecx, dword ptr [edx + 0x1c]
0x544cec: mov esi, dword ptr [edi + 0x34]
0x544cef: and ecx, 0xffff
0x544cf5: imul ecx, ecx, 0x34
0x544cf8: add ecx, esi
0x544cfa: or dword ptr [ecx + 4], 2
0x544cfe: mov dword ptr [edx + 0x1c], ebp
0x544d01: or dword ptr [eax + 4], 4
0x544d05: lea ecx, [ebx + 1]
0x544d08: test cx, cx
0x544d0b: mov esi, ebp
0x544d0d: jl 0x544d40
0x544d0f: mov bx, word ptr [edi + 0x2e]
0x544d13: cmp cx, bx
0x544d16: jge 0x544d40
0x544d18: movsx edx, word ptr [edi + 0x22]
0x544d1c: movsx eax, cx
0x544d1f: imul eax, edx
0x544d22: add eax, dword ptr [edi + 0x34]
0x544d25: cmp word ptr [eax], 0
0x544d29: jne 0x544d35
0x544d2b: inc ecx
0x544d2c: add eax, edx
0x544d2e: cmp cx, bx
0x544d31: jl 0x544d25
0x544d33: jmp 0x544d40
0x544d35: movsx esi, word ptr [eax]
0x544d38: movsx ecx, cx
0x544d3b: shl esi, 0x10
0x544d3e: or esi, ecx
0x544d40: cmp esi, ebp
0x544d42: mov ebx, esi
0x544d44: jne 0x544c90
0x544d4a: pop esi
0x544d4b: pop edi
0x544d4c: pop ebp
0x544d4d: pop ebx
0x544d4e: ret 
#endif
