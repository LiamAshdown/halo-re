// antenna_update_physics
// address 0x4fae10, size 943 bytes
// name confidence: 0.6 (already named in functions.md/phase2, not a FUN_xxx stub; matches the
//   body: a per-segment spring/verlet integrator over antenna->vertices)
// rewrite confidence: 0.35 (control flow and arithmetic are preserved faithfully; several
//   details were only resolved by disassembling the function directly, and one call's exact
//   operands remain UNSURE -- see below)
// evidence: types/objects.h antenna (degenerate 0x05, vertices 0x1c stride 0x20),
//   antenna_vertex (position 0x00, velocity 0x0c, unknown_1c 0x1c), types/tags.h Antenna
//   (physics TagDependency 0x30, spring_strength_coefficient 0x90, vertices TagReflexive 0xc4),
//   AntennaVertex (spring_strength_coefficient 0x00, length 0x24, offset Point3D 0x74);
//   antenna_apply_marker_delta 0x4fb1c0 (this function's own first call); vector3d_* register
//   conventions from src/math/ (normalize: v in ECX; angle_between: a,b in ECX,EDX; rotate:
//   v in EAX, axis in ECX, sin/cos on the stack); point_physics_tick 0x50b530 (module=physics,
//   modules.json), whose own decompile shows an implicit float* velocity in/out via unaff_ESI.
// register convention: resolved by disassembling 0x4fae10 directly (objdump -d -M intel
//   bin/halo.exe, 0x4fae10..0x4fb1be). All three parameters are plain cdecl stack arguments
//   ([esp+4], [esp+8], [esp+0xc] respectively after the prologue) -- Ghidra's decompiled
//   signature shows them as ordinary param_1/param_2/param_3 with no in_REG markers, and the
//   disassembly confirms there is no register-passed argument here.
//   // blam-cc: stack -> ant, antenna_tag, dt
// VERIFIED (2026-09-30): the vector3d_angle_between_4cd4f0 call is (ECX = bend_delta, EDX = the local (0,0,1) at esp+0x8c);
//   the rest offset is only rotated afterwards (see the call site).
// UNSURE: PTR_DAT_0069671c (the degenerate-axis fallback vector) is left as an unidentified
//   constant; it sits 4 bytes past the "up" default at 0x00696718/0x0069671c/0x00696720 that
//   object_placement_data cites, so it may overlap that vector rather than being independent.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

extern tag_instance *tag_instances; // 0x0087bc14
extern real_vector3d *global_left3d_pointer; // 0x0069671c, UNSURE: see file header
extern double sqrt(double x); // a single x87 FSQRT instruction in the original (Ghidra's SQRT())
extern double sin(double x); // x87 FSIN
extern double cos(double x); // x87 FCOS
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, vector in ECX
extern real vector3d_angle_between_4cd4f0(real_vector3d *a, real_vector3d *b); // 0x4cd4f0, ECX, EDX
extern void vector3d_rotate_about_axis(real_vector3d *v, real_vector3d *axis, real sin_angle, real cos_angle);
    // 0x4cd820, v in EAX, axis in ECX, sin/cos on the stack
extern void antenna_apply_marker_delta(real_vector3d *out_forward, real_point3d *out_position,
                                        antenna *ant, Antenna *antenna_tag, bsp_leaf_reference *node_ref); // 0x4fb1c0
extern uint32_t point_physics_tick(real_vector3d *velocity /*ESI*/, uint32_t mode, void *physics_tag_data,
                                      bsp_leaf_reference *node_ref, uint32_t flags, real_point3d *position,
                                      real_vector3d *wind_direction, void *unused_c, void *unused_d,
                                      float damping_constant, float dt);
    // 0x50b530, module=physics (out of this module's scope; only this call site's shape is
    // preserved). blam-cc: ESI -> velocity, stack -> the rest in the order shown; confirmed by
    // disassembling both this call site and 0x50b530's own entry (mov ebp,[esp+8] reads the
    // physics_tag_data argument; unaff_ESI is read and written as a float[3] throughout).

