// biped_reset_state  (Ghidra: no function created; the phase-4 types agent carved a placeholder
//   "missed_559f10" from the object_type_definition vtable evidence)
// address 0x559f10, size 94 bytes
// name confidence 0.4, rewrite confidence 0.65
// evidence: out/phase4/units_types_notes.md: "The biped row's +0x38, +0x50 and +0x54 columns are
//   0x559e40, 0x559f10 and 0x559f70 ... 0x559f10 is the biped reset hook, i.e. the function that
//   would have pinned biped_data's initial values the way 0x570b00 pins the vehicle's." Named to
//   match vehicle_reset_state.c (0x570b00, the vehicle row's own +0x50 column), which zeroes
//   vehicle_data's live 0x4cc..0x520 span the same way this function zeroes the whole 0x84-byte
//   biped_data extension (0x21 dwords = 0x84 bytes, exactly types/units.h's biped_data size),
//   then reseeds ground_normal/unknown_520 (+0x514..+0x520) from the same k_default_resting_plane
//   constant object_physics_mass_point_resolve_ground_contact.c already established (0x0069c53c),
//   and finally marks unknown_4f8 (the "reacted within the last 15 ticks" rate-limit stamp) with
//   the standard datum_index/-1 style invalid sentinel this module uses throughout.
// register convention: object index in a single register argument (matches every other biped_*
//   per-object helper in this address range); blam-cc: object_index only.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;           // 0x008603b0
extern float k_default_resting_plane[4];  // 0x0069c53c

// object_type_definition "biped" row, +0x50 column. Clears the whole biped_data extension to
// zero, reseeds its ground_normal/unknown_520 from the shared default resting plane, and marks
// unknown_4f8's rate-limit stamp invalid.
void biped_reset_state(uint32_t object_index)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    biped_data *biped = (biped_data *)((uint8_t *)obj + k_unit_object_size);
    uint32_t *field = (uint32_t *)biped;
    int32_t i;

    for (i = 0x21; i != 0; i--) {
        *field++ = 0;
    }

    biped->ground_normal.i = k_default_resting_plane[0];
    biped->ground_normal.j = k_default_resting_plane[1];
    biped->ground_normal.k = k_default_resting_plane[2];
    biped->ground_plane_distance = *(uint32_t *)&k_default_resting_plane[3];
    biped->last_falling_reaction_tick = -1;
}

#if 0
Original Ghidra decompilation (0x559f10):

void missed_559f10(uint param_1)

{
  int iVar1;
  int iVar2;
  undefined4 *puVar3;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  puVar3 = (undefined4 *)(iVar1 + 0x4cc);
  for (iVar2 = 0x21; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar3 = 0;
    puVar3 = puVar3 + 1;
  }
  *(undefined4 *)(iVar1 + 0x514) = DAT_0069c53c;
  *(undefined4 *)(iVar1 + 0x518) = DAT_0069c540;
  *(undefined4 *)(iVar1 + 0x51c) = DAT_0069c544;
  *(undefined4 *)(iVar1 + 0x520) = DAT_0069c548;
  *(undefined4 *)(iVar1 + 0x4f8) = 0xffffffff;
  return;
}
#endif
