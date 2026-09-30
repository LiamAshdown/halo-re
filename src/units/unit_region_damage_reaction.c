// unit_region_damage_reaction  (Ghidra: no function created; the phase-4 types agent carved a
//   stub "missed_56f1c0" from the object_type_definition vtable evidence)
// address 0x56f1c0, size 67 bytes
// name confidence 0.35, rewrite confidence 0.6
// evidence: out/phase4/units_types_notes.md: "The unit row's other columns are ... 0x56f1c0
//   (+0x40 region damage) ...". Gated on the unit not already being dead (object.vitality_flags
//   bit 4, objects.h `_object_health_frozen_bit`), it dispatches reaction animation 3 or 4
//   (0x5614a0, unit_dispatch_reaction_animation, already rewritten this pass) depending on bit
//   0x200 of its third argument -- the shape expected of a per-region damage-response callback
//   that reacts differently depending on which response flag the destroyed region carried.
// register convention: object index and one further value in two register arguments, plus a
//   third value (the region's damage response flags) in a third register; matches this module's
//   convention of `object_index` plus extra per-call context for object_type_definition columns
//   that are invoked from the damage system. blam-cc: object_index, param_2 (unused here), flags.
// param_2 is never read (the stack slot exists in the caller's call only).
// VERIFIED against disassembly 0x56f1c0..0x56f203 (2026-09-30): frozen bit test, then reaction 3 + ((flags & 0x200) != 0).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data; // 0x008603b0

extern void unit_dispatch_reaction_animation(int32_t unit_index, int16_t reaction_code); // 0x5614a0, ESI unit, stack code,
    // blam-cc: ESI -> unit index (object_index here, 0x56f1ca), stack -> reaction_code. UNSURE: the
    // definition (src/units/unit_dispatch_reaction_animation.c, rewrite confidence 0.15) does not
    // model the ESI argument yet, so the prototype is kept as that file declares it.

// object_type_definition "unit" row, +0x40 column ("region damage"). Unless the unit is already
// dead, dispatches reaction animation 4 when damage response flag 0x200 is set, otherwise 3.
void unit_region_damage_reaction(uint32_t object_index, uint32_t unused, uint32_t flags)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    (void)unused;

    if ((obj->vitality_flags & 4) == 0) {
        unit_dispatch_reaction_animation((int32_t)object_index, (int16_t)(((flags & 0x200) != 0) + 3));
    }
}

#if 0
Original Ghidra decompilation (0x56f1c0):

void missed_56f1c0(uint param_1,undefined4 param_2,uint param_3)

{
  if ((*(byte *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc) + 0x106) & 4
      ) == 0) {
    unit_dispatch_reaction_animation(((param_3 & 0x200) != 0) + '\x03');
  }
  return;
}
#endif
