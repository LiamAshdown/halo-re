// effect_compute_spawn_basis  (Ghidra: FUN_00451930, still unnamed there; named from its own
// summary in out/phase4/effects_functions.md: "Resolves a spawn point (from a fixed table or a
// transform) and builds an orthonormal basis around it for particle system placement")
// address 0x451930, size 251 bytes
// name confidence: 0.3   rewrite confidence: 0.1 (VERY LOW -- almost every operand below is
// register-elided in the original decompile, with zero callers in this batch to cross-check
// against)
// evidence: callee names only (matrix4x3_inverse_transform_point, vector3d_build_perpendicular,
// vector3d_normalize_with_length, matrix4x3_from_forward_up); no field-level evidence survives.
// register convention: UNRECOVERABLE from this pack -- every callee is invoked with zero visible
// arguments, meaning all of them are register-passed and none of those registers are named by
// Ghidra (not even as in_/unaff_ locals for most of them). The parameter list and body below are
// a structural best guess preserving the visible branch shape and call order only; do not trust
// the specific types or field offsets.
// UNSURE: everything. This function needs re-disassembly (objdump -d) before it can be trusted;
// flagged here rather than fabricated with false confidence.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "effects.h"

extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, math module; vector in ECX
extern void matrix4x3_from_forward_up(real_matrix4x3 *out, real_vector3d *forward,
    real_vector3d *up); // 0x4cb970, UNSURE signature/registers
extern void matrix4x3_inverse_transform_point(real_matrix4x3 *m, real_point3d *point,
    real_point3d *out); // 0x4cbf80, UNSURE signature/registers
extern void vector3d_build_perpendicular(real_vector3d *v, real_vector3d *out); // 0x4cd670,
    // UNSURE signature/registers

// UNSURE (see file header): reconstructs a spawn basis either from a fixed table entry (when the
// marker's transform is the identity, i.e. `*(int *)(source + 8) == 0`) or by inverse-transforming
// a point through it, then builds a perpendicular/forward-up basis around the result.
void effect_compute_spawn_basis(object_marker *out_marker, object_marker *source,
    int16_t table_index, real_point3d *spawn_point_table)
{
    real_point3d point;

    out_marker->node_index = source->node_index;

    if (*(int32_t *)&source->transform.scale == 0) {
        point = spawn_point_table[table_index];
    } else {
        matrix4x3_inverse_transform_point(&source->transform, &spawn_point_table[table_index], &point);
    }

    {
        real_vector3d perpendicular;
        real_vector3d forward;

        forward.i = point.x;
        forward.j = point.y;
        forward.k = point.z;

        vector3d_build_perpendicular(&forward, &perpendicular);
        vector3d_normalize_with_length(&perpendicular);
        matrix4x3_from_forward_up(&out_marker->transform, &forward, &perpendicular);

        out_marker->transform.position.x = point.x;
        out_marker->transform.position.y = point.y;
        out_marker->transform.position.z = point.z;
    }
}

#if 0
Original Ghidra decompilation (0x451930):

void FUN_00451930(undefined2 *param_1)

{
  short in_AX;
  undefined4 *puVar1;
  undefined2 *unaff_EBX;
  undefined4 local_18;
  undefined4 local_14;
  undefined4 local_10;

  *param_1 = *unaff_EBX;
  if (*(int *)(unaff_EBX + 2) == 0) {
    puVar1 = (undefined4 *)(*(int *)(unaff_EBX + 8) + in_AX * 0xc);
    local_18 = *puVar1;
    local_14 = puVar1[1];
    local_10 = puVar1[2];
  }
  else {
    matrix4x3_inverse_transform_point();
  }
  vector3d_build_perpendicular();
  vector3d_normalize_with_length();
  matrix4x3_from_forward_up(param_1 + 2);
  *(undefined4 *)(param_1 + 0x16) = local_18;
  *(undefined4 *)(param_1 + 0x18) = local_14;
  *(undefined4 *)(param_1 + 0x1a) = local_10;
  return;
}
#endif
