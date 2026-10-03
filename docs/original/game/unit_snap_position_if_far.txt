// unit_snap_position_if_far  (Ghidra: unit_snap_position_if_far, already named)
// address 0x4772e0, size 99 bytes
// name confidence: 0.5   rewrite confidence: 0.4
// evidence: out/phase4/game_functions.md ("Detects when a unit has moved farther than a small
//   threshold and, if so, snaps its cached position and marks it dirty for network
//   reconciliation"); types/objects.h object (position +0x5c, velocity +0x68,
//   _object_at_rest_bit 0x20).
// register convention: EAX -> new_position, ECX -> obj.
//   // blam-cc: EAX -> new_position, ECX -> obj
//
// UNSURE: the trailing call to object_set_position_and_recalculate is shown by Ghidra with zero
// visible arguments; the established signature elsewhere in this codebase takes a datum_index,
// but this function only has an object pointer in scope, so the real argument(s) are not
// recoverable here without more context than this single function provides.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "game.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern real_vector3d global_origin3d; // 0x0065c230

extern double sqrt(double x); // x87 FSQRT
extern void object_set_position_and_recalculate(void *position_or_object, uint32_t unknown); // 0x4f52c0, UNSURE args here

// If `obj`'s cached position differs from new_position by more than 1.1 world units, snaps its
// velocity to zero and marks it at-rest (_object_at_rest_bit) so downstream physics doesn't try
// to interpolate the jump. Then always calls object_set_position_and_recalculate.
void unit_snap_position_if_far(real_point3d *new_position, object *obj)
    // blam-cc: EAX -> new_position, ECX -> obj
{
    float dx = new_position->x - obj->position.x;
    float dy = new_position->y - obj->position.y;
    float dz = new_position->z - obj->position.z;

    if (sqrt(dy * dy + dz * dz + dx * dx) > 1.1) {
        obj->velocity = global_origin3d;
        obj->flags = obj->flags | _object_at_rest_bit;
    }
    object_set_position_and_recalculate(obj, 0); // UNSURE: see header
}

#if 0
Original Ghidra decompilation (0x4772e0), from tools/pack.py 0x4772e0:

void unit_snap_position_if_far(void)

{
  float fVar1;
  float fVar2;
  float fVar3;
  undefined *puVar4;
  float *in_EAX;
  int in_ECX;

  puVar4 = PTR_DAT_00696714;
  fVar1 = *in_EAX - *(float *)(in_ECX + 0x5c);
  fVar2 = in_EAX[1] - *(float *)(in_ECX + 0x60);
  fVar3 = in_EAX[2] - *(float *)(in_ECX + 100);
  if (1.1 < SQRT(fVar2 * fVar2 + fVar3 * fVar3 + fVar1 * fVar1)) {
    *(undefined4 *)(in_ECX + 0x68) = *(undefined4 *)PTR_DAT_00696714;
    *(undefined4 *)(in_ECX + 0x6c) = *(undefined4 *)(puVar4 + 4);
    *(undefined4 *)(in_ECX + 0x70) = *(undefined4 *)(puVar4 + 8);
    *(uint *)(in_ECX + 0x10) = *(uint *)(in_ECX + 0x10) | 0x20;
  }
  object_set_position_and_recalculate();
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
