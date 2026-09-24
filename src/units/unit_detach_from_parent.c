// unit_detach_from_parent  (Ghidra: already named unit_detach_from_parent)
// address 0x570140, size 98 bytes
// name confidence: 0.6 (functions.md summary matches)
// rewrite confidence: 0.15 -- zero recorded callers and every input arriving as an "unaff_"
//   register are the signature of a shared tail block Ghidra split out of a larger function
//   rather than a real call target; unaff_EBX (the object base) cannot be resolved here.
// evidence: types/objects.h object.flags (0x010); types/units.h unit_data.flags (0x204, bit
//   0x8000 = _unit_flag_detached); callees vector3d_cross_product, object_set_position_and_relink,
//   object_attach_to_object (all established elsewhere in this module), unit_try_ready_weapon (sets
//   melee_state per other files' evidence, called here as a "reset" with (1,0)).
// register convention: UNRESOLVED (unaff_EBX must come from the caller this block was split
//   from).
//   // blam-cc: UNSURE -- see header
// UNSURE: reproduced as literally as possible; the object.flags |= 0x20 and
//   unit_data.flags |= 0x8000 writes and the unit_try_ready_weapon(1, 0) call are the only parts with
//   unambiguous meaning regardless of which object unaff_EBX names.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"

extern void vector3d_cross_product(real_vector3d *out, real_vector3d *ecx_operand,
                                    real_vector3d *stack_operand); // 0x4052c0, UNSURE args here
extern void object_set_position_and_relink(real_point3d *position, uint32_t object_index); // 0x4f5350
extern void object_attach_to_object(uint32_t parent_index, uint32_t child_index,
                                     uint32_t marker_word); // 0x4f6440
extern void unit_try_ready_weapon(int32_t a, int32_t b); // 0x569a20  // real signature (unit_try_ready_weapon.c): uint8_t unit_try_ready_weapon(uint32_t unit_index, uint8_t is_melee, int32_t fire_trigger_event); Ghidra recovered 2 of 3 args at this call site

// Detaches the unit from its current parent/attachment object.
// UNSURE: see file header -- this function's real base pointer (unaff_EBX) is not recoverable
// from the decompile alone.
void unit_detach_from_parent(uint32_t unit_index, real_vector3d *cross_out,
                              real_vector3d *cross_ecx_operand, real_vector3d *cross_stack_operand,
                              real_point3d *reposition_target)
{
    object *obj = 0; // UNSURE: stands in for unaff_EBX; see file header

    vector3d_cross_product(cross_out, cross_ecx_operand, cross_stack_operand);
    object_set_position_and_relink(reposition_target, unit_index);
    object_attach_to_object(unit_index, unit_index, 0); // UNSURE: real arguments not recoverable

    obj->flags |= 0x20;
    {
        unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
        unit->flags |= 0x8000;
    }
    unit_try_ready_weapon(1, 0);
}

#if 0
Original Ghidra decompilation (0x570140):

void unit_detach_from_parent(void)

{
  int unaff_EBX;
  undefined4 in_stack_00000024;
  undefined4 in_stack_0000003c;

  vector3d_cross_product();
  object_set_position_and_relink(in_stack_0000003c);
  object_attach_to_object(in_stack_00000024);
  *(uint *)(unaff_EBX + 0x10) = *(uint *)(unaff_EBX + 0x10) | 0x20;
  *(uint *)(unaff_EBX + 0x204) = *(uint *)(unaff_EBX + 0x204) | 0x8000;
  FUN_00569a20(1,0);
  return;
}
#endif
