// game_engine_rate_location_crowding  (Ghidra: FUN_00461ad0)
// address 0x461ad0, size 388 bytes
// name confidence: 0.5   rewrite confidence: 0.4
// RENAMED by review (was game_engine_compute_proximity_damage_scale). The only caller is
// game_engine_rate_player_starting_location (0x461d90) -- see that file for the proof that the
// whole 0x461ad0 / 0x461c60 / 0x461d90 chain is the player starting-location scorer, not damage
// scaling. `point` is a candidate ScenarioPlayerStartingLocation's position, and the thresholds
// this function applies (0.0 below 0.25 world units of any player's unit, x0.1 out to 1.0, and
// for players on another team 0.0 below 2.0 ramping linearly back to full by 5.0) are Halo's
// familiar spawn-crowding rules.
// evidence: types/game.h player::unit (+0x34), player::team (+0x20), game_variant::teams (+0x34,
// aliased 0x006f1cbc); types/memory.h data_iterator; src/objects/object_get_position.c.
// register convention: a player index in EAX (in_EAX); the candidate point is the one stack
// parameter. Self is NOT excluded from the walk -- the caller only ever asks about a location
// the player is not standing on yet.
//   // blam-cc: EAX -> self_index, stack -> point
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "fn_game.h"
#include <stdint.h>

extern data_array *player_data; // 0x0087a480
extern game_engine_definition *current_game_engine; // 0x006f1d20
extern game_variant game_engine_variant;            // 0x006f1c88 (teams aliased 0x006f1cbc)

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0
extern void object_get_position(real_point3d *out, datum_index object_index); // 0x4f6900, blam-cc: EAX -> out, ECX -> object_index (matches src/objects/object_get_position.c)
extern double sqrt(double x); // a single x87 FSQRT instruction

// blam-cc: EAX -> self_index, stack -> point
float game_engine_rate_location_crowding(uint32_t self_index, real_point3d *point)
{
    uint8_t no_engine = (current_game_engine == 0);
    player *self = (player *)((uint8_t *)player_data->data + (self_index & 0xffff) * sizeof(player));
    uint8_t teams_enabled = (uint8_t)game_engine_variant.teams;
    float scale = 1.0f;
    data_iterator iter;
    void *element;

    iter.data = player_data;
    iter.next_index = 0;
    iter.index = (datum_index)0xffffffff;
    iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;

    element = data_iterator_next(&iter);
    while (element != 0) {
        player *other = (player *)element;

        if (other->unit != (datum_index)0xffffffff) {
            real_point3d other_position;
            float dx, dy, dz, distance;

            object_get_position(&other_position, other->unit);
            dx = point->x - other_position.x;
            dy = point->y - other_position.y;
            dz = point->z - other_position.z;
            distance = (float)sqrt((double)(dx * dx + dy * dy + dz * dz));

            if ((((no_engine ? 0u : 0xffffffffu) & teams_enabled) == 0) ||
                (other->team != self->team) || (distance <= 0.25f)) {
                if (0.25f <= distance) {
                    if (distance < 1.0f) {
                        scale = scale * 0.1f;
                    }
                } else {
                    scale = 0.0f;
                }
                if (other->team != self->team) {
                    if (2.0f <= distance) {
                        if (distance <= 5.0f) {
                            scale = (distance - 2.0f) * scale * 0.33333334f;
                        }
                    } else {
                        scale = 0.0f;
                    }
                }
            }
        }
        element = data_iterator_next(&iter);
    }
    return scale;
}

#if 0
Original Ghidra decompilation (0x461ad0), from tools/pack.py 0x461ad0:

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

float10 FUN_00461ad0(float *param_1)

{
  float fVar1;
  byte bVar2;
  uint in_EAX;
  int iVar3;
  int iVar4;
  bool bVar5;
  float local_28;
  float local_1c;
  float local_18;
  float local_14;

  bVar5 = DAT_006f1d20 == 0;
  iVar3 = (in_EAX & 0xffff) * 0x200 + *(int *)(DAT_0087a480 + 0x34);
  bVar2 = (byte)_DAT_006f1cbc;
  local_28 = 1.0;
  iVar4 = data_iterator_next();
  while (iVar4 != 0) {
    if (*(int *)(iVar4 + 0x34) != -1) {
      object_get_position();
      fVar1 = SQRT((*param_1 - local_1c) * (*param_1 - local_1c) +
                   (param_1[1] - local_18) * (param_1[1] - local_18) +
                   (param_1[2] - local_14) * (param_1[2] - local_14));
      if ((((bVar5 - 1U & bVar2) == 0) || (*(int *)(iVar4 + 0x20) != *(int *)(iVar3 + 0x20))) ||
         (fVar1 <= 0.25)) {
        if (0.25 <= fVar1) {
          if (fVar1 < 1.0) {
            local_28 = local_28 * 0.1;
          }
        }
        else {
          local_28 = 0.0;
        }
        if (*(int *)(iVar4 + 0x20) != *(int *)(iVar3 + 0x20)) {
          if (2.0 <= fVar1) {
            if (fVar1 <= 5.0) {
              local_28 = (fVar1 - 2.0) * local_28 * 0.33333334;
            }
          }
          else {
            local_28 = 0.0;
          }
        }
      }
    }
    iVar4 = data_iterator_next();
  }
  return (float10)local_28;
}
#endif
