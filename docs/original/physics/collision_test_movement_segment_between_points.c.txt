// collision_test_movement_segment_between_points  (Ghidra: FUN_00401a20; renamed for this
//   rewrite -- Ghidra decompiled this as a bare "call FUN_00505880(param_1); return;" one-liner,
//   which is wrong: it never split the real prologue/epilogue out. objdump shows a real 5-argument
//   wrapper around collision_test_movement_segment.)
// address 0x401a20, size 60 bytes
// name confidence: 0.55   rewrite confidence: 0.7
// evidence: objdump 0x401a20..0x401a5b. The body builds delta = target(ECX) - origin(EAX) into a
//   12-byte local (real_vector3d), then calls collision_test_movement_segment(flags, origin,
//   &delta, exclude_object_index, result) with flags/exclude_object_index/result taken from this
//   function's own incoming stack arguments 1..3 -- i.e. it is collision_test_movement_segment
//   with the delta vector computed from two endpoints instead of passed directly, exactly
//   mirroring how observer_collision_test_ray (0x449170, src/camera) wraps the same callee from
//   an origin/target pair, except this one forwards flags/exclude/result instead of hardcoding
//   them. src/items/item_update.c already calls this address (as `FUN_00401a20`) with the
//   collision mask, `ignore_object_index` and an output record, treating the EAX/ECX point
//   arguments as opaque/unreconstructed register state at that call site (its own header
//   documents that tradeoff); this file is the address's own definition, independent of how any
//   one caller's decompile chose to show it. Placed in `physics` (not `unknown`) because its
//   whole body, and its callee, are the module's own collision_result-producing entry point.
// register convention: origin point in EAX, target point in ECX, no other registers read;
//   flags / exclude_object_index / result are ordinary incoming stack arguments (confirmed by
//   the `[esp+0x10]`, `[esp+0x14]`, `[esp+0x18]` reads relative to the post-prologue frame,
//   i.e. the function's own 1st/2nd/3rd stack parameters).
//   // blam-cc: EAX -> origin, ECX -> target, stack -> flags, exclude_object_index, result
// UNSURE: name is inferred from shape (delta-from-two-points wrapper around
//   collision_test_movement_segment), not from any string or vtable; no *_types_notes.md entry
//   covers this address directly.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "projectiles.h"
#include "physics.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint8_t collision_test_movement_segment(uint32_t flags, real_point3d *origin,
    real_vector3d *delta, uint32_t exclude_object_index, collision_result *result); // 0x505880, this module

// Sweeps a movement segment between two explicit endpoints: builds delta = target - origin and
// forwards to collision_test_movement_segment, which does the actual BSP/object collision test.
uint8_t collision_test_movement_segment_between_points(real_point3d *origin, real_point3d *target,
    uint32_t flags, uint32_t exclude_object_index, collision_result *result)
{
    real_vector3d delta;

    delta.i = target->x - origin->x;
    delta.j = target->y - origin->y;
    delta.k = target->z - origin->z;

    return collision_test_movement_segment(flags, origin, &delta, exclude_object_index, result);
}

#if 0
Original Ghidra decompilation (0x401a20) -- wrong; Ghidra folded this address into a bare
one-argument tail call of its callee instead of recognizing its own prologue:

void FUN_00401a20(undefined4 param_1)

{
  collision_test_movement_segment(param_1);
  return;
}

-- The real code, from objdump -d -M intel --start-address=0x401a20 --stop-address=0x401a5b
-- bin/halo.exe:

  401a20:	83 ec 0c             	sub    esp,0xc
  401a23:	d9 01                	fld    DWORD PTR [ecx]
  401a25:	8b 54 24 14          	mov    edx,DWORD PTR [esp+0x14]
  401a29:	d8 20                	fsub   DWORD PTR [eax]
  401a2b:	d9 1c 24             	fstp   DWORD PTR [esp]
  401a2e:	d9 41 04             	fld    DWORD PTR [ecx+0x4]
  401a31:	d8 60 04             	fsub   DWORD PTR [eax+0x4]
  401a34:	d9 5c 24 04          	fstp   DWORD PTR [esp+0x4]
  401a38:	d9 41 08             	fld    DWORD PTR [ecx+0x8]
  401a3b:	8b 4c 24 18          	mov    ecx,DWORD PTR [esp+0x18]
  401a3f:	d8 60 08             	fsub   DWORD PTR [eax+0x8]
  401a42:	51                   	push   ecx
  401a43:	52                   	push   edx
  401a44:	8b 54 24 18          	mov    edx,DWORD PTR [esp+0x18]
  401a48:	8d 4c 24 08          	lea    ecx,[esp+0x8]
  401a4c:	d9 5c 24 10          	fstp   DWORD PTR [esp+0x10]
  401a50:	51                   	push   ecx
  401a51:	50                   	push   eax
  401a52:	52                   	push   edx
  401a53:	e8 28 3e 10 00       	call   0x505880
  401a58:	83 c4 20             	add    esp,0x20
  401a5b:	c3                   	ret
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
