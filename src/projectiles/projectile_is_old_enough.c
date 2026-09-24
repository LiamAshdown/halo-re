// projectile_is_old_enough  (Ghidra: missed_4c1270, created by hand this pass -- Ghidra never
// recovered it as a function; only reachable through the projectile object_type_definition row)
// address 0x4c1270, size 57 bytes
// name confidence: 0.7   rewrite confidence: 0.85
// evidence: named already in out/phase4/projectiles_types_notes.md ("+0x74
//   projectile_is_old_enough -- missed" and "projectile_is_old_enough (0x4c1270): if
//   object[0x0c] == -1 return true, else return game_time >= object[0x0c] +
//   [0x006894c8]"), which is also that file's own correction to types/objects.h's
//   object+0x0c ("a game-tick stamp, not a datum handle"). types/projectiles.h
//   k_projectile_minimum_age_ticks (0x006894c8). global 0x006f1d6c game_time_globals (+0x0c the
//   game tick). This function is the twin of this batch's weapon_is_old_enough (0x4c6290) and
//   equipment_is_old_enough (0x4bc420), which read the two adjacent globals 0x006894c4 and
//   0x006894cc for their own types.
// register convention: object index is a plain stack cdecl parameter, matching the rest of this
//   directly-indexed (non object_try_and_get) family.
// blam-cc: stack -> object_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "projectiles.h"

extern data_array *object_data; // 0x008603b0
extern void *game_time_globals; // 0x006f1d6c, +0x0c the game tick
extern int32_t k_projectile_minimum_age_ticks; // 0x006894c8

// The projectile row's "is old enough" hook (object_type_definition +0x74). An object that has
// never been stamped (object.unknown_00c == -1) always counts as old enough; otherwise it is old
// enough once the game tick has advanced past the stamped tick plus this type's minimum age.
uint8_t projectile_is_old_enough(uint32_t object_index) // blam-cc: stack -> object_index
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    int32_t stamp = obj->unknown_00c;

    if (stamp == -1) {
        return 1;
    }
    return stamp + k_projectile_minimum_age_ticks <= *(int32_t *)((uint8_t *)game_time_globals + 0xc);
}

#if 0
Original Ghidra decompilation (0x4c1270):

undefined4 missed_4c1270(uint param_1)

{
  int iVar1;

  iVar1 = *(int *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc) + 0xc);
  if (iVar1 == -1) {
    return 0xffffff01;
  }
  iVar1 = iVar1 + DAT_006894c8;
  return CONCAT31((int3)((uint)iVar1 >> 8),iVar1 <= *(int *)(DAT_006f1d6c + 0xc));
}
#endif