void antenna_update_physics(antenna *ant, Antenna *antenna_tag, float dt)
    // blam-cc: stack -> ant, antenna_tag, dt (see file header)
{
    real_vector3d marker_forward;
    real_point3d marker_position;
    bsp_leaf_reference node_ref;

    antenna_apply_marker_delta(&marker_forward, &marker_position, ant, antenna_tag, &node_ref);

    if (ant->degenerate == 0 && dt > 0.0f) {
        int32_t vertex_count = antenna_tag->vertices.count;

        if (vertex_count != -1 && vertex_count + 1 >= 0) {
            AntennaVertex *tag_vertices = (AntennaVertex *)antenna_tag->vertices.pointer;
            float inv_dt = 1.0f / dt;
            real_point3d anchor = { 0.0f, 0.0f, 0.0f };   // local_80/7c/78; not read until the
                                                           // second pass, which always follows a
                                                           // first pass that assigns it
            int32_t runtime_index = 0;   // iVar7: the antenna_vertex index (0..vertex_count)
            int16_t completed = 0;       // local_98: the loop's own pass counter

            do {
                antenna_vertex *vertex = &ant->vertices[runtime_index];
                int32_t tag_index = runtime_index;
                AntennaVertex *tag_vertex;
                float blend, one_minus_blend;
                real_point3d new_position;
                real_vector3d bend_delta;

                if (tag_index == vertex_count) {
                    // the trailing tip vertex has no tag entry of its own; reuse the last one
                    tag_index = vertex_count - 1;
                }
                tag_vertex = &tag_vertices[tag_index];
                blend = antenna_tag->spring_strength_coefficient * tag_vertex->spring_strength_coefficient;
                vertex->step_count += 1;

                if (completed == 0) {
                    new_position = marker_position;
                    bend_delta = marker_forward;
                } else {
                    float rest_scale;

                    new_position = vertex->position;

                    point_physics_tick(&vertex->velocity, 0,
                        tag_instances[antenna_tag->physics.tag_id.index].data,
                        &node_ref, 0xffffffff, &new_position, 0, 0, 0, 0.02f, dt);
                        // 0.02f matches the literal bit pattern 0x3ca3d70a in the original

                    // Rescale the (physics-updated position - anchor) delta to the tag's rest
                    // segment length, then blend it against the previous segment's rotated
                    // offset chain position by the per-vertex spring coefficient.
                    {
                        float dx = new_position.x - anchor.x;
                        float dy = new_position.y - anchor.y;
                        float dz = new_position.z - anchor.z;
                        rest_scale = (float)(tag_vertex->length / sqrt(dx * dx + dy * dy + dz * dz));
                        one_minus_blend = 1.0f - blend;
                        new_position.x = bend_delta.i * blend + (rest_scale * dx + anchor.x) * one_minus_blend;
                            // UNSURE: bend_delta here is really "local_5c" (the previous
                            // iteration's rotated offset chain position), not the same variable
                            // as the delta fed to the cross product below; they share this
                            // name only because Ghidra could not keep them apart either. See
                            // the rotate step at the end of this iteration where local_5c/58/54
                            // are actually produced.
                        new_position.y = bend_delta.j * blend + one_minus_blend * (rest_scale * dy + anchor.y);
                        new_position.z = bend_delta.k * blend + one_minus_blend * (rest_scale * dz + anchor.z);
                    }
                    bend_delta.i = new_position.x - anchor.x;
                    bend_delta.j = new_position.y - anchor.y;
                    bend_delta.k = new_position.z - anchor.z;
                }

                // Perpendicular axis: cross(bend_delta, (0,0,1)) with the compiler's folded
                // zero terms kept literal.
                {
                    real_vector3d axis;
                    real_vector3d offset;
                    float length;
                    float angle, s, c;

                    axis.i = bend_delta.k * 0.0f - bend_delta.j;
                    axis.j = bend_delta.i - bend_delta.k * 0.0f;
                    axis.k = bend_delta.j * 0.0f - bend_delta.i * 0.0f;

                    length = vector3d_normalize_with_length(&axis);
                    if (length == 0.0f) {
                        axis = *global_left3d_pointer;
                    }

                    offset.i = tag_vertex->offset.x;
                    offset.j = tag_vertex->offset.y;
                    offset.k = tag_vertex->offset.z;

                    // VERIFIED against disassembly 0x4fb0f7..0x4fb123 (2026-09-30): ECX = bend_delta (esp+0x28), EDX = the local (0,0,1) built at
                    // 0x4fae8b..0x4fae99 (esp+0x8c), NOT the rest offset
                    {
                        real_vector3d world_up_z = { 0.0f, 0.0f, 1.0f };

                        angle = vector3d_angle_between_4cd4f0(&bend_delta, &world_up_z);
                    }
                    s = (real)sin((double)angle);
                    c = (real)cos((double)angle);
                    vector3d_rotate_about_axis(&offset, &axis, s, c);

                    anchor.x = new_position.x;
                    anchor.y = new_position.y;
                    anchor.z = new_position.z;

                    completed = completed + 1;
                    runtime_index = completed;

                    {
                        float old_x = vertex->position.x;
                        float old_y = vertex->position.y;
                        float old_z = vertex->position.z;

                        vertex->velocity.i = (new_position.x - old_x) * inv_dt;
                        vertex->velocity.j = (new_position.y - old_y) * inv_dt;
                        vertex->velocity.k = (new_position.z - old_z) * inv_dt;
                        vertex->position = new_position;
                    }

                    // (offset.x + new_position.x, ...) becomes the NEXT iteration's bend_delta
                    // seed in the original (local_5c/58/54); re-derive it the same way here so
                    // the next pass's "one_minus_blend * (... )" term lines up. This is folded
                    // into bend_delta directly above rather than kept as a separate variable.
                    bend_delta.i = offset.i + new_position.x;
                    bend_delta.j = offset.j + new_position.y;
                    bend_delta.k = offset.k + new_position.z;
                }

                vertex_count = antenna_tag->vertices.count;
            } while (runtime_index < vertex_count + 1);
        }
    }
}

#if 0
Original Ghidra decompilation (0x4fae10):

