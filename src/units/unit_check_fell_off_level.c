// unit_check_fell_off_level  (Ghidra: unit_check_fell_off_level, renamed)
// address 0x55e4a0, size 78 bytes
// name confidence: 0.35   rewrite confidence: 0.55
// evidence: object.flags/location_cluster_index/position (0x010/0x09c/0x05c, objects.h);
//   0x006f1d20 is "the network / predicted-state flag every damage and seat path branches on"
//   (types/units.h globals).
// reconciled: R04 0x006f1d20 int32_t network_predicted_state_flag -> game.h game_engine_definition *current_game_engine (all accesses are DWORD; non-NULL = multiplayer engine loaded)
// register convention: ECX -> object_index.
// blam-cc: ECX -> object_index
// FIXED (register inputs, objdump): this file had no blam-cc note at all, so ECX (read at
// 0x55e4a9, "mov eax,ecx") looked unclaimed even though the body already used object_index
// correctly. Added the missing note.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data;  // 0x008603b0
extern game_engine_definition *current_game_engine;  // 0x006f1d20, game.h; non-NULL = multiplayer engine loaded (R04)

extern void object_delete(uint32_t object_index); // 0x4f5bd0, UNSURE exact signature

// Detects when a unit has fallen far below the level (Z < -2000) while marked deleted-pending or
// outside any BSP cluster, and deletes it.
void unit_check_fell_off_level(uint32_t object_index)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;

    if (current_game_engine == 0 &&
        ((obj->flags & 0x200000) != 0 || obj->location_cluster_index == -1)) {
        if (obj->position.z < -2000.0f) {
            object_delete(object_index);
        }
    }
}

#if 0
Original Ghidra decompilation (0x55e4a0):

uint FUN_0055e4a0(void)

{
  float fVar1;
  uint uVar2;
  uint in_ECX;

  uVar2 = *(uint *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_ECX & 0xffff) * 0xc);
  if ((DAT_006f1d20 == 0) &&
     (((*(uint *)(uVar2 + 0x10) & 0x200000) != 0 || (*(short *)(uVar2 + 0x9c) == -1)))) {
    fVar1 = *(float *)(uVar2 + 100);
    uVar2 = CONCAT22((short)(uVar2 >> 0x10),
                     (ushort)(fVar1 < -2000.0) << 8 | (ushort)NAN(fVar1) << 10 |
                     (ushort)(fVar1 == -2000.0) << 0xe);
    if (fVar1 < -2000.0) {
      uVar2 = object_delete();
    }
  }
  return uVar2 & 0xffffff00;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
