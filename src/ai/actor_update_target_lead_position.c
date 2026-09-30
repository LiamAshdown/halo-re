// actor_update_target_lead_position  (Ghidra: actor_update_target_lead_position, already named)
// address 0x429570, size 161 bytes
// name confidence: 0.5   rewrite confidence: 0.95
// evidence: types/ai.h actor.unknown_164/168/16c/170 (a cached lead point, read/written as
//   floats despite their int32_t typing -- the same "declared type from one accessor,
//   written as float bits by another" pattern seen throughout this module),
//   body_position(0x12c)/flying(0x99)/active_unit_index(0x158)/unit_index(0x18)/
//   movement_context. Calls object_try_and_get (0x4f6ec0), biped_get_cached_look_at_position
//   (0x55ab30) and unit_predict_aim_target_position (0x571de0), all already established
//   elsewhere in this module (src/ai/actor_target_get_relationship_object.c).
// register convention: EAX -> actor_index.
//   // blam-cc: EAX -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "objects.h"
#include "units.h"
#include "fn_ai.h"
#include "fn_units.h"

extern data_array *actor_data; // 0x00880360

extern void *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, ECX, stack


// REWRITTEN from objdump 0x429570..0x429610. EAX: actor. When the cached location (+0x164) is unset, the point
//   (+0x168) starts at the body position (+0x12c) and, unless flying (+0x99), the location comes from the vehicle
//   the actor rides (+0x158, when its seat kind +0x15e is 2..3; 0x571de0) or from its own biped (0x55ab30),
//   both refining the point in place. The draft called object_try_and_get and 0x571de0 without the unit or the
//   point.
// blam-cc: EAX -> actor_index
void actor_update_target_lead_position(datum_index actor_index)
{
    uint8_t *a = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
    real_point3d *point = (real_point3d *)(a + 0x168);
    datum_index vehicle;

    if (((struct actor *)a)->unknown_164 != -1) {
        return;
    }
    *point = *(real_point3d *)&((actor *)a)->body_position.x;
    if (a[0x99]) {
        return;
    }
    vehicle = ((actor *)a)->active_unit_index;
    if (vehicle != k_datum_index_none) {
        int16_t seat_kind = ((struct actor *)a)->movement_context;

        if (seat_kind >= 2 && seat_kind <= 3) {
            ((struct actor *)a)->unknown_164 = unit_predict_aim_target_position(vehicle, point);
        }
        return;
    }
    if (object_try_and_get(((actor *)a)->unit_index, 1) != 0) {
        ((struct actor *)a)->unknown_164 = (int32_t)biped_get_cached_look_at_position(((actor *)a)->unit_index, point);
    }
}

#if 0
Original Ghidra decompilation (0x429570):

void actor_update_target_lead_position(void)

{
  uint in_EAX;
  undefined4 uVar1;
  int iVar2;
  int iVar3;

  iVar2 = (in_EAX & 0xffff) * 0x724;
  iVar3 = iVar2 + *(int *)(DAT_00880360 + 0x34);
  if (*(int *)(iVar2 + 0x164 + *(int *)(DAT_00880360 + 0x34)) == -1) {
    *(undefined4 *)(iVar3 + 0x168) = *(undefined4 *)(iVar3 + 300);
    *(undefined4 *)(iVar3 + 0x16c) = *(undefined4 *)(iVar3 + 0x130);
    *(undefined4 *)(iVar3 + 0x170) = *(undefined4 *)(iVar3 + 0x134);
    if (*(char *)(iVar3 + 0x99) == '\0') {
      if (*(int *)(iVar3 + 0x158) == -1) {
        uVar1 = *(undefined4 *)(iVar3 + 0x18);
        iVar2 = object_try_and_get(1);
        if (iVar2 != 0) {
          uVar1 = FUN_0055ab30(uVar1,(undefined4 *)(iVar3 + 0x168));
          *(undefined4 *)(iVar3 + 0x164) = uVar1;
        }
      }
      else if ((1 < *(short *)(iVar3 + 0x15e)) && (*(short *)(iVar3 + 0x15e) < 4)) {
        uVar1 = FUN_00571de0();
        *(undefined4 *)(iVar3 + 0x164) = uVar1;
        return;
      }
    }
  }
  return;
}
#endif
