// weapon_is_old_enough  (Ghidra: missed_4c6290, created by hand this pass -- Ghidra never
// recovered it as a function; only reachable through the weapon object_type_definition row)
// address 0x4c6290, size 57 bytes
// name confidence: 0.6   rewrite confidence: 0.8
// evidence: the weapon row (0x0069b748) carries this address at +0x74, the same column that
//   holds equipment_is_old_enough (0x4bc420, this batch) and projectile_is_old_enough (0x4c1270,
//   already named in out/phase4/projectiles_types_notes.md, "projectile row +0x74"); byte for
//   byte identical body apart from the per-type age-threshold global. types/objects.h
//   object.network_update_tick -- per out/phase4/projectiles_types_notes.md's correction ("types/
//   objects.h: object + 0x0c is a game-tick stamp, not a datum handle"). global 0x006f1d6c
//   game_time (+0x0c the game tick).
// register convention: object index is a plain stack cdecl parameter, matching the rest of this
//   directly-indexed (non object_try_and_get) family.
// blam-cc: stack -> object_index
// UNSURE: 0x006894c4 has no established name; declared here as k_weapon_minimum_age_ticks by
//   analogy with types/projectiles.h's k_projectile_minimum_age_ticks (0x006894c8) and this
//   batch's equipment_is_old_enough (0x006894cc, k_equipment_minimum_age_ticks).
// reconciled: R27 object.unknown_00c (datum_index) -> int32_t network_update_tick (game tick stamp, -1 = never)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "items.h"
#include "game.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data; // 0x008603b0
extern game_time_globals *game_time; // 0x006f1d6c
extern int32_t k_weapon_minimum_age_ticks; // 0x006894c4, UNSURE name

// The weapon row's "is old enough" hook (object_type_definition +0x74). An object that has never
// been stamped (object.network_update_tick == -1) always counts as old enough; otherwise it is old
// enough once the game tick has advanced past the stamped tick plus this type's minimum age.
uint8_t weapon_is_old_enough(uint32_t object_index) // blam-cc: stack -> object_index
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    int32_t stamp = obj->network_update_tick;

    if (stamp == -1) {
        return 1;
    }
    return stamp + k_weapon_minimum_age_ticks <= game_time->game_time;
}

#if 0
Original Ghidra decompilation (0x4c6290):

undefined4 missed_4c6290(uint param_1)

{
  int iVar1;

  iVar1 = *(int *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc) + 0xc);
  if (iVar1 == -1) {
    return 0xffffff01;
  }
  iVar1 = iVar1 + DAT_006894c4;
  return CONCAT31((int3)((uint)iVar1 >> 8),iVar1 <= *(int *)(DAT_006f1d6c + 0xc));
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
