// unit_evaluate_flee_reaction  (Ghidra: unit_evaluate_flee_reaction, renamed)
// address 0x55e2d0, size 450 bytes
// name confidence: 0.3   rewrite confidence: 0.85
// FIXED from objdump 0x55e2d0..0x55e491: EDI -> object_index; both ground probes get the unit, the direction
//   (global down, then the parent's velocity * 60 less gravity * 1800) and the normal out; the 0x26 test is the
//   parent's angular velocity (+0x8c).
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

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern game_time_globals *game_time; // 0x006f1d6c, the game time globals (types/game.h)

extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990
extern real vector3d_length(real_vector3d *v);                 // 0x401960, EAX
extern void ai_communication_broadcast(int32_t event_code, datum_index unit_index, datum_index object_a,
    int32_t reason, datum_index object_b, datum_index object_c, uint32_t *extra_data); // 0x42d340, all stack
extern int32_t unit_test_placement_candidate(uint32_t unit_index, const real_vector3d *direction,
    real_vector3d *out_normal, float distance, real_point3d *out_position); // 0x55aa20, ECX, ESI, EBX, stack
extern real_vector3d *global_down3d_pointer; // 0x0069672c
extern float k_physics_gravity; // 0x0069c52c

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
        (int8_t)unit->weapon_control_idle_ticks > 0x78 && *(uint8_t *)((uint8_t *)parent + 0x4d0) > 0x1e &&
        (biped->last_falling_reaction_tick == -1 ||
         (int32_t)(biped->last_falling_reaction_tick + 0xf) < game_time->game_time)) {
        real_vector3d direction;
        real_vector3d normal;

        biped->last_falling_reaction_tick = game_time->game_time;
        // 0x55e37e: no ground within 8 below; then along the parent's velocity (per second, less 1800 g):
        // nothing there, or too steep (normal.k <= 0.3), and the rider screams 0x28
        if (unit_test_placement_candidate(object_index, global_down3d_pointer, 0, 8.0f, 0) == -1) {
            direction.i = parent->velocity.i * 60.0f;
            direction.j = parent->velocity.j * 60.0f;
            direction.k = parent->velocity.k * 60.0f - k_physics_gravity * 1800.0f;
            if (!(vector3d_normalize_with_length(&direction) > 0.0f) ||
                unit_test_placement_candidate(object_index, &direction, &normal, 8.0f, 0) == -1 ||
                !(normal.k > 0.3f)) {
                ai_communication_broadcast(0x28, object_index, (datum_index)-1, -1, (datum_index)-1, (datum_index)-1, 0);
                return;
            }
        }
        // 0x55e42f: upright and barely spinning (angular velocity < 3 degrees) is 0x26, else 0x27
        if (parent->up.k > 0.6f && vector3d_length(&parent->angular_velocity) < 0.05235988f) {
            ai_communication_broadcast(0x26, object_index, (datum_index)-1, -1, (datum_index)-1, (datum_index)-1, 0);
            return;
        }
        ai_communication_broadcast(0x27, object_index, (datum_index)-1, -1, (datum_index)-1, (datum_index)-1, 0);
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
        ai_communication_broadcast(0x28, object_index, (datum_index)-1, -1, (datum_index)-1, (datum_index)-1, 0);
        return;
      }
      iVar2 = FUN_0055aa20(0x41000000,0);
      if ((iVar2 == -1) || (local_4 <= 0.3)) goto LAB_0055e413;
    }
    if (0.6 < (float)puVar1[0x22]) {
      fVar3 = (float10)vector3d_length();
      if (fVar3 < (float10)0.05235988) {
        ai_communication_broadcast(0x26, object_index, (datum_index)-1, -1, (datum_index)-1, (datum_index)-1, 0);
        return;
      }
    }
    ai_communication_broadcast(0x27, object_index, (datum_index)-1, -1, (datum_index)-1, (datum_index)-1, 0);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
