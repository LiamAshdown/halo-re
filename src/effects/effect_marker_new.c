// effect_marker_new  (Ghidra: FUN_004517d0; named per out/phase4/effects_types_notes.md, which
// refers to this address directly: "effect_marker_new 0x4517d0 allocates it and copies
// object_marker.transform straight in, whose source pointer is pre-incremented by two shorts
// before the first store and therefore starts at object_marker.transform (+0x04)")
// address 0x4517d0, size 117 bytes
// name confidence: 0.55   rewrite confidence: 0.9 (VERIFIED against objdump 0x4517d0..0x451844)
// evidence: types/effects.h effect_location_marker (marker_index 0x02, next_marker 0x04,
// transform 0x08) and effect.location_markers[32] (0x5c); types/objects.h object_marker
// (node_index 0x00, transform 0x04).
// register convention: resolved object_marker pointer in EAX (in_EAX); location index and
// first-person flag are Ghidra's own recognised stack parameters.
//   // blam-cc: EAX -> resolved_marker, stack -> (self, location_index, first_person)
// UNSURE: `self` (param_1) and `location_index` (param_2) are Ghidra-recognised stack
// parameters here, but the caller (effect_rebuild_markers 0x451710) does not show a matching
// visible push for either at its one call site -- most likely both survive in registers across
// that call that Ghidra simply printed as if they were re-pushed. Kept as ordinary parameters
// since this function's own decompile treats them that way.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "effects.h"
#include "fn_memory.h"

extern data_array *effect_location_data; // 0x0087abe0


// Allocates an effect_location_marker for a marker resolved by effect_rebuild_markers, copies
// its transform, and links it at the head of effect->location_markers[location_index].
datum_index effect_marker_new(effect *self, int16_t location_index, object_marker *resolved_marker,
    uint8_t first_person) // returns the new marker's handle, or -1 (EAX from datum_new, 0x4517d9..0x451844)
{
    datum_index handle = datum_new(effect_location_data);

    if (handle != k_datum_index_none) {
        effect_location_marker *marker =
            &((effect_location_marker *)effect_location_data->data)[(uint16_t)handle];
        uint16_t node_index = resolved_marker->node_index;

        if (node_index != 0xffff) {
            node_index = first_person ? (node_index | 0x8000) : (node_index & 0x7fff);
        }
        marker->marker_index = node_index;
        marker->transform = resolved_marker->transform;

        marker->next_marker = self->location_markers[location_index];
        self->location_markers[location_index] = handle;
    }
    return handle;
}

#if 0
Original Ghidra decompilation (0x4517d0):

void FUN_004517d0(int param_1,short param_2,char param_3)

{
  uint *puVar1;
  ushort *in_EAX;
  uint uVar2;
  ushort uVar3;
  int iVar4;
  int iVar5;
  undefined4 *puVar6;
  undefined8 uVar7;

  uVar7 = datum_new();
  uVar2 = (uint)uVar7;
  if (uVar2 != 0xffffffff) {
    uVar3 = *in_EAX;
    iVar5 = (uVar2 & 0xffff) * 0x3c + *(int *)((int)((ulonglong)uVar7 >> 0x20) + 0x34);
    if (uVar3 == 0xffff) {
      uVar3 = 0xffff;
    }
    else if (param_3 == '\0') {
      uVar3 = uVar3 & 0x7fff;
    }
    else {
      uVar3 = uVar3 | 0x8000;
    }
    *(ushort *)(iVar5 + 2) = uVar3;
    puVar6 = (undefined4 *)(iVar5 + 8);
    for (iVar4 = 0xd; in_EAX = in_EAX + 2, iVar4 != 0; iVar4 = iVar4 + -1) {
      *puVar6 = *(undefined4 *)in_EAX;
      puVar6 = puVar6 + 1;
    }
    puVar1 = (uint *)(param_1 + 0x5c + param_2 * 4);
    *(uint *)(iVar5 + 4) = *puVar1;
    *puVar1 = uVar2;
  }
  return;
}
#endif
