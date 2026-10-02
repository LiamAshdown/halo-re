// game_engine_rate_location_ally_bonus  (Ghidra: FUN_00461c60)
// address 0x461c60, size 300 bytes
// name confidence: 0.5   rewrite confidence: 0.4
// RENAMED by review (was game_engine_compute_nearby_ally_bonus -- same idea, wrong subsystem).
// The only caller is game_engine_rate_player_starting_location (0x461d90), which passes a
// candidate ScenarioPlayerStartingLocation's position in ESI; see that file for the proof that
// this chain scores spawn points, not damage. Calling convention detail: 0x461d90 reaches it
// with "mov eax,ebp ; call 0x461c60" and NO push, so the point really does arrive in ESI.
// evidence: out/phase4/game_functions.md ("Computes a scaling bonus that grows with the number
// of nearby same-team entities within a mid-range distance band"); types/game.h player::team
// (+0x20), player::unit (+0x34).
// register convention: a player index in EAX (in_EAX); the search point in ESI (unaff_ESI),
// forwarded unchanged from this function's own (unrecovered) caller.
//   // blam-cc: EAX -> self_index, unaff_ESI -> point
// UNSURE: FUN_006283c0 is outside this batch's own range and evidence; it is called once per
// qualifying nearby ally with no visible arguments (matching Ghidra's own rendering) and its
// return value summed into the bonus accumulator. `(fVar2 < 6.0) != (fVar2 == 6.0)` is
// transcribed as the logically equivalent `distance <= 6.0f`.
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *player_data; // 0x0087a480

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0
extern void object_get_position(real_point3d *out, datum_index object_index); // 0x4f6900, blam-cc: EAX -> out, ECX -> object_index (matches src/objects/object_get_position.c)
extern double sqrt(double x); // a single x87 FSQRT instruction
extern double pow(double base, double exponent); // C runtime (the retail copy is the CRT _CIpow at 0x6283c0)

// blam-cc: EAX -> self_index, unaff_ESI -> point
float game_engine_rate_location_ally_bonus(uint32_t self_index, real_point3d *point)
{
    player *self = (player *)((uint8_t *)player_data->data + (self_index & 0xffff) * sizeof(player));
    float bonus = 0.0f;
    data_iterator iter;
    void *element;

    iter.data = player_data;
    iter.next_index = 0;
    iter.index = (datum_index)0xffffffff;
    iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;

    element = data_iterator_next(&iter);
    if (element != 0) {
        do {
            player *other = (player *)element;
            if (other->team == self->team && other->unit != (datum_index)0xffffffff) {
                real_point3d other_position;
                float dx, dy, dz, distance;

                object_get_position(&other_position, other->unit);
                dx = point->x - other_position.x;
                dy = point->y - other_position.y;
                dz = point->z - other_position.z;
                distance = (float)sqrt((double)(dx * dx + dy * dy + dz * dz));

                if (1.0f <= distance && distance <= 6.0f) {
                    // FIXED 2026-09-28: 0x461d23..0x461d3f computes pow(1 - (distance - 1) * 0.2, 0.6) (0x00673008 is the double 0.6f).
                    bonus = bonus + (float)pow((double)(1.0f - (distance - 1.0f) * 0.2f), (double)0.6f);
                }
            }
            element = data_iterator_next(&iter);
        } while (element != 0);

        if (3.0f < bonus) {
            bonus = 3.0f;
        }
    }
    return bonus * 3.0f + 1.0f;
}

#if 0
Original Ghidra decompilation (0x461c60), from tools/pack.py 0x461c60:

float10 FUN_00461c60(void)

{
  int iVar1;
  float fVar2;
  uint in_EAX;
  int iVar3;
  float *unaff_ESI;
  float10 fVar4;
  float local_24;
  float local_1c;
  float local_18;
  float local_14;

  iVar1 = *(int *)(DAT_0087a480 + 0x34);
  local_24 = 0.0;
  iVar3 = data_iterator_next();
  if (iVar3 != 0) {
    do {
      if ((*(int *)((in_EAX & 0xffff) * 0x200 + iVar1 + 0x20) == *(int *)(iVar3 + 0x20)) &&
         (*(int *)(iVar3 + 0x34) != -1)) {
        object_get_position();
        fVar2 = SQRT((*unaff_ESI - local_1c) * (*unaff_ESI - local_1c) +
                     (unaff_ESI[1] - local_18) * (unaff_ESI[1] - local_18) +
                     (unaff_ESI[2] - local_14) * (unaff_ESI[2] - local_14));
        if ((1.0 <= fVar2) && (fVar2 < 6.0 != (fVar2 == 6.0))) {
          fVar4 = (float10)FUN_006283c0();
          local_24 = (float)(fVar4 + (float10)local_24);
        }
      }
      iVar3 = data_iterator_next();
    } while (iVar3 != 0);
    if (3.0 < local_24) {
      local_24 = 3.0;
    }
  }
  return (float10)local_24 * (float10)3.0 + (float10)1.0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
