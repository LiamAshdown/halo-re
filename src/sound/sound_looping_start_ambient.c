// sound_looping_start_ambient  (Ghidra: FUN_00544250)
// address 0x544250, size 64 bytes
// name confidence: 0.4   rewrite confidence: 0.9
// evidence: calls looping_sound_new (0x543c20) with the caller's EAX as object_index, EDX moved
//   into EDI as definition_index, ECX = 0x0065512c (empty marker name) and function_index -1,
//   then sets _game_looping_sound_script_gain_bit and game_looping_sound.scale from its stack
//   argument. Only caller: game_sound_update (0x5445c0), for the cluster background sound, with
//   EAX = -1.
// Phase-4 review: rewritten from the disassembly appended below (the earlier draft passed
//   function_index 0 and lost the object argument).
// register convention: EAX -> object_index, EDX -> definition_index, stack -> scale.

#include "tags.h"
#include "memory.h"
#include "sound.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *game_looping_sound_data; // 0x007461a0
extern char k_empty_string[];               // 0x0065512c

extern datum_index looping_sound_new(datum_index object_index, datum_index definition_index, char *marker_name,
    int16_t function_index); // 0x543c20, blam-cc: EAX, EDI, ECX, stack

// blam-cc: EAX -> object_index, EDX -> definition_index, stack -> scale
// Creates a script-gain game_looping_sound for `definition_index` with gain `scale`.
datum_index sound_looping_start_ambient(datum_index object_index, datum_index definition_index, float scale)
{
    datum_index new_sound = looping_sound_new(object_index, definition_index, k_empty_string, -1);

    if (new_sound != k_datum_index_none) {
        game_looping_sound *self = &((game_looping_sound *)game_looping_sound_data->data)[new_sound & 0xffff];
        self->flags |= _game_looping_sound_script_gain_bit;
        self->scale = scale;
    }

    return new_sound;
}

#if 0
Original Ghidra decompilation (0x544250):

void FUN_00544250(undefined4 param_1)

{
  uint uVar1;
  int iVar2;
  int iVar3;

  uVar1 = looping_sound_new(0xffffffff);
  if (uVar1 != 0xffffffff) {
    iVar2 = (uVar1 & 0xffff) * 0x34;
    iVar3 = iVar2 + *(int *)(DAT_007461a0 + 0x34);
    *(uint *)(iVar3 + 4) = *(uint *)(iVar2 + 4 + *(int *)(DAT_007461a0 + 0x34)) | 1;
    *(undefined4 *)(iVar3 + 8) = param_1;
  }
  return;
}

Disassembly (0x544250..0x544290, capstone; phase-4 review):

0x544250: push edi
0x544251: push -1
0x544253: mov ecx, 0x65512c
0x544258: mov edi, edx
0x54425a: call 0x543c20
0x54425f: add esp, 4
0x544262: cmp eax, -1
0x544265: je 0x54428e
0x544267: mov edx, dword ptr [0x7461a0]
0x54426d: mov edi, dword ptr [edx + 0x34]
0x544270: mov ecx, eax
0x544272: and ecx, 0xffff
0x544278: imul ecx, ecx, 0x34
0x54427b: mov edx, dword ptr [ecx + edi + 4]
0x54427f: add ecx, edi
0x544281: or edx, 1
0x544284: mov dword ptr [ecx + 4], edx
0x544287: mov edx, dword ptr [esp + 8]
0x54428b: mov dword ptr [ecx + 8], edx
0x54428e: pop edi
0x54428f: ret 
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
