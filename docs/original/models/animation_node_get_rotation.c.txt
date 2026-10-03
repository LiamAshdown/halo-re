// animation_node_get_rotation  (Ghidra: model_node_get_interpolated_rotation, wrong name;
// renamed per out/phase4/models_types_notes.md "Misnamed or misattributed functions" table --
// this evaluates the compressed animation codec's per-node rotation curve, not a "model node")
// address 0x4d6b60, size 386 bytes
// name confidence: 0.55   rewrite confidence: 0.85 (review pass: checked against objdump)
// evidence: out/phase4/models_types_notes.md animation_compressed_header section (header
//   layout, keyframe header bit packing, "frames before the first keyframe interpolate from
//   the default value at time 0"). Ghidra drops every argument of all 5 callee calls here
//   (the compressed-codec decode, the keyframe search and the quaternion lerp/normalize), so
//   this was rebuilt from objdump -d -M intel bin/halo.exe (scratchpad/halo_disasm.txt,
//   0x4d6b60..0x4d6ce1):
//   - the rotation_keyframe_headers appended array is read at header_base + 0x2c +
//     rotation_index*4, matching the struct comment ("followed by
//     uint32 rotation_keyframe_headers[animated rotation count]").
//   - FUN_00623e40 is floor() (see src/models/animation_overlay_interpolated_frame_orientations.c), and
//     flooring an already-floored value through the subsequent x87 fistp is a no-op, so the
//     "rounded frame" is exactly (int16_t)floor(frame).
//   - three ways to pick the bracketing keyframe pair (time_a/source_a, time_b/source_b), by
//     comparing the rounded frame against the node's keyframe time sub-array (times[0] ..
//     times[count-1]): below the first real keyframe interpolates from the node's default
//     (time 0) up to keyframe 0; a rounded frame EQUAL to the last real keyframe time (jne at
//     0x4d6c1d, not >=) interpolates from that keyframe (weight 0 exactly on it) towards the
//     default at a virtual time one past it; otherwise animation_keyframe_time_search finds
//     the keyframe pair directly (a frame past the last time would spin in 0x4d6b10, as in
//     the original).
//   - the two quaternion_lerp pointer arguments (ECX/EDX) were identified past a transient
//     `push ecx` used purely to reserve the stack slot the interpolation factor is stored
//     into right after (the pushed value itself is discarded): ECX = &quat_b, EDX = &quat_a,
//     ESI = the caller's out pointer directly (the lerp result is written straight to *out,
//     with no separate local for it), weighted so t=0 reproduces quat_a and t=1 reproduces
//     quat_b, matching quaternion_lerp's a=t-weighted / b=(1-t)-weighted convention.
// register convention: animation in ECX (in_ECX); frame, rotation index, node and the output
//   real_quaternion pointer as the recognized stack parameters, in that order.
//   // blam-cc: ECX -> animation, stack -> frame, rotation_index, node, out

#include "tags.h"
#include "math.h"
#include "models.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern double floor(double x); // 0x623e40, MSVC CRT, see src/models/animation_overlay_interpolated_frame_orientations.c
extern void animation_quaternion48_decode(animation_quaternion48 *source, real_quaternion *out); // 0x4d6380, this batch
extern int16_t animation_keyframe_time_search(uint16_t *times, int16_t count, int16_t frame); // 0x4d6b10, this batch
extern void quaternion_lerp(real_quaternion *a, real_quaternion *b, real_quaternion *out, real t); // 0x4cdcc0
extern void quaternion_normalize(real_quaternion *q); // 0x4cdb20

