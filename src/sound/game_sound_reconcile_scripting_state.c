// game_sound_reconcile_scripting_state  (Ghidra: FUN_00543b30)
// address 0x543b30, size 233 bytes
// name confidence: 0.35   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md summary "Re-establishes or tears down object<->looping-
//   sound datum links and invalidates a per-tag cached looping-sound-definition index."; the
//   first loop mirrors src/sound/game_sound_revert_scripting_sounds.c's datum_next scan over
//   game_looping_sound_data, testing game_looping_sound.flags bit 0x10
//   (_game_looping_sound_scripted_bit) and SoundLooping.flags bit 0x02 (not_a_loop, per
//   types/tags.h's SoundLoopingFlags comment); the trailing loop's tag_iterator_next scan and
//   local tag_iterator construction follow src/cache/model_dispose_vertex_buffers.c exactly, and
//   the byte-offset arithmetic on the write target (tag data + 0x90) lines up with Sound.scripting_time
//   (types/sound.h header comment: "promotion_time 0x8c, scripting_time 0x90, scripting_sound
//   0x94"), not SoundLooping (which is only 0x54 bytes and has no offset 0x90).
// register convention: plain __cdecl, no parameters.
// blam-cc: (no arguments)
// UNSURE: the "snd!" tag group fourcc used to filter the trailing tag_iterator is inferred from
//   the offset-0x90 write matching Sound.scripting_time; cache.h has no _tag_group_sound constant
//   yet (only _tag_group_gbxmodel is established), so the raw fourcc is used here with a comment
//   rather than adding one to types/cache.h.
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found.

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "sound.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *game_looping_sound_data; // 0x007461a0
extern tag_instance *tag_instances;         // 0x0087bc14

extern datum_index datum_next(int16_t after_index, data_array *array); // 0x4d0630
    // memory module; blam-cc: DX -> after_index, EDI -> array
extern datum_index tag_iterator_next(tag_iterator *iterator); // 0x4425d0
    // blam-cc: ESI -> iterator
extern void datum_delete(data_array *array, datum_index handle); // 0x4d0510
    // blam-cc: EAX -> array, EDX -> handle

// For every live object-looping-sound datum flagged as scripted, either re-establishes its
// definition tag's SoundLooping.runtime_scripting_sound back-reference (when the tag still
// defines a loop) or deletes the now-stale datum (when the tag's not_a_loop flag has been set).
// Afterwards, walks every sound tag and invalidates its cached Sound.scripting_time.
void game_sound_reconcile_scripting_state(void)
{
    tag_iterator iterator;
    datum_index index = datum_next(-1, game_looping_sound_data);
    datum_index tag_id;

    while (index != k_datum_index_none) {
        game_looping_sound *self = &((game_looping_sound *)game_looping_sound_data->data)[(uint16_t)index];

        if ((self->flags & _game_looping_sound_scripted_bit) != 0) {
            SoundLooping *definition = (SoundLooping *)tag_instances[self->definition_index & 0xffff].data;

            if ((definition->flags & 0x02) == 0) {
                *(uint32_t *)&definition->runtime_scripting_sound = (uint32_t)index;
            } else {
                datum_delete(game_looping_sound_data, index);
            }
        }

        index = datum_next((int16_t)index, game_looping_sound_data);
    }

    iterator.next_index = 0;
    iterator.group_tag = 0x736e6421; // "snd!" (UNSURE: inferred, not yet in types/cache.h)

    tag_id = tag_iterator_next(&iterator);
    while (tag_id != (datum_index)0xffffffff) {
        Sound *sound_tag = (Sound *)tag_instances[(uint16_t)tag_id].data;
        sound_tag->scripting_time = 0xffffffff;
        tag_id = tag_iterator_next(&iterator);
    }
}

#if 0
Original Ghidra decompilation (0x543b30):

void FUN_00543b30(void)

