// sound_looping_set_alternate  (Ghidra: FUN_00544200)  [earlier name looping_sound_object_set_alternate]
// address 0x544200, size 75 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// evidence: same SoundLooping.runtime_scripting_sound (0x1c) lookup idiom as
//   src/sound/sound_looping_stop.c; sets or clears game_looping_sound.flags bit 0x08
//   (types/sound.h _game_looping_sound_alternate_bit, "0x544200, forwarded to
//   sound_looping_set_state").
// register convention: SoundLooping tag handle in EAX (in_EAX), boolean as the one stack
// parameter Ghidra recognizes directly (param_1).
// blam-cc: EAX -> looping_definition, stack -> alternate

// Phase-4 review rename: the hs command sound_looping_set_alternate <looping_sound> <boolean>: toggles _game_looping_sound_alternate_bit; called from the hs table (0x47ff74).

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "sound.h"
#include "fn_sound.h"

extern data_array *game_looping_sound_data; // 0x007461a0
extern tag_instance *tag_instances;         // 0x0087bc14

// blam-cc: EAX -> looping_definition, stack -> alternate
// Sets or clears the alternate-loop option flag on the looping-sound datum currently referenced
// by a SoundLooping tag's runtime_scripting_sound back-reference.
void sound_looping_set_alternate(datum_index looping_definition, uint8_t alternate)
{
    if (looping_definition != k_datum_index_none) {
        SoundLooping *definition = (SoundLooping *)tag_instances[looping_definition & 0xffff].data;
        datum_index scripted_sound = *(datum_index *)&definition->runtime_scripting_sound;

        if (scripted_sound != k_datum_index_none) {
            game_looping_sound *self = &((game_looping_sound *)game_looping_sound_data->data)[(uint16_t)scripted_sound];

            if (alternate != 0) {
                self->flags |= _game_looping_sound_alternate_bit;
            } else {
                self->flags &= ~_game_looping_sound_alternate_bit;
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x544200):

void FUN_00544200(char param_1)

{
  uint uVar1;
  uint in_EAX;
  int iVar2;

  if ((in_EAX != 0xffffffff) &&
     (uVar1 = *(uint *)(*(int *)((in_EAX & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x1c),
     uVar1 != 0xffffffff)) {
    iVar2 = (uVar1 & 0xffff) * 0x34 + *(int *)(DAT_007461a0 + 0x34);
    if (param_1 != '\0') {
      *(uint *)(iVar2 + 4) = *(uint *)(iVar2 + 4) | 8;
      return;
    }
    *(uint *)(iVar2 + 4) = *(uint *)(iVar2 + 4) & 0xfffffff7;
  }
  return;
}

Disassembly (0x544200..0x54424b, capstone; phase-4 review):

0x544200: cmp eax, -1
0x544203: je 0x54424a
0x544205: mov ecx, dword ptr [0x87bc14]
0x54420b: and eax, 0xffff
0x544210: shl eax, 5
0x544213: mov eax, dword ptr [eax + ecx + 0x14]
0x544217: mov eax, dword ptr [eax + 0x1c]
0x54421a: cmp eax, -1
0x54421d: je 0x54424a
0x54421f: mov edx, dword ptr [0x7461a0]
0x544225: mov ecx, dword ptr [edx + 0x34]
0x544228: and eax, 0xffff
0x54422d: imul eax, eax, 0x34
0x544230: add eax, ecx
0x544232: mov cl, byte ptr [esp + 4]
0x544236: test cl, cl
0x544238: mov ecx, dword ptr [eax + 4]
0x54423b: je 0x544244
0x54423d: or ecx, 8
0x544240: mov dword ptr [eax + 4], ecx
0x544243: ret 
0x544244: and ecx, 0xfffffff7
0x544247: mov dword ptr [eax + 4], ecx
0x54424a: ret 
#endif
