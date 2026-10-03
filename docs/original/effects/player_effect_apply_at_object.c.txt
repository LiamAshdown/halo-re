// player_effect_apply_at_object  (Ghidra: FUN_00456900, still unnamed there; named directly by
//   out/phase4/effects_types_notes.md: "player_effect_apply_at_object 0x456900")
// address 0x456900, size 115 bytes
// name confidence: 0.5   rewrite confidence: 0.9 (VERIFIED against objdump 0x456900..0x456972)
// evidence: types/game.h player_globals.local_players (+0x04) and player.unit (+0x34); this
//   module's player_effect_apply_continuous_damage (0x4567c0), whose (tag_reference,
//   local_player_index, distance) signature this function's tail call feeds.
// register convention: damage origin point in ESI (unaff_ESI); the tag reference and local
//   player index this function passes straight through to player_effect_apply_continuous_damage
//   are never read here, so they must themselves be live-through register parameters of this
//   function (EAX, DX) rather than something it computes.
//   // blam-cc: stack -> tag_reference, ESI -> origin; local_player_index is always 0 (xor edx,edx at 0x456943)
//   //   (pass-through), unaff_ESI -> origin
// UNSURE: the pass-through register parameters are inferred from player_effect_apply_continuous_damage's
//   own established signature, not from anything visible in this function's own decompile.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "game.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *player_data;             // 0x0087a480
extern player_globals *local_player_globals; // 0x0087a478

extern double sqrt(double x); // x87 FSQRT
extern void object_get_position(real_point3d *out, uint32_t object_index); // 0x4f6900, objects module
extern void player_effect_apply_continuous_damage(uint32_t tag_reference,
    int16_t local_player_index, float distance); // 0x4567c0, this module

void player_effect_apply_at_object(uint32_t tag_reference, int16_t local_player_index,
    real_point3d *origin) // blam-cc: in_EAX, in_DX, unaff_ESI
{
    datum_index player_index = local_player_globals->local_players[0];

    if (player_index != (datum_index)0xffffffff) {
        player *record = &((player *)player_data->data)[player_index & 0xffff];

        if (record->unit != (datum_index)0xffffffff) {
            real_point3d position;
            float dx, dy, dz;

            object_get_position(&position, record->unit);
            dx = origin->x - position.x;
            dy = origin->y - position.y;
            dz = origin->z - position.z;
            player_effect_apply_continuous_damage(tag_reference, local_player_index,
                (float)sqrt((double)(dy * dy + dx * dx + dz * dz)));
        }
    }
}

#if 0
Original Ghidra decompilation (0x456900):

void FUN_00456900(void)

{
  float *unaff_ESI;
  float local_c;
  float local_8;
  float local_4;

  if ((*(uint *)(DAT_0087a478 + 4) != 0xffffffff) &&
     (*(int *)((*(uint *)(DAT_0087a478 + 4) & 0xffff) * 0x200 + 0x34 + *(int *)(DAT_0087a480 + 0x34)
              ) != -1)) {
    object_get_position();
    FUN_004567c0(SQRT((unaff_ESI[1] - local_8) * (unaff_ESI[1] - local_8) +
                      (*unaff_ESI - local_c) * (*unaff_ESI - local_c) +
                      (unaff_ESI[2] - local_4) * (unaff_ESI[2] - local_4)));
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
