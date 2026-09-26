// biped_check_evade_reaction  (Ghidra: biped_check_evade_reaction, renamed)
// address 0x55e190, size 313 bytes
// name confidence: 0.3   rewrite confidence: 0.3
// evidence: unit_data.flags bit 0x1000 "gates evade (0x55e190) and fall damage" and
//   biped_data.unknown_4f8 "0x55e190 and 0x55e2d0 rate-limit their reactions to once every 15
//   ticks" -- both already attributed to this function by types/units.h. object.velocity at
//   0x068 (objects.h); Biped.biped_flags 0x2f4 (types/tags.h).
// UNSURE: DAT_00746fa0+0x18c (the globals tag's grenade table, per types/units.h) and its own
//   +0x94 radius-like field; unit_test_placement_candidate's exact role here (a line-of-sight or
//   clearance probe toward a nearby open position, per functions.md's summary).
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
extern uint8_t *globals_tag_data;   // 0x00746fa0, +0x174 player info, +0x18c/0x190 grenade tables (types/units.h)
extern float unit_evade_scale;      // 0x0069c52c, UNSURE

// object_get_position (0x4f6900, defined in src/objects/object_get_position.c) writes the
// object position through the pointer in EAX and leaves that same pointer in EAX on return;
// the object index is in ECX. Ghidra binds a different subset of the two operands at each call
// site in this module, so the declaration is left unprototyped.
extern real_point3d *object_get_position();
extern uint32_t unit_test_placement_candidate(float distance, real_point3d *out_position,
                                               real_vector3d *direction, void **out_hit_object); // 0x55aa20, this batch
extern void unit_dispatch_reaction_animation(int32_t unit_index, int16_t reaction_code); // 0x5614a0, ESI unit, stack code, next batch: reaction dispatcher

// Rate-limited (every 15 ticks) evasion check: if the unit is unattached, not a special weapon
// type (Biped tag flags 0x84 clear), unattended (flags bit 0x1000 clear), has an actor and isn't
// mid scripted-action, and has been grounded (unknown_501) more than 30 ticks, probes for a
// nearby open position via unit_test_placement_candidate; if none is found, or a clearance/height
// test against the grenade-table radius fails, dispatches an evade reaction (code 0).
void biped_check_evade_reaction(uint32_t object_index)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    Biped *tag = (Biped *)tag_instances[obj->definition_tag & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    biped_data *biped = (biped_data *)((uint8_t *)obj + k_unit_object_size);

    if ((obj->vitality_flags & 4) == 0 && (tag->biped_flags & 0x84) == 0 &&
        (unit->flags & 0x1000) == 0 && unit->actor_index != k_datum_index_none &&
        unit->animation_state != 0x1d && (int8_t)biped->unknown_501 > 0x1e &&
        (biped->unknown_4f8 == -1 ||
         (int32_t)(biped->unknown_4f8 + 0xf) < game_time->game_time)) {
        void *table = *(void **)(globals_tag_data + 0x18c);
        real_point3d position;
        real_vector3d direction = {0};
        uint32_t found;

        biped->unknown_4f8 = game_time->game_time;
        found = unit_test_placement_candidate(6.0f, &position, &direction, 0); // UNSURE: 0x40c00000 == 6.0f

        if (found == k_datum_index_none) {
            unit_dispatch_reaction_animation((int32_t)object_index, 0);
        } else {
            object_get_position(&position);
            {
                float radius = *(float *)((uint8_t *)table + 0x94);
                float dz = (position.z - 0.0f) * unit_evade_scale; // UNSURE: local_10/local_4 pairing, see file header
                if (obj->velocity.k <= 0.0f &&
                    radius * radius <= obj->velocity.k * obj->velocity.k + dz + dz) {
                    unit_dispatch_reaction_animation((int32_t)object_index, 0);
                }
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x55e190):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0055e190(uint param_1)

{
  float fVar1;
  uint *puVar2;
  int iVar3;
  float fVar4;
  int iVar5;
  float local_10;
  undefined1 local_c [8];
  float local_4;

  puVar2 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  if ((((((*(byte *)((int)puVar2 + 0x106) & 4) == 0) &&
        ((*(byte *)(*(int *)((*puVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x2f4) & 0x84) == 0)
        ) && ((puVar2[0x81] & 0x1000) == 0)) &&
      ((puVar2[0x7d] != 0xffffffff && (*(char *)((int)puVar2 + 0x2a3) != '\x1d')))) &&
     (('\x1e' < *(char *)((int)puVar2 + 0x501) &&
      ((puVar2[0x13e] == 0xffffffff ||
       ((int)(puVar2[0x13e] + 0xf) < (int)*(uint *)(DAT_006f1d6c + 0xc))))))) {
    iVar3 = *(int *)(DAT_00746fa0 + 0x18c);
    puVar2[0x13e] = *(uint *)(DAT_006f1d6c + 0xc);
    iVar5 = FUN_0055aa20(0x40c00000,local_c);
    if ((iVar5 == -1) ||
       ((object_get_position(), (float)puVar2[0x1c] <= 0.0 &&
        (fVar1 = *(float *)(iVar3 + 0x94), fVar4 = (local_10 - local_4) * _DAT_0069c52c,
        fVar1 * fVar1 <= (float)puVar2[0x1c] * (float)puVar2[0x1c] + fVar4 + fVar4)))) {
      FUN_005614a0(0);
    }
  }
  return;
}
#endif
