// sound_instance_queue_definition_switch  (Ghidra: FUN_0054dd90, still unnamed there)
// address 0x54dd90, size 39 bytes
// name confidence: 0.55   rewrite confidence: 0.85
// evidence: types/sound.h's own note: "pending_definition_index 0x98 0x54dd90 queues a
// definition switch, -1 none" attributes this exact address to that field.
// register convention: EAX -> sound_handle, ECX -> new_definition_index.
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found.

#include "tags.h"
#include "memory.h"
#include "sound.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *sound_data; // 0x007252c0, "sounds" 0x200 x 0xb0

// blam-cc: EAX -> sound_handle, ECX -> new_definition_index
// Queues a Sound tag switch for `sound_handle` if `new_definition_index` differs from its
// current definition (a later pass picks this up via pending_definition_index).
void sound_instance_queue_definition_switch(datum_index sound_handle, datum_index new_definition_index)
{
    sound *instance;

    instance = (sound *)((uint8_t *)sound_data->data + (sound_handle & 0xffff) * sizeof(sound));
    if (instance->definition_index != new_definition_index) {
        instance->pending_definition_index = new_definition_index;
    }
}

#if 0
Original Ghidra decompilation (0x54dd90):

void FUN_0054dd90(void)

{
  uint in_EAX;
  int iVar1;
  int in_ECX;

  iVar1 = (in_EAX & 0xffff) * 0xb0;
  if (*(int *)(iVar1 + 8 + *(int *)(DAT_007252c0 + 0x34)) != in_ECX) {
    *(int *)(iVar1 + *(int *)(DAT_007252c0 + 0x34) + 0x98) = in_ECX;
  }
  return;
}

Disassembly (0x54dd90..0x54ddb7, capstone; phase-4 review):

0x54dd90: mov edx, dword ptr [0x7252c0]
0x54dd96: and eax, 0xffff
0x54dd9b: imul eax, eax, 0xb0
0x54dda1: push esi
0x54dda2: mov esi, dword ptr [edx + 0x34]
0x54dda5: mov edx, dword ptr [eax + esi + 8]
0x54dda9: add eax, esi
0x54ddab: cmp edx, ecx
0x54ddad: pop esi
0x54ddae: je 0x54ddb6
0x54ddb0: mov dword ptr [eax + 0x98], ecx
0x54ddb6: ret 
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
