// observer_collision_test_ray  (Ghidra: FUN_00449170; renamed for this rewrite)
// address 0x449170, size 98 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: out/phase4/camera_types_notes.md's proposed name ("single ray"); confirmed against
// objdump: a thin wrapper around collision_test_movement_segment (0x505880; flags, origin,
// delta, exclude object, collision_result *), building delta = target - origin, excluding no
// object (-1), and on a hit copying collision_result.t (+0x14) to the caller.
// register convention (objdump 0x449176 test dl,dl; 0x449184 fld [esi]; 0x44918a fsub [eax];
// 0x4491be mov ecx,[esp+0x64]): origin in EAX, target in ESI, the mask selector in DL, the
// output fraction pointer on the stack. Result in AL.
// review fix (phase 4 gate): the result buffer is the 0x50 byte collision_result
// (types/projectiles.h; this frame reserves exactly 0x50 bytes at esp+0x10). The earlier 0x18
// byte scratch array was overrun by the callee. TYPES-GAP closed: no local type is needed.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "projectiles.h"
#include "camera.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint8_t collision_test_movement_segment(uint32_t flags, real_point3d *origin,
    real_vector3d *delta, uint32_t exclude_object_index, collision_result *result); // 0x505880, physics module

// blam-cc: EAX -> origin, DL -> use_alternate_mask, ESI -> target; stack -> out_fraction
// Casts a single ray from origin to target (flags 0x40a1 when use_alternate_mask is set, else
// 0x40e1) and, if it hits anything, writes the hit fraction to *out_fraction and returns 1.
uint8_t observer_collision_test_ray(real_point3d *origin, uint8_t use_alternate_mask,
    real_point3d *target, float *out_fraction)
{
    uint32_t flags = use_alternate_mask ? 0x40a1 : 0x40e1;
    real_vector3d delta;
    collision_result collision;

    delta.i = target->x - origin->x;
    delta.j = target->y - origin->y;
    delta.k = target->z - origin->z;

    if (collision_test_movement_segment(flags, origin, &delta, 0xffffffff, &collision) != 0) {
        *out_fraction = collision.t;
        return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x449170):

undefined4 FUN_00449170(undefined4 *param_1)

{
  char cVar1;
  undefined4 uVar2;
  char in_DL;
  undefined4 local_3c;

  uVar2 = 0x40e1;
  if (in_DL != '\0') {
    uVar2 = 0x40a1;
  }
  cVar1 = FUN_00505880(uVar2);
  if (cVar1 != '\0') {
    *param_1 = local_3c;
    return 1;
  }
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
