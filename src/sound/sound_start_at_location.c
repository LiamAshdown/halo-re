// sound_start_at_location  (Ghidra: FUN_00543d80; earlier draft name sound_start_from_parameter_block)
// address 0x543d80, size 71 bytes
// name confidence: 0.55   rewrite confidence: 0.85
// evidence: the 11 dwords behind EAX are copied to sound_location+0x0c (position, forward,
//   velocity, leaf, cluster: types/sound.h sound_placement), the location is absolute with
//   gain 1.0 and the stack argument as scale, and the sound is started with no owner and no
//   location proc. Callers: effect_event_apply, material_effects_play_at_marker,
//   particle_impact_response_dispatch, item_update, resolution_list_add_resolution.
// Phase-4 review: rewritten from the disassembly below (the earlier draft forwarded the block
//   dwords as sound_play_new's own arguments).
// register convention: EDX -> definition_index, EAX -> placement, stack -> scale.
// obstruction/occlusion (location 0x38/0x3c) are left uninitialized by the binary; zeroed here.

#include "tags.h"
#include "memory.h"
#include "sound.h"

extern datum_index sound_play_new(datum_index definition_index, sound_location *location, datum_index owner_index,
    sound_location_proc location_proc, void *callback_data, int32_t callback_data_size, uint32_t first_person_hint); // 0x549af0

// blam-cc: EDX -> definition_index, EAX -> placement, stack -> scale
// Starts a one-shot, ownerless sound at a fixed world placement.
datum_index sound_start_at_location(datum_index definition_index, sound_placement *placement, float scale)
{
    sound_location location;

    location.position = placement->position;
    location.forward = placement->forward;
    location.velocity = placement->velocity;
    location.leaf_index = placement->leaf_index;
    location.cluster_index = placement->cluster_index;
    location.unknown_36 = placement->unknown_2a;
    location.obstruction = 0.0f;
    location.occlusion = 0.0f;
    location.type = _sound_location_absolute;
    location.scale = scale;
    location.gain = 1.0f;

    return sound_play_new(definition_index, &location, k_datum_index_none, (sound_location_proc)0, (void *)0, 0, 0);
}

#if 0
Original Ghidra decompilation (0x543d80):

void FUN_00543d80(void)

{
  undefined4 *in_EAX;
  int iVar1;
  undefined4 *puVar2;
  undefined4 local_34 [13];

  puVar2 = local_34;
  for (iVar1 = 0xb; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar2 = *in_EAX;
    in_EAX = in_EAX + 1;
    puVar2 = puVar2 + 1;
  }
  FUN_00549af0();
  return;
}

Disassembly (0x543d80..0x543dc7, capstone; phase-4 review):

0x543d80: sub esp, 0x40
0x543d83: push esi
0x543d84: push edi
0x543d85: push 0
0x543d87: push 0
0x543d89: push 0
0x543d8b: mov esi, eax
0x543d8d: mov eax, dword ptr [esp + 0x58]
0x543d91: push 0
0x543d93: mov ecx, 0xb
0x543d98: lea edi, [esp + 0x24]
0x543d9c: rep movsd dword ptr es:[edi], dword ptr [esi]
0x543d9e: push -1
0x543da0: lea ecx, [esp + 0x1c]
0x543da4: push ecx
0x543da5: push edx
0x543da6: mov dword ptr [esp + 0x28], eax
0x543daa: mov word ptr [esp + 0x24], 1
0x543db1: mov dword ptr [esp + 0x2c], 0x3f800000
0x543db9: call 0x549af0
0x543dbe: add esp, 0x1c
0x543dc1: pop edi
0x543dc2: pop esi
0x543dc3: add esp, 0x40
0x543dc6: ret 
#endif
