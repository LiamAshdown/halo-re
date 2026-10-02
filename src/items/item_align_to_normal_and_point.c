// item_align_to_normal_and_point  (Ghidra: item_align_to_normal_and_point, already named via
// cea-pdb hint on the shared "ground point" string)
// address 0x4bd5d0, size 364 bytes
// name confidence: 0.7   rewrite confidence: 0.55
// evidence: types/objects.h object_marker (node_index 0x00, transform 0x04, node_transform
//   0x38), object.position (0x05c); types/math.h real_matrix4x3 (scale/forward/left/up/position
//   layout), real_quaternion (i/j/k/w layout); callees object_get_node_local_transform,
//   vector3d_cross_product, vector3d_normalize_with_length, matrix4x3_from_forward_up (all
//   already named/typed elsewhere in this codebase), quaternion_rotate_vector (typed below from
//   its own body, one caller: this function).
// register convention: output real_point3d * in EAX (in_EAX); item index in ECX (in_ECX);
//   normal vector pointer and reference-point pointer as the two stack parameters (Ghidra's own
//   `item_align_to_normal_and_point(float *param_1, undefined4 *param_2)`).
//   // blam-cc: EAX -> out_position, ECX -> item_index, stack -> normal, point
// resolved from disassembly (objdump -d -M intel bin/halo.exe, 0x4bd5d0..0x4bd73b), because
// Ghidra's own decompile elides every vector3d_cross_product / vector3d_normalize_with_length /
// quaternion_rotate_vector argument as unrecognized registers, and mis-slices the marker's
// node_transform.up vector across three separately-named locals (local_80/local_78/local_7c) in
// an order that does not match the actual FLD sequence. Tracing the FPU instructions directly
// gives a standard "shortest-arc quaternion" construction (Stan Melax's formulation: axis =
// up x normal, s = sqrt(2*(1+dot(up,normal))), q = (axis/s, s/2)), with a degenerate fallback
// for s <= 0.01 that projects the marker's forward onto the plane perpendicular to normal via
// two chained cross products instead. The float constants at 0x00672ac4 / 0x00672abc /
// 0x00672dd0 read from the binary as 1.0f / 0.5f / 0.01 (double), confirming the formula.
// register convention for quaternion_rotate_vector (0x4cdd40, math module, this rewrite's only
// caller): quaternion on the stack, vector to rotate in ECX, output vector in EDX; body
// confirmed against its own decompile as the standard quaternion-rotate-vector formula.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "items.h"

extern data_array *object_data; // 0x008603b0

extern int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name,
    object_marker *marker, uint32_t flags); // 0x4f6080
extern void vector3d_cross_product(real_vector3d *out, real_vector3d *ecx_operand, real_vector3d *stack_operand); // 0x4052c0, out = stack_operand x ecx_operand
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, vector in ECX
extern void matrix4x3_from_forward_up(real_vector3d *up, real_vector3d *forward, real_matrix4x3 *out); // 0x4cb970, up in EAX, forward in ECX
extern void quaternion_rotate_vector(real_quaternion *q, real_vector3d *v, real_vector3d *out); // 0x4cdd40, q on the stack, v in ECX, out in EDX
extern void object_recompute_basis_from_marker_delta(object *obj, object_marker *marker,
    real_matrix4x3 *output_matrix); // 0x4f62f0, obj in EAX
extern double sqrt(double x); // a single x87 FSQRT instruction

