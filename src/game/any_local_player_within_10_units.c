// any_local_player_within_10_units  (orphan pass 4: FUN_00453330, no Ghidra name)
// address 0x453330, size 125 bytes
// name confidence: 0.4 (out/phase4/effects_types_notes.md: "'is any local player within 10
//   world units'; reads the player globals at 0x0087a478 and the camera positions at
//   0x006ac6d0")
// rewrite confidence: 0.85 (VERIFIED 2026-09-28 against objdump 0x453330..0x4533ac.) (control flow and the squared-distance test are confirmed against
//   the decompilation; the player_globals and camera-position array field offsets are not
//   independently retyped here -- see UNSURE)
// evidence: out/phase4/effects_types_notes.md "0x453330 | players | ...". No "players" module
//   exists in src/ yet, so this is filed under src/game alongside its other player/camera
//   helpers (camera_observer_*, cheat_*), per that note's own suggestion to relocate it "when
//   that module is written".
// register convention: EDX = const real_point3d *query_point (in_EDX). The function takes no
//   other visible parameter; "10 units" (100 squared) is a literal, not an argument.
// blam-cc: any_local_player_within_10_units(const real_point3d *query_point /*EDX*/), returns a
//   bool in AL (0x4533a4 `mov al,bl` / 0x4533a9 `mov al,1`; the orphan pass 4 review narrowed the
//   return type from uint32_t). The 0x006ac6d0 table is observers[i].camera.position
//   (types/camera.h observer_camera, 0x29c-byte observer stride), the same record
//   sound_environment_update and structure_regions_find_within_radius read.
// UNSURE: player_globals (0x0087a478) and the camera-position table (0x006ac6d0, stride
//   0xa7 floats = 0x29c bytes per player) are treated as raw byte offsets, not named/typed
//   structs -- this pass does not establish a "players" module. The loop bound (`sVar2 < 1`)
//   means only player slot 0 is ever tested, which is preserved exactly rather than assumed to
//   be a bug.

#include "tags.h"
#include "math.h"
#include "memory.h"
#include "objects.h"
#include "units.h"
#include "game.h"

extern player_globals *local_player_globals; // 0x0087a478
extern float camera_point[]; // 0x006ac6d0, stride 0x29c bytes (0xa7 floats) per player slot
extern float camera_position_y_table[]; // 0x006ac6d4, same stride
extern float camera_position_z_table[]; // 0x006ac6d8, same stride

uint8_t any_local_player_within_10_units(const real_point3d *query_point) // blam-cc: EDX query_point; result in AL
{
    int16_t local_player_count = local_player_globals->local_player_count;
    int16_t slot;

    if (local_player_count > 2) {
        return 1;
    }

    for (slot = 0; slot < 1; slot++) {
        if (slot != -1 && slot < 1 && (int32_t)local_player_globals->local_players[slot] != -1) {
            float dx = query_point->x - camera_point[slot * 0xa7];
            float dy = query_point->y - camera_position_y_table[slot * 0xa7];
            float dz = query_point->z - camera_position_z_table[slot * 0xa7];
            if (dx * dx + dz * dz + dy * dy < 100.0f) { // x87 order 0x453379..0x453387; `jnp`: NaN is not < 100
                return 1;
            }
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x453330):

undefined4 FUN_00453330(void)

{
  int iVar1;
  short sVar2;
  float *in_EDX;

  if (2 < *(short *)(DAT_0087a478 + 0xc)) {
    return 1;
  }
  sVar2 = 0;
  do {
    if (((sVar2 != -1) && (sVar2 < 1)) &&
       (iVar1 = (int)sVar2, *(int *)(DAT_0087a478 + 4 + iVar1 * 4) != -1)) {
      if ((in_EDX[1] - (float)(&DAT_006ac6d4)[iVar1 * 0xa7]) *
          (in_EDX[1] - (float)(&DAT_006ac6d4)[iVar1 * 0xa7]) +
          (in_EDX[2] - (float)(&DAT_006ac6d8)[iVar1 * 0xa7]) *
          (in_EDX[2] - (float)(&DAT_006ac6d8)[iVar1 * 0xa7]) +
          (*in_EDX - (float)(&DAT_006ac6d0)[iVar1 * 0xa7]) *
          (*in_EDX - (float)(&DAT_006ac6d0)[iVar1 * 0xa7]) < 100.0) {
        return 1;
      }
    }
    sVar2 = sVar2 + 1;
  } while (sVar2 < 1);
  return 0;
}
#endif
