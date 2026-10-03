// rasterizer_lens_flare_batch_apply_material  (Ghidra: FUN_00536b70)
// address 0x536b70, size 149 bytes
// name confidence: 0.4   rewrite confidence: 0.55
// evidence: functions.md summary ("State-caching helper that avoids redundant texture/material
//   state changes when drawing a batch of same-material screen-space sprites"); the four fields
//   compared/cached (0x007bf040..0x007bf04c) match lens_flare_applied_key exactly
//   (bitmap_tag_index, second_bitmap_tag_index, bitmap_index, shader_stage_config), matching the
//   in-flight key at ESI (lens_flare_current_key, 0x00746fb0, per types/rasterizer.h).
// register convention: ESI -> key (lens_flare_batch_key*).
// blam-cc: ESI -> key
// UNSURE: the low byte of the return value is `cVar1 == 0` (bind failure), and the high 3 bytes
//   are leftover register bits (uVar2's top bytes) the original never masks off; modeled here as
//   returning just the low byte's boolean, since nothing in this module is shown reading the rest.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern lens_flare_batch_key lens_flare_applied_key; // 0x007bf040

extern uint8_t rasterizer_validate_and_rebind_texture(uint32_t bitmap_tag_id, int16_t stage, int16_t frame); // 0x5187e0, EAX tag
extern uint8_t rasterizer_resolve_and_cache_submap_c(uint32_t bitmap_tag_id, int16_t bitmap_type, int16_t stage,
    int16_t default_index, int16_t frame); // 0x518a60, EAX tag, DI bitmap_type
extern void rasterizer_set_shader_stage_config(int16_t mode); // 0x519200

// Finds (or LRU-evicts and reassigns) a batching slot matching the current material key for the
// screen-space sprite rendering system by comparing it against the last-applied key and only
// re-binding the texture/shader-stage state that actually changed.
uint8_t rasterizer_lens_flare_batch_apply_material(lens_flare_batch_key *key)
{
    uint8_t ok = 1;

    if (lens_flare_applied_key.bitmap_tag_index != key->bitmap_tag_index ||
        lens_flare_applied_key.second_bitmap_tag_index != key->second_bitmap_tag_index ||
        lens_flare_applied_key.bitmap_index != key->bitmap_index) {
        uint8_t failed;
        if (key->second_bitmap_tag_index == -1) {
            // 0x536bb7: EAX = -1 (or eax,-1), push (uint16)key[+0], (uint16)key[+8]
            failed = rasterizer_validate_and_rebind_texture(0xffffffffu, (int16_t)(uint16_t)key->bitmap_tag_index,
                (int16_t)(uint16_t)key->bitmap_index);
        } else {
            // 0x536b92..0x536bac: EAX = key[+4], DI = 0, push (uint16)key[+0], 1, (uint16)key[+8]
            failed = rasterizer_resolve_and_cache_submap_c((uint32_t)key->second_bitmap_tag_index, 0,
                (int16_t)(uint16_t)key->bitmap_tag_index, 1, (int16_t)(uint16_t)key->bitmap_index);
        }
        ok = (failed == 0);
        lens_flare_applied_key.bitmap_tag_index = key->bitmap_tag_index;
        lens_flare_applied_key.second_bitmap_tag_index = key->second_bitmap_tag_index;
        lens_flare_applied_key.bitmap_index = key->bitmap_index;
    }

    if (lens_flare_applied_key.shader_stage_config != key->shader_stage_config) {
        rasterizer_set_shader_stage_config((int16_t)key->shader_stage_config);
        lens_flare_applied_key.shader_stage_config = key->shader_stage_config;
    }

    return ok;
}

#if 0
Original Ghidra decompilation (0x536b70):

undefined4 FUN_00536b70(void)

{
  char cVar1;
  uint uVar2;
  undefined2 extraout_var;
  int *unaff_ESI;
  bool bVar3;

  bVar3 = true;
  if (((DAT_007bf040 != *unaff_ESI) || (DAT_007bf044 != unaff_ESI[1])) ||
     (DAT_007bf048 != unaff_ESI[2])) {
    if (unaff_ESI[1] == -1) {
      cVar1 = FUN_005187e0((short)*unaff_ESI,(short)unaff_ESI[2]);
    }
    else {
      cVar1 = FUN_00518a60((short)*unaff_ESI,1,(short)unaff_ESI[2]);
    }
    bVar3 = cVar1 == '\0';
    DAT_007bf040 = *unaff_ESI;
    DAT_007bf044 = unaff_ESI[1];
    DAT_007bf048 = unaff_ESI[2];
  }
  uVar2 = (uint)*(ushort *)(unaff_ESI + 3);
  if (DAT_007bf04c != *(ushort *)(unaff_ESI + 3)) {
    rasterizer_set_shader_stage_config();
    DAT_007bf04c = *(ushort *)(unaff_ESI + 3);
    uVar2 = CONCAT22(extraout_var,DAT_007bf04c);
  }
  return CONCAT31((int3)(uVar2 >> 8),bVar3);
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
