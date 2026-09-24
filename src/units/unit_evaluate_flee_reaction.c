// unit_evaluate_flee_reaction  (Ghidra: unit_evaluate_flee_reaction, renamed)
// address 0x55e2d0, size 450 bytes
// name confidence: 0.3   rewrite confidence: 0.25
// evidence: unit_data.actor_index (0x1f4/500 decimal, types/units.h), unit_data.unknown_322
//   ("flees above 120" -- 0x78 hex == 120, types/units.h), biped_data.unknown_4f8 ("0x55e190
//   and 0x55e2d0 rate-limit their reactions to once every 15 ticks", types/units.h). Parent
//   (vehicle) tag offset 0x17c bit 0x40 is UNSURE; parent object offset 0x4d0 could be either
//   biped_data.unknown_4d0 or vehicle_data.airborne_ticks depending on the parent's real type,
//   kept as a raw offset.
// register convention: object index in EDI, no other inputs.
//   // blam-cc: EDI -> object_index
// reconciled: R32 hs_game_time_globals -> game.h game_time_globals (current_tick->game_time, budget_flag_1/2->active/paused, seconds_per_tick->leftover_time; same offsets)

#include "tags.h"
#include "memory.h"
#include "hs.h"
#include "math.h"
#include "game.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern game_time_globals *game_time; // 0x006f1d6c, the game time globals (types/game.h)

extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990
extern real vector3d_length(real_vector3d *v);                 // 0x401960, UNSURE args (mirrors vector3d_normalize_with_length)
extern void ai_communication_broadcast(int32_t line_id);        // 0x42d340
extern uint32_t unit_test_placement_candidate(float distance, real_point3d *out_position,
                                               real_vector3d *direction, void **out_hit_object); // 0x55aa20, this batch

// Evaluates whether a (typically vehicle-mounted) unit should flee or evade: gated on the
// parent vehicle's Unit-tag flag 0x40 (UNSURE), the unit having an actor and not being mid
// scripted-action, and unknown_322 having climbed past 120 ticks. Rate-limited to once every 15
// ticks via biped_data.unknown_4f8. Broadcasts one of three AI communication lines (0x26/0x27/
// 0x28) depending on whether a nearby open position was found and how fast the unit is turning.
void unit_evaluate_flee_reaction(uint32_t object_index)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    biped_data *biped = (biped_data *)((uint8_t *)obj + k_unit_object_size);
    object *parent = ((object_header *)object_data->data)[obj->parent_object & 0xffff].data;
    void *parent_tag = tag_instances[parent->definition_tag & 0xffff].data;

    if ((*(uint8_t *)((uint8_t *)parent_tag + 0x17c) & 0x40) != 0 &&
        unit->actor_index != k_datum_index_none && unit->animation_state != 0x1d &&
        (int8_t)unit->unknown_322 > 0x78 && *(uint8_t *)((uint8_t *)parent + 0x4d0) > 0x1e &&
        (biped->unknown_4f8 == -1 ||
         (int32_t)(biped->unknown_4f8 + 0xf) < game_time->game_time)) {
        real_point3d position;
        real_vector3d direction;

        biped->unknown_4f8 = game_time->game_time;

        if (unit_test_placement_candidate(8.0f, &position, &direction, 0) == k_datum_index_none) { // UNSURE: 0x41000000 == 8.0f
            if (vector3d_normalize_with_length(&direction) <= 0.0f) {
                ai_communication_broadcast(0x28);
                return;
            }
            if (unit_test_placement_candidate(8.0f, &position, &direction, 0) == k_datum_index_none ||
                direction.i <= 0.3f) { // UNSURE: local_4, see file header
                ai_communication_broadcast(0x28);
                return;
            }
        }
        if (parent->up.k > 0.6f) {
            if (vector3d_length(&direction) < 0.05235988f) {
                ai_communication_broadcast(0x26);
                return;
            }
        }
        ai_communication_broadcast(0x27);
    }
}

#if 0
Original Ghidra decompilation (0x55e2d0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0055e2d0(void)

{
  uint *puVar1;
  int iVar2;
  uint unaff_EDI;
  float10 fVar3;
  float local_4;

  iVar2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (unaff_EDI & 0xffff) * 0xc);
  puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (*(uint *)(iVar2 + 0x11c) & 0xffff) * 0xc)
  ;
  if ((((((*(byte *)(*(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x17c) & 0x40) != 0
         ) && (*(int *)(iVar2 + 500) != -1)) && (*(char *)(iVar2 + 0x2a3) != '\x1d')) &&
      (('x' < *(char *)(iVar2 + 0x322) && (0x1e < (byte)puVar1[0x134])))) &&
     ((*(int *)(iVar2 + 0x4f8) == -1 ||
      (*(int *)(iVar2 + 0x4f8) + 0xf < *(int *)(DAT_006f1d6c + 0xc))))) {
    *(int *)(iVar2 + 0x4f8) = *(int *)(DAT_006f1d6c + 0xc);
    iVar2 = FUN_0055aa20(0x41000000,0);
    if (iVar2 == -1) {
      fVar3 = (float10)vector3d_normalize_with_length();
      if (fVar3 <= (float10)0.0) {
LAB_0055e413:
        ai_communication_broadcast(0x28);
        return;
      }
      iVar2 = FUN_0055aa20(0x41000000,0);
      if ((iVar2 == -1) || (local_4 <= 0.3)) goto LAB_0055e413;
    }
    if (0.6 < (float)puVar1[0x22]) {
      fVar3 = (float10)vector3d_length();
      if (fVar3 < (float10)0.05235988) {
        ai_communication_broadcast(0x26);
        return;
      }
    }
    ai_communication_broadcast(0x27);
  }
  return;
}
#endif