// Evaluates one node's compressed rotation curve at a (possibly fractional) frame: finds the
// two bracketing keyframes (or the node's default, used as an implicit keyframe at time 0
// before the first real one and again, symmetrically, one frame past the last real one) and
// slerps between them, or returns the matching keyframe/default directly when frame lands
// exactly on it.
void animation_node_get_rotation(ModelAnimationsAnimation *animation, real frame,
                                  int16_t rotation_index, int16_t node, real_quaternion *out)
{
    uint8_t *header_base;
    animation_compressed_header *header;
    uint32_t keyframe_header;
    int16_t count;
    int32_t first_index;
    animation_quaternion48 *defaults;

    header_base = (uint8_t *)animation->frame_data.pointer + animation->offset_to_compressed_data;
    header = (animation_compressed_header *)header_base;
    keyframe_header = ((uint32_t *)(header_base + 0x2c))[rotation_index];
    count = (int16_t)(keyframe_header & k_animation_keyframe_count_mask);
    defaults = (animation_quaternion48 *)(header_base + header->rotation_defaults);

    if (count == 0) {
        animation_quaternion48_decode(&defaults[node], out);
        quaternion_normalize(out);
        return;
    }

    first_index = (int16_t)(keyframe_header >> k_animation_keyframe_index_shift); // movsx ecx,dx at 0x4d6bbd: low 16 bits, signed

    {
        uint16_t *times = (uint16_t *)(header_base + header->rotation_keyframe_times) + first_index;
        animation_quaternion48 *keyframes = (animation_quaternion48 *)(header_base + header->rotation_keyframes) + first_index;
        int16_t rounded_frame = (int16_t)floor((double)frame);
        animation_quaternion48 *source_a;
        animation_quaternion48 *source_b;
        int16_t time_a, time_b;

        // both compares are movsx rounded vs movzx time (0x4d6bed..0x4d6bf5, 0x4d6c0f..0x4d6c1b):
        // the times are unsigned here, unlike the signed compares inside 0x4d6b10
        if ((int32_t)rounded_frame < (int32_t)times[0]) {
            time_a = 0;
            source_a = &defaults[node];
            time_b = (int16_t)times[0];
            source_b = &keyframes[0];
        } else if ((int32_t)rounded_frame == (int32_t)times[count - 1]) {
            time_a = (int16_t)times[count - 1];
            source_a = &keyframes[count - 1];
            time_b = (int16_t)(time_a + 1);
            source_b = &defaults[node];
        } else {
            int16_t index = animation_keyframe_time_search(times, count, rounded_frame);
            time_a = (int16_t)times[index];
            time_b = (int16_t)times[index + 1];
            source_a = &keyframes[index];
            source_b = &keyframes[index + 1];
        }

        if (frame != (real)time_a) {
            real_quaternion quat_a, quat_b;
            real t;

            animation_quaternion48_decode(source_a, &quat_a);
            animation_quaternion48_decode(source_b, &quat_b);
            t = (frame - (real)time_a) / (real)(time_b - time_a);
            quaternion_lerp(&quat_b, &quat_a, out, t);
            quaternion_normalize(out);
        } else {
            animation_quaternion48_decode(source_a, out);
            quaternion_normalize(out);
        }
    }
}

#if 0
Original Ghidra decompilation (0x4d6b60):

void model_node_get_interpolated_rotation(float param_1,short param_2)

{
  uint uVar1;
  short sVar2;
  int *piVar3;
  int in_ECX;
  ushort uVar4;
  ushort *puVar5;
  float10 fVar6;

  piVar3 = (int *)(*(int *)(in_ECX + 0xac) + *(int *)(in_ECX + 0x88));
  uVar1 = piVar3[param_2 + 0xb];
  if ((uVar1 & 0xfff) == 0) {
    model_vertex_unpack_compressed_normal();
    quaternion_normalize();
    return;
  }
  puVar5 = (ushort *)(*piVar3 + (short)(uVar1 >> 0xc) * 2 + (int)piVar3);
  fVar6 = (float10)FUN_00623e40((double)param_1);
  param_2 = (short)(int)ROUND((float)fVar6);
  if ((int)param_2 < (int)(uint)*puVar5) {
    uVar4 = 0;
  }
  else {
    uVar4 = puVar5[(short)((ushort)uVar1 & 0xfff) + -1];
    if ((int)param_2 != (uint)uVar4) {
      sVar2 = FUN_004d6b10();
      uVar4 = puVar5[sVar2];
    }
  }
  if (param_1 != (float)(int)(short)uVar4) {
    model_vertex_unpack_compressed_normal();
    model_vertex_unpack_compressed_normal();
    quaternion_lerp();
    quaternion_normalize();
    return;
  }
  model_vertex_unpack_compressed_normal();
  quaternion_normalize();
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
