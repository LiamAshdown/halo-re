// biped_update_idle_basis  (Ghidra: biped_update_idle_basis, renamed)
// address 0x55e840, size 157 bytes
// name confidence: 0.3   rewrite confidence: 0.9 (checked against objdump 0x55e840..0x55e8dc)
// evidence: biped_data.flags bit 0x20 / ground_adjust_iteration / ground_adjust_iteration_limit
//   (0x524/0x525, types/units.h); biped_data.unknown_501 ("ticks in the current grounded
//   state"); unit_data.animation_state (0x2a3); Biped.biped_flags bit 0x400.
// register convention: object index in ESI, a 2-byte animation-state output array in EDI.
//   // blam-cc: ESI -> object_index, EDI -> state_out

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern uint32_t biped_ground_adjust_step(uint32_t object_index); // 0x557a90, stack (0x55e883 pushes only the object)
extern void unit_rotate_basis_about_axis(uint32_t object_index);      // 0x55e6b0, this batch
extern void unit_update_up_vector(Biped *biped_tag, object *obj); // 0x560800, next batch

// Selects between three per-tick basis states for an idle biped: while a ground-adjust solve is
// still in progress, keeps stepping it (unit_rotate_basis_about_axis is not reached here); once
// grounded for more than 2 ticks (and the tag doesn't request otherwise), holds the "idle basis
// refresh" state, re-running unit_rotate_basis_about_axis once on entry; otherwise resets the
// idle refresh state and levels the up-vector via unit_update_up_vector.
void biped_update_idle_basis(uint32_t object_index, uint8_t *state_out)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    Biped *tag = (Biped *)tag_instances[obj->definition_tag & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    biped_data *biped = (biped_data *)((uint8_t *)obj + k_unit_object_size);

    if ((biped->flags & 0x20) != 0 && biped->ground_adjust_iteration < biped->ground_adjust_iteration_limit) {
        biped_ground_adjust_step(object_index);
        state_out[1] = 0;
        return;
    }

    if ((int8_t)biped->airborne_ticks > 2 && (tag->biped_flags & 0x400) == 0) {
        if (unit->animation_state == 0x18) {
            unit_rotate_basis_about_axis(object_index); // 0x55e8b0: EAX = object
        }
        state_out[0] = 0x18;
        state_out[1] = 0;
        return;
    }

    if (unit->animation_state == 0x18) {
        biped->bank_angle = 0.0f;
        // EAX -> the Biped tag, ECX -> the object; both are register-carried, and this
        // function already has them to hand.
        unit_update_up_vector(tag, obj);
    }
    state_out[0] = 0x19;
    state_out[1] = 0;
}

#if 0
Original Ghidra decompilation (0x55e840):

void FUN_0055e840(void)

{
  uint *puVar1;
  uint unaff_ESI;
  undefined1 *unaff_EDI;

  puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (unaff_ESI & 0xffff) * 0xc);
  if (((puVar1[0x133] & 0x20) != 0) && ((byte)puVar1[0x149] < *(byte *)((int)puVar1 + 0x525))) {
    FUN_00557a90();
    unaff_EDI[1] = 0;
    return;
  }
  if (('\x02' < *(char *)((int)puVar1 + 0x501)) &&
     ((*(uint *)(*(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x2f4) & 0x400) == 0))
  {
    if (*(char *)((int)puVar1 + 0x2a3) == '\x18') {
      FUN_0055e6b0();
    }
    *unaff_EDI = 0x18;
    unaff_EDI[1] = 0;
    return;
  }
  if (*(char *)((int)puVar1 + 0x2a3) == '\x18') {
    puVar1[0x144] = 0;
    FUN_00560800();
  }
  *unaff_EDI = 0x19;
  unaff_EDI[1] = 0;
  return;
}
#endif
