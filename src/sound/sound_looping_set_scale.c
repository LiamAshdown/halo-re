// sound_looping_set_scale  (Ghidra: looping_sound_object_set_gain)  [earlier name looping_sound_object_set_gain]
// address 0x544180, size 114 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// evidence: same SoundLooping.runtime_scripting_sound (0x1c) lookup idiom as
//   src/sound/sound_looping_stop.c; writes game_looping_sound.scale (0x08, types/sound.h)
//   clamped to [0, 1].
// register convention: SoundLooping tag handle in EAX (in_EAX), gain as the one stack parameter
//   Ghidra recognizes directly (param_1).
// blam-cc: EAX -> looping_definition, stack -> gain

// Phase-4 review rename: the hs command sound_looping_set_scale <looping_sound> <real>: clamps to [0,1] into the scripted game_looping_sound.scale; called from the hs table (0x47ff22).

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "sound.h"
#include "fn_sound.h"

extern data_array *game_looping_sound_data; // 0x007461a0
extern tag_instance *tag_instances;         // 0x0087bc14

// blam-cc: EAX -> looping_definition, stack -> gain
// Sets the playback gain (clamped to [0, 1]) of the looping-sound datum currently referenced by a
// SoundLooping tag's runtime_scripting_sound back-reference.
void sound_looping_set_scale(datum_index looping_definition, float gain)
{
    if (looping_definition != k_datum_index_none) {
        SoundLooping *definition = (SoundLooping *)tag_instances[looping_definition & 0xffff].data;
        datum_index scripted_sound = *(datum_index *)&definition->runtime_scripting_sound;

        if (scripted_sound != k_datum_index_none) {
            game_looping_sound *self = &((game_looping_sound *)game_looping_sound_data->data)[(uint16_t)scripted_sound];

            if (gain < 0.0f) {
                self->scale = 0.0f;
            } else if (gain > 1.0f) {
                self->scale = 1.0f;
            } else {
                self->scale = gain;
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x544180):

void looping_sound_object_set_gain(float param_1)

{
  uint uVar1;
  uint in_EAX;
  int iVar2;

  if ((in_EAX != 0xffffffff) &&
     (uVar1 = *(uint *)(*(int *)((in_EAX & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x1c),
     uVar1 != 0xffffffff)) {
    iVar2 = (uVar1 & 0xffff) * 0x34 + *(int *)(DAT_007461a0 + 0x34);
    if (param_1 < 0.0) {
      *(undefined4 *)(iVar2 + 8) = 0;
      return;
    }
    if (1.0 < param_1) {
      *(undefined4 *)(iVar2 + 8) = 0x3f800000;
      return;
    }
    *(float *)(iVar2 + 8) = param_1;
  }
  return;
}

Disassembly (0x544180..0x5441f2, capstone; phase-4 review):

0x544180: cmp eax, -1
0x544183: je 0x5441f1
0x544185: mov ecx, dword ptr [0x87bc14]
0x54418b: and eax, 0xffff
0x544190: shl eax, 5
0x544193: mov eax, dword ptr [eax + ecx + 0x14]
0x544197: mov eax, dword ptr [eax + 0x1c]
0x54419a: cmp eax, -1
0x54419d: je 0x5441f1
0x54419f: mov edx, dword ptr [0x7461a0]
0x5441a5: fld dword ptr [esp + 4]
0x5441a9: mov ecx, dword ptr [edx + 0x34]
0x5441ac: fcomp dword ptr [0x672ac0]
0x5441b2: and eax, 0xffff
0x5441b7: imul eax, eax, 0x34
0x5441ba: add eax, ecx
0x5441bc: mov ecx, eax
0x5441be: fnstsw ax
0x5441c0: test ah, 5
0x5441c3: jp 0x5441cf
0x5441c5: fld dword ptr [0x672ac0]
0x5441cb: fstp dword ptr [ecx + 8]
0x5441ce: ret 
0x5441cf: fld dword ptr [esp + 4]
0x5441d3: fcomp dword ptr [0x672ac4]
0x5441d9: fnstsw ax
0x5441db: test ah, 0x41
0x5441de: jne 0x5441ea
0x5441e0: fld dword ptr [0x672ac4]
0x5441e6: fstp dword ptr [ecx + 8]
0x5441e9: ret 
0x5441ea: fld dword ptr [esp + 4]
0x5441ee: fstp dword ptr [ecx + 8]
0x5441f1: ret 
#endif
