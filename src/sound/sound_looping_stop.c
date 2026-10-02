// sound_looping_stop  (Ghidra: looping_sound_object_detach)  [earlier name looping_sound_object_detach]
// address 0x544120, size 88 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// evidence: despite the "object" in its already-established name, this operates on a SoundLooping
//   tag handle (tag_instances lookup, matching every sibling scripting_sound helper in this file
//   set): reads SoundLooping.runtime_scripting_sound (0x1c, types/tags.h), and when set, clears
//   the referenced game_looping_sound's _game_looping_sound_scripted_bit (0x10) and sets its
//   _game_looping_sound_stop_requested_bit (0x02) (types/sound.h game_looping_sound_flags),
//   before clearing the tag's own back-reference to k_datum_index_none.
// register convention: SoundLooping tag handle in EAX (in_EAX).
// blam-cc: EAX -> looping_definition

// Phase-4 review rename: the hs command sound_looping_stop <looping_sound>: detaches the definition runtime_scripting_sound and flags it stop_requested; called from the hs table (0x47fede) and the main menu music code.

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "sound.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *game_looping_sound_data; // 0x007461a0
extern tag_instance *tag_instances;         // 0x0087bc14

// blam-cc: EAX -> looping_definition
// Detaches the game_looping_sound datum currently referenced by a SoundLooping tag's
// runtime_scripting_sound back-reference: requests it stop and clears the scripted flag, then
// clears the tag's own reference.
void sound_looping_stop(datum_index looping_definition)
{
    if (looping_definition != k_datum_index_none) {
        SoundLooping *definition = (SoundLooping *)tag_instances[looping_definition & 0xffff].data;
        datum_index scripted_sound = *(datum_index *)&definition->runtime_scripting_sound;

        if (scripted_sound != k_datum_index_none) {
            game_looping_sound *self = &((game_looping_sound *)game_looping_sound_data->data)[(uint16_t)scripted_sound];

            self->flags &= ~_game_looping_sound_scripted_bit;
            self->flags |= _game_looping_sound_stop_requested_bit;

            *(uint32_t *)&definition->runtime_scripting_sound = (uint32_t)k_datum_index_none;
        }
    }
}

#if 0
Original Ghidra decompilation (0x544120):

void looping_sound_object_detach(void)

{
  uint *puVar1;
  int iVar2;
  int iVar3;
  uint in_EAX;
  int iVar4;

  iVar3 = DAT_007461a0;
  if (in_EAX != 0xffffffff) {
    iVar2 = *(int *)((in_EAX & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    if (*(uint *)(iVar2 + 0x1c) != 0xffffffff) {
      puVar1 = (uint *)((*(uint *)(iVar2 + 0x1c) & 0xffff) * 0x34 + *(int *)(DAT_007461a0 + 0x34) +
                       4);
      *puVar1 = *puVar1 & 0xffffffef;
      iVar4 = (*(uint *)(iVar2 + 0x1c) & 0xffff) * 0x34;
      *(uint *)(iVar4 + *(int *)(iVar3 + 0x34) + 4) =
           *(uint *)(iVar4 + 4 + *(int *)(iVar3 + 0x34)) | 2;
      *(undefined4 *)(iVar2 + 0x1c) = 0xffffffff;
    }
  }
  return;
}

Disassembly (0x544120..0x544178, capstone; phase-4 review):

0x544120: cmp eax, -1
0x544123: je 0x544177
0x544125: mov ecx, dword ptr [0x87bc14]
0x54412b: and eax, 0xffff
0x544130: shl eax, 5
0x544133: mov ecx, dword ptr [eax + ecx + 0x14]
0x544137: mov eax, dword ptr [ecx + 0x1c]
0x54413a: cmp eax, -1
0x54413d: je 0x544177
0x54413f: mov edx, dword ptr [0x7461a0]
0x544145: and eax, 0xffff
0x54414a: imul eax, eax, 0x34
0x54414d: push esi
0x54414e: add eax, dword ptr [edx + 0x34]
0x544151: and dword ptr [eax + 4], 0xffffffef
0x544155: mov eax, dword ptr [ecx + 0x1c]
0x544158: mov esi, dword ptr [edx + 0x34]
0x54415b: and eax, 0xffff
0x544160: imul eax, eax, 0x34
0x544163: mov edx, dword ptr [eax + esi + 4]
0x544167: add eax, esi
0x544169: or edx, 2
0x54416c: mov dword ptr [eax + 4], edx
0x54416f: mov dword ptr [ecx + 0x1c], 0xffffffff
0x544176: pop esi
0x544177: ret 
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