// Builds a rotation that tilts an item's "ground point" marker basis so its up vector matches
// `normal`, positions the result at `point` (defaulting to the marker's own node-space
// position), applies it to the item's basis, and returns the item's resulting world position.
void item_align_to_normal_and_point(real_point3d *out_position, uint32_t item_index,
    real_vector3d *normal, real_point3d *point)
    // blam-cc: EAX -> out_position, ECX -> item_index, stack -> normal, point
{
    object *obj = ((object_header *)object_data->data)[item_index & 0xffff].data;
    object_marker marker;
    real_vector3d rotated_forward;
    real_matrix4x3 basis;
    real_vector3d *up;
    real_vector3d *forward;
    real dot;
    real s;
    real_point3d discard; // both NULL fallbacks below point at marker.node_transform.position
                          // in the original; out_position's is only ever read back from there
                          // when the caller passed NULL, so a scratch local is equivalent

    if (object_get_node_local_transform(item_index, (char *)"ground point", &marker, 1) == 0) {
        return;
    }

    if (point == 0) {
        point = &marker.node_transform.position;
    }
    if (out_position == 0) {
        out_position = &discard;
    }

    up = &marker.node_transform.up;
    forward = &marker.node_transform.forward;
    dot = up->i * normal->i + up->j * normal->j + up->k * normal->k;
    s = (real)sqrt((double)(2.0f * (dot + 1.0f)));

    if (s <= 0.01f) {
        real_vector3d cross1;
        vector3d_cross_product(&cross1, forward, normal);         // cross1 = normal x forward
        vector3d_cross_product(&rotated_forward, normal, &cross1); // rotated_forward = cross1 x normal
        vector3d_normalize_with_length(&rotated_forward);
    } else {
        real_quaternion q;
        real inverse_s = 1.0f / s;
        real_vector3d axis;
        vector3d_cross_product(&axis, normal, up); // axis = up x normal
        q.i = axis.i * inverse_s;
        q.j = axis.j * inverse_s;
        q.k = axis.k * inverse_s;
        q.w = s * 0.5f;
        quaternion_rotate_vector(&q, forward, &rotated_forward);
    }

    matrix4x3_from_forward_up(normal, &rotated_forward, &basis);
    basis.position = *point;

    object_recompute_basis_from_marker_delta(obj, &marker, &basis);

    *out_position = obj->position;
}

#if 0
Original Ghidra decompilation (0x4bd5d0):

void item_align_to_normal_and_point(float *param_1,undefined4 *param_2)

{
  float fVar1;
  short sVar2;
  undefined4 *in_EAX;
  uint in_ECX;
  undefined1 local_d4 [84];
  float local_80;
  float local_7c;
  float local_78;
  undefined4 local_74 [4];
  undefined1 local_64 [40];
  undefined4 local_3c;
  undefined4 local_38;
  undefined4 local_34;
  int local_1c;
  float local_18;
  float local_14;
  float local_10;
  float local_c;
  float local_8;

  local_1c = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_ECX & 0xffff) * 0xc);
  sVar2 = object_get_node_local_transform();
  if (sVar2 != 0) {
    if (param_2 == (undefined4 *)0x0) {
      param_2 = local_74;
    }
    if (in_EAX == (undefined4 *)0x0) {
      in_EAX = local_74;
    }
    fVar1 = local_80 * *param_1 + local_78 * param_1[2] + local_7c * param_1[1] + 1.0;
    local_8 = SQRT(fVar1 + fVar1);
    if (local_8 <= 0.01) {
      vector3d_cross_product(param_1);
      vector3d_cross_product(&local_14);
    }
    else {
      vector3d_cross_product(&local_80);
      fVar1 = 1.0 / local_8;
      local_18 = local_18 * fVar1;
      local_14 = local_14 * fVar1;
      local_10 = local_10 * fVar1;
      local_c = local_8 * 0.5;
      quaternion_rotate_vector(&local_18);
    }
    vector3d_normalize_with_length();
    matrix4x3_from_forward_up(local_64);
    local_3c = *param_2;
    local_34 = param_2[2];
    local_38 = param_2[1];
    object_recompute_basis_from_marker_delta(local_d4,local_64);
    *in_EAX = *(undefined4 *)(local_1c + 0x5c);
    in_EAX[1] = *(undefined4 *)(local_1c + 0x60);
    in_EAX[2] = *(undefined4 *)(local_1c + 100);
  }
  return;
}
#endif