{
  byte *pbVar1;
  int iVar2;
  int iVar3;
  uint uVar4;
  int iVar5;
  short *psVar6;
  short sVar7;

  iVar2 = DAT_007461a0;
  uVar4 = datum_next();
  iVar3 = DAT_0087bc14;
  do {
    do {
      if (uVar4 == 0xffffffff) {
        uVar4 = tag_iterator_next();
        while (uVar4 != 0xffffffff) {
          *(undefined4 *)(*(int *)((uVar4 & 0xffff) * 0x20 + 0x14 + iVar3) + 0x90) = 0xffffffff;
          uVar4 = tag_iterator_next();
        }
        return;
      }
      iVar5 = (uVar4 & 0xffff) * 0x34;
      if ((*(byte *)(iVar5 + 4 + *(int *)(iVar2 + 0x34)) & 0x10) != 0) {
        pbVar1 = *(byte **)((*(uint *)(iVar5 + *(int *)(iVar2 + 0x34) + 0xc) & 0xffff) * 0x20 + 0x14
                           + iVar3);
        if ((*pbVar1 & 2) == 0) {
          *(uint *)(pbVar1 + 0x1c) = uVar4;
        }
        else {
          datum_delete();
        }
      }
      iVar5 = uVar4 + 1;
      uVar4 = 0xffffffff;
      sVar7 = (short)iVar5;
    } while ((sVar7 < 0) || (*(short *)(iVar2 + 0x2e) <= sVar7));
    psVar6 = (short *)((int)sVar7 * (int)*(short *)(iVar2 + 0x22) + *(int *)(iVar2 + 0x34));
    do {
      if (*psVar6 != 0) {
        uVar4 = (int)*psVar6 << 0x10 | (int)(short)iVar5;
        break;
      }
      iVar5 = iVar5 + 1;
      psVar6 = (short *)((int)psVar6 + (int)*(short *)(iVar2 + 0x22));
    } while ((short)iVar5 < *(short *)(iVar2 + 0x2e));
  } while( true );
}

Disassembly (0x543b30..0x543c19, capstone; phase-4 review):

0x543b30: sub esp, 0x14
0x543b33: push ebx
0x543b34: push esi
0x543b35: push edi
0x543b36: mov edi, dword ptr [0x7461a0]
0x543b3c: or edx, 0xffffffff
0x543b3f: call 0x4d0630
0x543b44: mov ebx, dword ptr [0x87bc14]
0x543b4a: mov esi, eax
0x543b4c: cmp esi, -1
0x543b4f: je 0x543bd4
0x543b55: push ebp
0x543b56: mov edx, dword ptr [edi + 0x34]
0x543b59: mov eax, esi
0x543b5b: and eax, 0xffff
0x543b60: imul eax, eax, 0x34
0x543b63: mov cl, byte ptr [eax + edx + 4]
0x543b67: add eax, edx
0x543b69: test cl, 0x10
0x543b6c: je 0x543b90
0x543b6e: mov eax, dword ptr [eax + 0xc]
0x543b71: and eax, 0xffff
0x543b76: shl eax, 5
0x543b79: mov eax, dword ptr [eax + ebx + 0x14]
0x543b7d: test byte ptr [eax], 2
0x543b80: jne 0x543b87
0x543b82: mov dword ptr [eax + 0x1c], esi
0x543b85: jmp 0x543b90
0x543b87: mov edx, esi
0x543b89: mov eax, edi
0x543b8b: call 0x4d0510
0x543b90: lea ecx, [esi + 1]
0x543b93: or ebp, 0xffffffff
0x543b96: test cx, cx
0x543b99: jl 0x543bcc
0x543b9b: mov si, word ptr [edi + 0x2e]
0x543b9f: cmp cx, si
0x543ba2: jge 0x543bcc
0x543ba4: movsx edx, word ptr [edi + 0x22]
0x543ba8: movsx eax, cx
0x543bab: imul eax, edx
0x543bae: add eax, dword ptr [edi + 0x34]
0x543bb1: cmp word ptr [eax], 0
0x543bb5: jne 0x543bc1
0x543bb7: inc ecx
0x543bb8: add eax, edx
0x543bba: cmp cx, si
0x543bbd: jl 0x543bb1
0x543bbf: jmp 0x543bcc
0x543bc1: movsx ebp, word ptr [eax]
0x543bc4: movsx ecx, cx
0x543bc7: shl ebp, 0x10
0x543bca: or ebp, ecx
0x543bcc: cmp ebp, -1
0x543bcf: mov esi, ebp
0x543bd1: jne 0x543b56
0x543bd3: pop ebp
0x543bd4: lea esi, [esp + 0xc]
0x543bd8: mov word ptr [esp + 0x10], 0
0x543bdf: mov dword ptr [esp + 0x1c], 0x736e6421
0x543be7: call 0x4425d0
0x543bec: or edi, 0xffffffff
0x543bef: cmp eax, edi
0x543bf1: je 0x543c12
0x543bf3: and eax, 0xffff
0x543bf8: shl eax, 5
0x543bfb: mov edx, dword ptr [eax + ebx + 0x14]
0x543bff: lea esi, [esp + 0xc]
0x543c03: mov dword ptr [edx + 0x90], edi
0x543c09: call 0x4425d0
0x543c0e: cmp eax, edi
0x543c10: jne 0x543bf3
0x543c12: pop edi
0x543c13: pop esi
0x543c14: pop ebx
0x543c15: add esp, 0x14
0x543c18: ret 
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
