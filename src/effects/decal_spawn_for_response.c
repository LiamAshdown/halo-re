// decal_spawn_for_response  (Ghidra: FUN_0044ece0; named per its own summary in
// out/phase4/effects_functions.md: "Decides whether a decal should be spawned for the current
// collision/damage response ... and, if so, invokes the decal placement algorithm")
// address 0x44ece0, size 209 bytes
// name confidence: 0.4   rewrite confidence: 0.85
// REWRITTEN from objdump 0x44ece0..0x44edb0 (the draft had no origin/direction/radius and called the
//   segment test and decal_place without them).
// blam-cc: ESI -> response_tag_index, BL -> deterministic, ECX -> origin, stack -> direction, radius,
//   marker_index (effect_event_apply 0x452e95 pushes a fourth, unread 0)
// Spawns the decal of a response: allowed when decals are on for every response (0x006893f5), or for a
//   deterministic spawn of a response whose tag +0x04 is 3; with decals enabled (0x00687004) the effect
//   seed is (for a deterministic spawn) replaced by the origin words xor 0xdeadc0de and restored after;
//   the segment origin..origin+direction is tested (collision_test_movement_segment 0x100061, -1), and a
//   structure hit (type 2) on a tag without flag 0x10 places the decal (decal_place: tag, the result,
//   direction, radius, deterministic, marker_index).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "effects.h"
#include "projectiles.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint8_t decals_enabled;             // 0x00687004
extern uint8_t decals_for_all_responses;   // 0x006893f5
extern tag_instance *tag_instances;        // 0x0087bc14
extern random_seed effect_random_seed;     // 0x00719cd4

extern uint8_t collision_test_movement_segment(uint32_t flags, real_point3d *origin, real_vector3d *delta,
    uint32_t exclude_object_index, collision_result *result); // 0x505880
extern void decal_place(datum_index decal_tag_index, collision_result *placement, real_vector3d *direction,
    real radius_scale, uint8_t object_attached, int16_t sequence_index); // 0x44edc0

void decal_spawn_for_response(datum_index response_tag_index, uint8_t deterministic, real_point3d *origin,
    real_vector3d *direction, real radius, int32_t marker_index)
{
    uint8_t allowed = 1;
    random_seed saved_seed = 0;
    collision_result result;

    if (decals_for_all_responses == 0 &&
        (deterministic != 1 ||
            *(int16_t *)((uint8_t *)tag_instances[response_tag_index & 0xffff].data + 4) != 3)) {
        allowed = 0;
    }
    if (decals_enabled == 0 || !allowed) {
        return;
    }
    if (deterministic != 0) {
        uint32_t *words = (uint32_t *)origin;

        saved_seed = effect_random_seed;
        effect_random_seed = words[2] ^ words[1] ^ words[0] ^ 0xdeadc0de;
    }
    if (collision_test_movement_segment(0x100061, origin, direction, 0xffffffff, &result) &&
        result.type == 2 &&
        (*(uint8_t *)tag_instances[response_tag_index & 0xffff].data & 0x10) == 0) {
        decal_place(response_tag_index, &result, direction, radius, deterministic, (int16_t)marker_index);
    }
    if (deterministic != 0) {
        effect_random_seed = saved_seed;
    }
}

#if 0
Original Ghidra decompilation (0x44ece0):

void FUN_0044ece0(void)

{
  bool bVar1;
  int iVar2;
  char cVar3;
  uint *in_ECX;
  char unaff_BL;
  uint unaff_ESI;
  uint uVar4;
  short local_54;

  iVar2 = DAT_0087bc14;
  bVar1 = true;
  if ((DAT_006893f5 == '\0') &&
     ((unaff_BL != '\x01' ||
      (*(short *)(*(int *)((unaff_ESI & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 4) != 3)))) {
    bVar1 = false;
  }
  if ((DAT_00687004 != '\0') && (bVar1)) {
    uVar4 = 0;
    if (unaff_BL != '\0') {
      uVar4 = DAT_00719cd4;
      DAT_00719cd4 = in_ECX[2] ^ in_ECX[1] ^ *in_ECX ^ 0xdeadc0de;
    }
    cVar3 = FUN_00505880(0x100061);
    if (((cVar3 != '\0') && (local_54 == 2)) &&
       ((**(byte **)((unaff_ESI & 0xffff) * 0x20 + 0x14 + iVar2) & 0x10) == 0)) {
      FUN_0044edc0();
    }
    if (unaff_BL != '\0') {
      DAT_00719cd4 = uVar4;
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
