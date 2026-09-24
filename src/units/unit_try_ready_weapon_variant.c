// unit_try_ready_weapon_variant  (Ghidra: FUN_00569b30)
// address 0x569b30, size 116 bytes, name confidence 0.3, rewrite confidence 0.4
// functions.md: "A variant weapon-mode gate similar to unit_try_ready_weapon, additionally
// checking unit-type and a flag at offset 0x4cc before allowing the state change."
// evidence: types/objects.h object.type (0xb4); types/units.h biped_data.flags (0x4cc, bit 0 =
//   grounded) -- the same byte overlaps vehicle_data.flags for a non-biped unit, per
//   out/phase4/units_types_notes.md's "overlap hazard" note; kept as a raw offset since the
//   condition is meant to apply regardless of which extension actually owns it.
// blam-cc: unaff_ESI -> unit_index, unaff_EDI -> fire_trigger_event.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data; // 0x008603b0

extern uint8_t unit_try_set_animation_state(uint32_t unit_index, int16_t new_state); // 0x565f90
extern void unit_set_throw_aim_direction(uint32_t unit_index); // 0x5704d0, UNSURE signature  // real signature (unit_set_throw_aim_direction.c): void unit_set_throw_aim_direction(uint32_t object_index, float direction_x, float direction_y); Ghidra recovered 1 of 3 args at this call site

uint8_t unit_try_ready_weapon_variant(uint32_t unit_index, int32_t fire_trigger_event) // blam-cc: unaff_ESI, unaff_EDI
{
    object *unit_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
    uint8_t result = 0;

    switch (unit->animation_state) {
    case 0x17: case 0x18: case 0x19: case 0x1a: case 0x1b: case 0x1d: case 0x1e: case 0x1f:
    case 0x20: case 0x21: case 0x22: case 0x23: case 0x27: case 0x29:
        break;
    default:
        if (((unit_obj->type != _object_type_biped) ||
             ((*(uint8_t *)((uint8_t *)unit_obj + k_unit_object_size) & 1) == 0)) &&
            (unit_try_set_animation_state(unit_index, 0x19) != 0)) {
            if (fire_trigger_event != 0) {
                unit_set_throw_aim_direction(unit_index);
            }
            result = 1;
        }
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x569b30):

undefined1 FUN_00569b30(void)

{
  int iVar1;
  char cVar2;
  undefined1 uVar3;
  uint unaff_ESI;
  int unaff_EDI;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (unaff_ESI & 0xffff) * 0xc);
  uVar3 = 0;
  switch(*(undefined1 *)(iVar1 + 0x2a3)) {
  case 0x17:
  case 0x18:
  case 0x19:
  case 0x1a:
  case 0x1b:
  case 0x1d:
  case 0x1e:
  case 0x1f:
  case 0x20:
  case 0x21:
  case 0x22:
  case 0x23:
  case 0x27:
  case 0x29:
    break;
  default:
    if (((*(short *)(iVar1 + 0xb4) != 0) || ((*(byte *)(iVar1 + 0x4cc) & 1) == 0)) &&
       (cVar2 = unit_try_set_animation_state(), cVar2 != '\0')) {
      if (unaff_EDI != 0) {
        FUN_005704d0();
      }
      uVar3 = 1;
    }
  }
  return uVar3;
}
#endif
