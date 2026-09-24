// actor_update_target_lead_position  (Ghidra: actor_update_target_lead_position, already named)
// address 0x429570, size 161 bytes
// name confidence: 0.5   rewrite confidence: 0.35
// evidence: types/ai.h actor.unknown_164/168/16c/170 (a cached lead point, read/written as
//   floats despite their int32_t typing -- the same "declared type from one accessor,
//   written as float bits by another" pattern seen throughout this module),
//   body_position(0x12c)/flying(0x99)/active_unit_index(0x158)/unit_index(0x18)/
//   unknown_15e. Calls object_try_and_get (0x4f6ec0), biped_get_cached_look_at_position
//   (0x55ab30) and unit_predict_aim_target_position (0x571de0), all already established
//   elsewhere in this module (src/ai/actor_target_get_relationship_object.c).
// register convention: EAX -> actor_index.
//   // blam-cc: EAX -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *actor_data; // 0x00880360

extern void *object_try_and_get(int32_t kind); // 0x4f6ec0
extern datum_index biped_get_cached_look_at_position(uint32_t object_index, real_point3d *out_position); // 0x55ab30
extern int32_t unit_predict_aim_target_position(void); // 0x571de0, UNSURE signature (register args not traced at this call site either)

// blam-cc: EAX -> actor_index
// Computes and caches a predicted intercept/lead position for the actor's target: seeds the
// cache from the actor's own body position, then, unless flying, refines it either via the
// local player's cached look-at position (when the actor has no active unit) or via a
// vehicle-specific aim prediction (when unknown_15e selects that path).
void actor_update_target_lead_position(datum_index actor_index)
{
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];

    if (self->unknown_164 == -1) {
        *(float *)&self->unknown_168 = self->body_position.x;
        *(float *)&self->unknown_16c = self->body_position.y;
        *(float *)&self->unknown_170 = self->body_position.z;

        if (self->flying == 0) {
            if (self->active_unit_index == (datum_index)k_datum_index_none) {
                if (object_try_and_get(1) != 0) {
                    self->unknown_164 = (int32_t)biped_get_cached_look_at_position(
                        self->unit_index, (real_point3d *)&self->unknown_168);
                }
            } else if (self->unknown_15e > 1 && self->unknown_15e < 4) {
                self->unknown_164 = unit_predict_aim_target_position();
                return;
            }
        }
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
