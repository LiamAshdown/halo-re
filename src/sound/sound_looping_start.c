// sound_looping_start  (Ghidra: FUN_00544090; earlier draft name sound_looping_start_scripted)
// address 0x544090, size 135 bytes
// name confidence: 0.7   rewrite confidence: 0.9
// evidence: the argument list (looping sound tag, object, scale) is exactly the hs command
//   sound_looping_start <looping_sound> <object> <real>; the scripted datum is registered in
//   SoundLooping.runtime_scripting_sound (tag+0x1c) with _game_looping_sound_scripted_bit, after
//   detaching the previous one (sound_looping_stop, 0x544120, EAX = definition) and,
//   for stops_music definitions (flag bit 2), stopping conflicting loops (0x544c70). Callers:
//   main_menu_play_title_music and the hs dispatch.
// Phase-4 review: rewritten from the disassembly appended below. The stack's first argument is
//   the object handed to looping_sound_new in EAX (the earlier draft discarded it and passed
//   function_index 0); function_index is -1 and the marker name is the empty string at
//   0x0065512c.
// register convention: EAX -> definition_index, stack -> (object_index, scale).

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "sound.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *game_looping_sound_data; // 0x007461a0
extern tag_instance *tag_instances;         // 0x0087bc14
extern char k_empty_string[];               // 0x0065512c

extern void sound_looping_stop(datum_index looping_definition); // 0x544120, blam-cc: EAX
extern void game_sound_stop_loops_conflicting_with_music(void); // 0x544c70
extern datum_index looping_sound_new(datum_index object_index, datum_index definition_index, char *marker_name,
    int16_t function_index); // 0x543c20, blam-cc: EAX, EDI, ECX, stack

// blam-cc: EAX -> definition_index, stack -> (object_index, scale)
// hs sound_looping_start: (re)starts the scripted instance of a looping sound, optionally on an
// object, with script-controlled gain.
void sound_looping_start(datum_index definition_index, datum_index object_index, float scale)
{
    SoundLooping *definition;
    datum_index new_sound;

    if (definition_index == k_datum_index_none) {
        return;
    }

    definition = (SoundLooping *)tag_instances[definition_index & 0xffff].data;
    sound_looping_stop(definition_index);
    if ((definition->flags & 0x04) != 0) { // stops_music
        game_sound_stop_loops_conflicting_with_music();
    }

    new_sound = looping_sound_new(object_index, definition_index, k_empty_string, -1);
    if (new_sound != k_datum_index_none) {
        game_looping_sound *self = &((game_looping_sound *)game_looping_sound_data->data)[new_sound & 0xffff];
        self->flags |= _game_looping_sound_script_gain_bit;
        self->scale = scale;
    }

    *(uint32_t *)&definition->runtime_scripting_sound = (uint32_t)new_sound;
    if (new_sound != k_datum_index_none) {
        ((game_looping_sound *)game_looping_sound_data->data)[new_sound & 0xffff].flags |=
            _game_looping_sound_scripted_bit;
    }
}

#if 0
Original Ghidra decompilation (0x544090):

void FUN_00544090(undefined4 param_1,undefined4 param_2)

{
  uint *puVar1;
  byte *pbVar2;
  int iVar3;
  uint in_EAX;
  uint uVar4;
  int iVar5;

  if (in_EAX != 0xffffffff) {
    pbVar2 = *(byte **)((in_EAX & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    looping_sound_object_detach();
    if ((*pbVar2 & 4) != 0) {
      FUN_00544c70();
    }
    uVar4 = looping_sound_new(0xffffffff);
    iVar3 = DAT_007461a0;
    if (uVar4 != 0xffffffff) {
      iVar5 = (uVar4 & 0xffff) * 0x34 + *(int *)(DAT_007461a0 + 0x34);
      puVar1 = (uint *)(iVar5 + 4);
      *puVar1 = *puVar1 | 1;
      *(undefined4 *)(iVar5 + 8) = param_2;
    }
    *(uint *)(pbVar2 + 0x1c) = uVar4;
    if (uVar4 != 0xffffffff) {
      iVar5 = (uVar4 & 0xffff) * 0x34;
      *(uint *)(iVar5 + *(int *)(iVar3 + 0x34) + 4) =
           *(uint *)(iVar5 + 4 + *(int *)(iVar3 + 0x34)) | 0x10;
    }
  }
  return;
}

Disassembly (0x544090..0x544117, capstone; phase-4 review):

0x544090: push edi
0x544091: mov edi, eax
0x544093: cmp edi, -1
0x544096: je 0x544115
0x544098: mov ecx, dword ptr [0x87bc14]
0x54409e: and eax, 0xffff
0x5440a3: shl eax, 5
0x5440a6: push esi
0x5440a7: mov esi, dword ptr [eax + ecx + 0x14]
0x5440ab: mov eax, edi
0x5440ad: call 0x544120
0x5440b2: test byte ptr [esi], 4
0x5440b5: je 0x5440bc
0x5440b7: call 0x544c70
0x5440bc: mov eax, dword ptr [esp + 0xc]
0x5440c0: push -1
0x5440c2: mov ecx, 0x65512c
0x5440c7: call 0x543c20
0x5440cc: mov edx, dword ptr [0x7461a0]
0x5440d2: add esp, 4
0x5440d5: cmp eax, -1
0x5440d8: je 0x5440f5
0x5440da: mov edi, dword ptr [edx + 0x34]
0x5440dd: mov ecx, eax
0x5440df: and ecx, 0xffff
0x5440e5: imul ecx, ecx, 0x34
0x5440e8: add ecx, edi
0x5440ea: or dword ptr [ecx + 4], 1
0x5440ee: mov edi, dword ptr [esp + 0x10]
0x5440f2: mov dword ptr [ecx + 8], edi
0x5440f5: cmp eax, -1
0x5440f8: mov dword ptr [esi + 0x1c], eax
0x5440fb: je 0x544114
0x5440fd: mov esi, dword ptr [edx + 0x34]
0x544100: and eax, 0xffff
0x544105: imul eax, eax, 0x34
0x544108: mov ecx, dword ptr [eax + esi + 4]
0x54410c: add eax, esi
0x54410e: or ecx, 0x10
0x544111: mov dword ptr [eax + 4], ecx
0x544114: pop esi
0x544115: pop edi
0x544116: ret 
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
