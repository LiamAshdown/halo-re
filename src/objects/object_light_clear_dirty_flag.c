// object_light_clear_dirty_flag
// address 0x4f29c0, size 58 bytes
// name confidence: 0.7 (still named "chimera__light_table" by an earlier, Chimera-derived pass;
//   types/objects.h's own light_flags comment and the light struct's stride note both name this
//   function "object_light_clear_dirty_flag" and flag the Chimera label as not a genuine Bungie
//   identifier -- see out/phase4/objects_types_notes.md's "Ghidra-split tails" section)
// rewrite confidence: 0.65
// evidence: types/objects.h light (stride 0x7c, flags at 0x02, _light_attached_bit 0x0002,
//   _light_transform_dirty_bit 0x0004); global 0x00860b14 light_data.
// register convention: light index in EAX (in_EAX).
// UNSURE: types/objects.h itself flags the bit mismatch here (tests _light_attached_bit but
//   clears _light_transform_dirty_bit); preserved exactly as decompiled rather than "fixed".

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern data_array *light_data; // 0x00860b14

extern datum_index *light_cluster_first; // 0x00860b20
extern void cluster_reference_remove_all(uint32_t handle, datum_index *link, void *cluster_list); // 0x552020, UNSURE: argument inferred; the same
                                                 //   callee used by object_unlink_cluster_or_
                                                 //   notify_parent elsewhere in this batch

void object_light_clear_dirty_flag(uint32_t light_index) // blam-cc: EAX -> light_index
{
    light *entry = (light *)light_data->data + (light_index & 0xffff);

    if ((entry->flags & _light_attached_bit) != 0) {
        cluster_reference_remove_all(light_index, (datum_index *)(entry + 0x10), &light_cluster_first);
        // 0x4f29e1 lea edx,[esi+0x10] / push edx / push eax / mov ebx,0x860b20 // UNSURE: see file header
        entry->flags &= (uint16_t)~_light_transform_dirty_bit;
    }
}

#if 0
Original Ghidra decompilation (0x4f29c0):

void chimera__light_table(void)

{
  byte *pbVar1;
  int iVar2;
  uint in_EAX;
  int iVar3;

  iVar2 = *(int *)(DAT_00860b14 + 0x34);
  iVar3 = (in_EAX & 0xffff) * 0x7c;
  if ((*(byte *)(iVar3 + 2 + iVar2) & 2) != 0) {
    FUN_00552020();
    pbVar1 = (byte *)(iVar3 + iVar2 + 2);
    *pbVar1 = *pbVar1 & 0xfb;
  }
  return;
}
#endif
