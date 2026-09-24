// biped_get_cached_look_at_position  (Ghidra: biped_get_cached_look_at_position, renamed)
// address 0x55ab30, size 464 bytes
// name confidence: 0.35   rewrite confidence: 0.4
// evidence: types/units.h biped_data fields unknown_4dc ("the cached look-at result"),
//   unknown_4e0 ("the cached look-at point 0x55ab30 refreshes"), unknown_4ec ("game tick that
//   cache was last refreshed"), unknown_4f0 ("the previous value of unknown_4dc"), unknown_4fc
//   ("the target 0x55e0a0 is tracking") -- all already attributed to this function by name in
//   the header, so used directly rather than re-derived.
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

extern char FUN_005014a0(datum_index target, int32_t param_2, int32_t param_3); // UNSURE module
extern void FUN_00501470(int32_t param_1, real_point3d *point);                 // UNSURE module
extern void FUN_005015a0(datum_index target, int32_t param_2, int32_t param_3, real_point3d *point,
                          void *scratch8);                                      // UNSURE module
extern void FUN_0044d860(real_point3d *point);                                  // UNSURE module
// object_get_position (0x4f6900, defined in src/objects/object_get_position.c) writes the
// object position through the pointer in EAX and leaves that same pointer in EAX on return;
// the object index is in ECX. Ghidra binds a different subset of the two operands at each call
// site in this module, so the declaration is left unprototyped.
extern real_point3d *object_get_position();
extern uint32_t unit_test_placement_candidate(float distance, real_point3d *out_position,
                                               real_vector3d *direction, void **out_hit_object); // 0x55aa20, this batch

// Periodically refreshes and returns the biped's cached target look-at position
// (biped_data.unknown_4dc/unknown_4e0). While unattached (or the Biped tag's bit 0x4 is set)
// and the cache is empty and due for a refresh, it re-resolves either the previously tracked
// reference or the current tracked target through the (unresolved) marker-lookup helpers, and
// falls back to unit_test_placement_candidate if neither yields a result. When attached without
// that tag bit, the cache is invalidated and the caller's output pointer is redirected straight
// at the object's own position.
datum_index biped_get_cached_look_at_position(uint32_t object_index, real_point3d *out_position)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    Biped *tag = (Biped *)tag_instances[obj->definition_tag & 0xffff].data;
    biped_data *biped = (biped_data *)((uint8_t *)obj + k_unit_object_size);
    real_point3d *write_target = out_position;

    if ((tag->biped_flags & 4) == 0 || (obj->vitality_flags & 4) != 0) {
        if (biped->unknown_4dc == k_datum_index_none &&
            (int32_t)biped->unknown_4ec < game_time->game_time) {
            real_point3d point = biped->unknown_4e0;
            biped->unknown_4ec = game_time->game_time;

            if (biped->unknown_4f0 == k_datum_index_none) {
                datum_index target = biped->unknown_4fc;
                if (target != k_datum_index_none && FUN_005014a0(target, 2, 1)) {
                    biped->unknown_4dc = target;
                    FUN_00501470(1, &point);
                    biped->unknown_4dc = target;
                }
            } else {
                uint8_t scratch[8];
                FUN_005015a0(biped->unknown_4f0, 2, 1, &point, scratch);
                FUN_0044d860(&point);
                biped->unknown_4dc = biped->unknown_4f0;
            }

            if (biped->unknown_4dc == k_datum_index_none) {
                biped->unknown_4dc = unit_test_placement_candidate(2.0f, &point, 0, 0); // UNSURE: 0x40000000 == 2.0f
            }
            if (biped->unknown_4dc != k_datum_index_none) {
                biped->unknown_4e0 = point;
                biped->unknown_4f0 = biped->unknown_4dc;
            }
        }
    } else {
        biped->unknown_4dc = k_datum_index_none;
        write_target = object_get_position(); // UNSURE: redirects the write below at the object's
                                               // own position instead of out_position; preserved
                                               // as-is even though it looks like a quirk.
    }

    *write_target = biped->unknown_4e0;
    return biped->unknown_4dc;
}

#if 0
Original Ghidra decompilation (0x55ab30):

uint FUN_0055ab30(uint param_1,uint *param_2)

{
  uint *puVar1;
  char cVar2;
  uint uVar3;
  undefined1 local_14 [8];
  uint local_c;
  uint local_8;
  uint local_4;

  puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  if (((*(byte *)(*(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x2f4) & 4) == 0) ||
     ((*(byte *)((int)puVar1 + 0x106) & 4) != 0)) {
    if ((puVar1[0x137] == 0xffffffff) && ((int)puVar1[0x13b] < (int)*(uint *)(DAT_006f1d6c + 0xc)))
    {
      local_c = puVar1[0x138];
      local_8 = puVar1[0x139];
      local_4 = puVar1[0x13a];
      puVar1[0x13b] = *(uint *)(DAT_006f1d6c + 0xc);
      if (puVar1[0x136] == 0xffffffff) {
        uVar3 = puVar1[0x13c];
        if ((uVar3 != 0xffffffff) && (cVar2 = FUN_005014a0(uVar3,2,1), cVar2 != '\0')) {
          puVar1[0x137] = uVar3;
          FUN_00501470(1,&local_c);
          puVar1[0x137] = uVar3;
        }
      }
      else {
        FUN_005015a0(puVar1[0x136],2,1,puVar1 + 0x138,local_14);
        FUN_0044d860(&local_c);
        puVar1[0x137] = puVar1[0x136];
      }
      if (puVar1[0x137] == 0xffffffff) {
        uVar3 = FUN_0055aa20(0x40000000,&local_c);
        puVar1[0x137] = uVar3;
      }
      if (puVar1[0x137] != 0xffffffff) {
        puVar1[0x138] = local_c;
        puVar1[0x139] = local_8;
        puVar1[0x13a] = local_4;
        puVar1[0x13c] = puVar1[0x137];
      }
    }
  }
  else {
    puVar1[0x137] = 0xffffffff;
    param_2 = (uint *)object_get_position();
  }
  *param_2 = puVar1[0x138];
  param_2[1] = puVar1[0x139];
  param_2[2] = puVar1[0x13a];
  return puVar1[0x137];
}
#endif
