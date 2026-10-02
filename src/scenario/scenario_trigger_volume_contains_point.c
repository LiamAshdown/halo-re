// scenario_trigger_volume_contains_point  (Ghidra: already named
// scenario_trigger_volume_contains_point)
// address 0x53f020, size 296 bytes
// name confidence: 0.6   rewrite confidence: 0.8
// evidence: types/scenario.h's own writeup of scenario_trigger_volume_box (the union overlay of
//   ScenarioTriggerVolume's tail, +0x48..+0x5f) and out/phase4/scenario_types_notes.md's
//   register table ("0x53f020: AX = trigger volume index, ECX = point. Returns a bool."), both
//   citing this exact function. Confirmed instruction by instruction against objdump -d -M intel
//   --start-address=0x53f020 --stop-address=0x53f150 bin/halo.exe: the rotational branch builds
//   its frame with matrix4x3_from_forward_up (up in EAX, forward in ECX, out on stack -- the
//   same convention src/camera/observer_compute_remaining_offset.c establishes), patches the
//   frame's position to the volume's starting_corner by hand (from_forward_up does not set it),
//   then calls matrix4x3_inverse_transform_point (m in ECX, point in ESI, out in EDX -- read
//   directly from its own disassembly at 0x4cbf80, not previously documented in types/math.h).
//   The trailing upper-z-bound check is the same 4 bytes (+0x5c) in both branches, which is why
//   both branches below fall into one shared `box->fixed.z_bounds[1]` comparison at the end.
// register convention: AX -> trigger_volume_index (int16_t); ECX -> point (real_point3d *); no
//   stack parameters.
//   // blam-cc: EAX -> trigger_volume_index, ECX -> point
// UNSURE: none left in this function's own body.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "scenario.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern Scenario *global_scenario; // 0x00746f8c

// blam-cc: up in EAX, forward in ECX, out on stack
extern void matrix4x3_from_forward_up(real_vector3d *up, real_vector3d *forward,
    real_matrix4x3 *out); // math module, 0x4cb970

// blam-cc: m in ECX, out in EDX, point in ESI (parameter order as src/math defines it)
extern void matrix4x3_inverse_transform_point(real_matrix4x3 *m, real_point3d *out,
    real_point3d *point); // math module, 0x4cbf80

// blam-cc: EAX -> trigger_volume_index, ECX -> point
// Tests whether `point` lies inside a scenario trigger volume. Type 0 (fixed) reads the volume's
// starting_corner/ending_corner_offset tail as three independent (lower, upper) axis-aligned
// bounds pairs and tests point directly against them. Type 1 (rotational) builds a frame from
// the volume's forward/up vectors and starting_corner, inverse-transforms point into it, and
// requires the result to fall strictly between the origin and ending_corner_offset (used as
// extents) on all three axes. Any other type returns false. Every comparison is written as a
// negated strict test because the x87 tests (test ah,0x41 / jne for the lower bound, test ah,5
// / jp for the upper) also reject an unordered (NaN) coordinate. The result is a bool in AL:
// EAX is a clean 0/1 on every path except the unknown-type exit, which clears only AL.
uint8_t scenario_trigger_volume_contains_point(int16_t trigger_volume_index, real_point3d *point)
{
    ScenarioTriggerVolume *volume;
    scenario_trigger_volume_box *box;
    float local_z;

    volume = &((ScenarioTriggerVolume *)global_scenario->trigger_volumes.pointer)
                 [trigger_volume_index];
    box = (scenario_trigger_volume_box *)&volume->starting_corner;

    if (volume->type == scenariotriggervolumetype_fixed) {
        if (!(box->fixed.x_bounds[0] < point->x)) return 0;
        if (!(box->fixed.y_bounds[0] < point->y)) return 0;
        if (!(box->fixed.z_bounds[0] < point->z)) return 0;
        if (!(point->x < box->fixed.x_bounds[1])) return 0;
        if (!(point->y < box->fixed.y_bounds[1])) return 0;
        local_z = point->z;
    } else if (volume->type == scenariotriggervolumetype_rotational) {
        real_matrix4x3 matrix;
        real_point3d local;

        matrix4x3_from_forward_up((real_vector3d *)&volume->rotation_vector_up,
            (real_vector3d *)&volume->rotation_vector_forward, &matrix);
        // from_forward_up leaves position unset; the frame's origin is the volume's own corner.
        matrix.position.x = volume->starting_corner.x;
        matrix.position.y = volume->starting_corner.y;
        matrix.position.z = volume->starting_corner.z;

        matrix4x3_inverse_transform_point(&matrix, &local, point);

        if (!(0.0f < local.x)) return 0;
        if (!(0.0f < local.y)) return 0;
        if (!(0.0f < local.z)) return 0;
        if (!(local.x < box->rotational.extents.i)) return 0;
        if (!(local.y < box->rotational.extents.j)) return 0;
        local_z = local.z;
    } else {
        return 0;
    }

    if (!(local_z < box->fixed.z_bounds[1])) { // == box->rotational.extents.k, same 4 bytes
        return 0;
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x53f020):

uint scenario_trigger_volume_contains_point(void)

{
  short in_AX;
  int iVar1;
  uint uVar2;
  float *in_ECX;
  int iVar3;
  float local_40;
  float local_3c;
  float local_38;
  undefined1 local_34 [40];
  undefined4 local_c;
  undefined4 local_8;
  undefined4 local_4;

  iVar1 = (int)*(short *)(in_AX * 0x60 + *(int *)(global_scenario + 0x364));
  iVar3 = in_AX * 0x60 + *(int *)(global_scenario + 0x364);
  if (iVar1 == 0) {
    if (*in_ECX <= *(float *)(iVar3 + 0x48)) {
      return 0;
    }
    if (in_ECX[1] <= *(float *)(iVar3 + 0x50)) {
      return 0;
    }
    if (in_ECX[2] <= *(float *)(iVar3 + 0x58)) {
      return 0;
    }
    if (*(float *)(iVar3 + 0x4c) <= *in_ECX) {
      return 0;
    }
    if (*(float *)(iVar3 + 0x54) <= in_ECX[1]) {
      return 0;
    }
    local_38 = in_ECX[2];
  }
  else {
    uVar2 = iVar1 - 1;
    if (uVar2 != 0) {
      return uVar2 & 0xffffff00;
    }
    matrix4x3_from_forward_up(local_34);
    local_c = *(undefined4 *)(iVar3 + 0x48);
    local_8 = *(undefined4 *)(iVar3 + 0x4c);
    local_4 = *(undefined4 *)(iVar3 + 0x50);
    matrix4x3_inverse_transform_point();
    if (local_40 <= 0.0) {
      return 0;
    }
    if (local_3c <= 0.0) {
      return 0;
    }
    if (local_38 <= 0.0) {
      return 0;
    }
    if (*(float *)(iVar3 + 0x54) <= local_40) {
      return 0;
    }
    if (*(float *)(iVar3 + 0x58) <= local_3c) {
      return 0;
    }
  }
  if (*(float *)(iVar3 + 0x5c) <= local_38) {
    return 0;
  }
  return 1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