void antenna_update_physics(int param_1,int param_2,float param_3)

{
  float *pfVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  int iVar5;
  float *pfVar6;
  int iVar7;
  float10 fVar8;
  unkbyte10 Var9;
  float10 fVar10;
  float local_a8;
  float local_a4;
  float local_a0;
  float local_9c;
  int local_98;
  float local_94;
  float local_90;
  float local_8c;
  float local_88;
  float local_84;
  float local_80;
  float local_7c;
  float local_78;
  float local_74;
  float local_70;
  float local_6c;
  float local_68;
  float local_64;
  float local_60;
  float local_5c;
  float local_58;
  float local_54;
  float local_50;
  float local_4c;
  float local_48;
  float local_44;
  float local_40;
  float local_3c;
  float local_30;
  undefined4 local_2c;
  undefined4 local_28;
  undefined4 local_24;
  float local_20;
  float local_1c;
  float local_18;
  undefined1 local_14 [8];
  float local_c;

  FUN_004fb1c0(param_2,local_14);
  if ((*(char *)(param_1 + 5) == '\0') && (0.0 < param_3)) {
    iVar5 = *(int *)(param_2 + 0xc4);
    iVar7 = 0;
    local_98 = 0;
    if (iVar5 != -1 && -1 < iVar5 + 1) {
      local_94 = 1.0 / param_3;
      local_2c = 0;
      local_28 = 0;
      local_24 = 0x3f800000;
      do {
        pfVar1 = (float *)(iVar7 * 0x20 + 0x1c + param_1);
        if (iVar7 == iVar5) {
          iVar7 = iVar5 + -1;
        }
        pfVar6 = (float *)(iVar7 * 0x80 + *(int *)(param_2 + 200));
        local_9c = *(float *)(param_2 + 0x90) * *pfVar6;
        *(short *)(pfVar1 + 7) = *(short *)(pfVar1 + 7) + 1;
        if ((short)local_98 == 0) {
          local_a0 = local_3c;
          local_a8 = local_44;
          local_a4 = local_40;
          local_90 = local_20;
          local_8c = local_1c;
          local_88 = local_18;
        }
        else {
          local_a8 = *pfVar1;
          local_a4 = pfVar1[1];
          local_a0 = pfVar1[2];
          FUN_0050b530(0,*(undefined4 *)
                          ((*(uint *)(param_2 + 0x3c) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14),
                       local_14,0xffffffff,&local_a8,0,0,0,0x3ca3d70a,param_3);
          local_a8 = local_a8 - local_80;
          local_a4 = local_a4 - local_7c;
          local_30 = local_a0 - local_78;
          local_84 = pfVar6[9] /
                     SQRT(local_a8 * local_a8 + local_a4 * local_a4 + local_30 * local_30);
          local_c = local_84 * local_a8 + local_80;
          fVar3 = 1.0 - local_9c;
          local_a8 = local_5c * local_9c + local_c * fVar3;
          local_a4 = local_58 * local_9c + fVar3 * (local_84 * local_a4 + local_7c);
          local_a0 = local_54 * local_9c + fVar3 * (local_84 * local_30 + local_78);
          local_90 = local_a8 - local_80;
          local_8c = local_a4 - local_7c;
          local_88 = local_a0 - local_78;
        }
        fVar4 = local_a4;
        fVar3 = local_a8;
        local_74 = local_88 * 0.0 - local_8c;
        local_70 = local_90 - local_88 * 0.0;
        local_6c = local_8c * 0.0 - local_90 * 0.0;
        local_50 = local_74;
        local_4c = local_70;
        local_48 = local_6c;
        fVar8 = (float10)vector3d_normalize_with_length();
        if ((float10)0.0 == fVar8) {
          local_74 = *(float *)PTR_DAT_0069671c;
          local_70 = *(float *)(PTR_DAT_0069671c + 4);
          local_6c = *(float *)(PTR_DAT_0069671c + 8);
        }
        local_68 = pfVar6[0x1d];
        local_64 = pfVar6[0x1e];
        local_60 = pfVar6[0x1f];
        Var9 = vector3d_angle_between_4cd4f0();
        fVar8 = (float10)fcos(Var9);
        fVar10 = (float10)fsin(Var9);
        vector3d_rotate_about_axis((float)fVar10,(float)fVar8);
        local_5c = local_68 + local_a8;
        local_78 = local_a0;
        local_98 = local_98 + 1;
        local_58 = local_64 + local_a4;
        iVar7 = (int)(short)local_98;
        local_54 = local_60 + local_a0;
        pfVar1[3] = (local_a8 - *pfVar1) * local_94;
        pfVar1[4] = (local_a4 - pfVar1[1]) * local_94;
        fVar2 = pfVar1[2];
        *pfVar1 = fVar3;
        pfVar1[1] = fVar4;
        pfVar1[2] = local_a0;
        pfVar1[5] = (local_a0 - fVar2) * local_94;
        iVar5 = *(int *)(param_2 + 0xc4);
        local_80 = fVar3;
        local_7c = fVar4;
      } while (iVar7 < iVar5 + 1);
    }
  }
  return;
}
#endif
