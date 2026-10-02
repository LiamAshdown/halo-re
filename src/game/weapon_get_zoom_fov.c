// weapon_get_zoom_fov  (Ghidra: FUN_0046fe10; renamed, no established name)
// address 0x46fe10, size 85 bytes
// name confidence: 0.35   rewrite confidence: 0.9 (VERIFIED against objdump 0x46fe10..0x46fe64 (difficulty table row lookup))
// evidence: out/phase4/game_functions.md ("Looks up the field-of-view value for a given
// weapon/zoom-table index and magnification level, defaulting to 1.0 if no zoom data is
// present"); types/game.h Globals *global_globals (0x00746fa0); src/math's established use of
// the shared float constant at 0x00672ac4 as a literal 1.0f (see e.g.
// src/math/vector3d_rotate_toward_with_acceleration.c).
// register convention: objdump 0x46fe10 shows no prologue push/sub, so `zoom_table_index` is a
// plain __cdecl stack argument (`mov edx,[esp+0x4]`, i.e. the caller's own pushed dword measured
// from entry esp), and `magnification` arrives in CX (Ghidra's `in_CX`), never spilled to the
// stack by this function itself.
//   // blam-cc: CX -> magnification, stack -> zoom_table_index
// UNSURE: the two globals_tag offsets (+0x11c count, +0x120 pointer) read off global_globals are
// modeled as a raw TagReflexive-shaped pair rather than a named field, because types/tags.h's own
// computed offsets for Globals (from the invader tag definitions) disagree with this module's own
// empirically pinned offsets for player_information/multiplayer_information (types/game.h notes
// the same conflict); which reflexive this retail build actually keeps at +0x11c is therefore not
// pinned here.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern Globals *global_globals; // 0x00746fa0

// blam-cc: CX -> magnification, stack -> zoom_table_index
// Returns the field-of-view multiplier for zoom level `magnification` (clamped to 0..3, or, for
// a negative magnification, level 0) of the zoom table at row `zoom_table_index`, or 1.0 if the
// globals tag or its zoom table is not loaded.
real weapon_get_zoom_fov(int16_t zoom_table_index, int16_t magnification)
{
    TagReflexive *zoom_table_reflexive; // UNSURE: see header -- raw offset +0x11c/+0x120, not a
                                         // named Globals field
    real *rows;

    if (global_globals == 0) {
        return 1.0f;
    }
    zoom_table_reflexive = (TagReflexive *)((uint8_t *)global_globals + 0x11c);
    if (zoom_table_reflexive->count == 0) {
        return 1.0f;
    }
    rows = (real *)(uintptr_t)zoom_table_reflexive->pointer;
    if (rows == 0) {
        return 1.0f;
    }

    if (magnification < 0) {
        return rows[(int32_t)zoom_table_index * 4];
    }
    if (magnification > 3) {
        magnification = 3;
    }
    return rows[(int32_t)zoom_table_index * 4 + magnification];
}

#if 0
Original Ghidra decompilation (0x46fe10), from tools/pack.py 0x46fe10:

float10 FUN_0046fe10(short param_1)

{
  int iVar1;
  short in_CX;
  float10 fVar2;

  fVar2 = (float10)1.0;
  if (((DAT_00746fa0 != 0) && (*(int *)(DAT_00746fa0 + 0x11c) != 0)) &&
     (iVar1 = *(int *)(DAT_00746fa0 + 0x120), iVar1 != 0)) {
    if (in_CX < 0) {
      return (float10)*(float *)(iVar1 + param_1 * 0x10);
    }
    if (3 < in_CX) {
      in_CX = 3;
    }
    fVar2 = (float10)*(float *)(iVar1 + ((int)in_CX + param_1 * 4) * 4);
  }
  return fVar2;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
