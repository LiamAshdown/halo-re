// physics_model_slide_along_contacts  (Ghidra: FUN_005067b0; renamed)
// address 0x5067b0, size 1257 bytes
// name confidence: 0.3   rewrite confidence: 0.35 (raised from 0.25: phase-4 integration pass corrected the no-hit position (contact->point, not contact->plane) and split the edge direction out of the slide direction) -- the lowest-confidence file in this batch;
//   see the UNSURE paragraphs below.
// evidence: types/physics.h physics_model_contact (0x2c stride; "0x5067b0 keeps an array of them
//   at the same 0x2c stride ... and reads +0x10 of entry n as the plane it slides along");
//   physics_shape_test_ray's own documented signature (physics_model*, origin, delta, out_contact);
//   plane3d_intersect_pair_to_line and plane3d_intersect_three (out/phase2 packs for
//   0x4cf1e0/0x4cf040, both math module, both fully decompiled) supply the vector-rejection /
//   line-projection / three-plane-intersection formulas this function's arithmetic matches
//   exactly once the physics_model_contact field offsets are substituted in.
// register convention: in_EAX (start position, real_point3d *) is Ghidra's only recognized
//   hidden register; param_1 is the initial movement delta (real_vector3d *), param_2 the
//   physics_model *, param_3 the out position (real_point3d *, only field actually stored),
//   param_5 the maximum iteration count (matches k_physics_collision_iterations elsewhere in
//   this module), param_6 the physics_model_contact[] scratch array 0x5067b0 itself owns one
//   slot of per iteration.
//   // blam-cc: EAX -> start_position (first C parameter by convention), then stack -> delta,
//   //          model, out_position, unused_param, max_iterations, contact_scratch
// UNSURE (major): param_4 is never read anywhere in this function's body; kept as an unused
//   parameter rather than dropped, since Ghidra's own recognized signature includes it and this
//   rewrite must not silently change the calling convention.
// UNSURE (major): the two-plane ("edge") and three-plane ("vertex") branches call
//   plane3d_intersect_pair_to_line() and plane3d_intersect_three() with ZERO visible arguments;
//   Ghidra lost every register that fed them. This rewrite reconstructs the calls using each
//   callee's own known parameter order (both already fully decompiled in a sibling math-module
//   pack) and the ONLY locals the surrounding code actually reads afterward -- but the exact
//   previous-plane operands (which of the up-to-two previously tracked contacts is "A" vs "B")
//   could not be confirmed against the machine code and are a best-effort reconstruction, not a
//   proven fact.
//   One thing the surrounding code DOES pin down, and an earlier rewrite of this file got wrong:
//   the edge direction the division consumes is local_64/60/5c, a local distinct from the
//   slide direction at local_58/54/50 -- `local_50 = dot(local_64.., local_4c..) / |local_64..|^2`
//   is computed first and only THEN does `local_58 = local_64 * local_50` overwrite the slide
//   direction. So plane3d_intersect_pair_to_line's out_direction is its own local, and the slide
//   direction is derived from it; this file now models that with edge_direction.
// UNSURE (major): the function ends with an indirect jump through a jump table Ghidra could not
//   recover ("Could not recover jumptable at 0x00506c92. Too many branches"). Every path that
//   reaches it has already stored the final position into *param_3, so this rewrite treats the
//   jump as a shared "return" epilogue; it is possible the real table performs additional
//   per-active-plane-count cleanup that this rewrite does not reproduce.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "physics.h"

extern uint8_t physics_shape_test_ray(physics_model *model, real_point3d *origin, real_vector3d *delta,
    physics_model_contact *out_contact); // 0x504bb0, this module (higher half)
extern uint8_t plane3d_intersect_pair_to_line(real_vector3d *out_direction, real_plane3d *plane_b,
    real_plane3d *plane_a, real_point3d *out_point); // 0x4cf1e0, math module
    // blam-cc: ECX -> out_direction, EDX -> plane_b, ESI -> plane_a (unaff), EDI -> out_point
extern uint8_t plane3d_intersect_three(real_plane3d *plane_c, real_plane3d *plane_a,
    real_plane3d *plane_b, real_point3d *out_point); // 0x4cf040, math module
    // blam-cc: stack -> plane_c, EBX -> plane_a (unaff), EDI -> plane_b (unaff), ESI -> out_point
extern double fabs(double x); // ABS is a single x87 FABS instruction
extern void point3d_project_onto_line(real_point3d *point, real_vector3d *direction,
    real_point3d *line_origin, real_point3d *out_result); // 0x5066e0, math module (misattributed
    // to physics; not rewritten in this batch per types/physics.h section 5)
    // blam-cc: stack -> point, EAX -> direction, ECX -> line_origin, EDX -> out_result

// Iteratively slides a moving point (start_position, delta) through up to max_iterations
// contacts against *model, accumulating up to three simultaneously-active constraint planes
// (a face slide, then an edge slide along two planes' intersection line, then a full stop at
// three planes' common point), and writes the final position to *out_position. param_4 is
// unused (see file header).
void physics_model_slide_along_contacts(real_point3d *start_position, real_vector3d *delta,
    physics_model *model, real_point3d *out_position, uint32_t param_4, int16_t max_iterations,
    physics_model_contact *contact_scratch)
{
    real_point3d position;
    real_vector3d remaining_delta;
    real_vector3d slide_direction;
    int16_t active_count = 0;
    int16_t iteration = 0;
    int16_t last_contact_index = -1;
    int16_t active_planes[3] = { 0, 0, 0 };
    int16_t prev_active_planes[3] = { 0, 0, 0 };

    position = *start_position;
    remaining_delta = *delta;
    slide_direction = *delta;

    while (1) {
        physics_model_contact *contact;
        real_plane3d contact_plane;
        real_point3d contact_point;
        real_vector3d edge_direction;  // local_64/60/5c -- plane3d_intersect_pair_to_line's own
                                       // out_direction, a DIFFERENT local from slide_direction
                                       // (local_58/54/50), which the code overwrites from it
        float remaining_fraction;
        float d;
        uint16_t new_active_count;

        if ((float)fabs((double)slide_direction.i) < 0.0001f &&
            (float)fabs((double)slide_direction.j) < 0.0001f &&
            (float)fabs((double)slide_direction.k) < 0.0001f) {
            break;
        }

        contact = &contact_scratch[iteration];
        if (!physics_shape_test_ray(model, &position, &slide_direction, contact)) {
            // pfVar9[1..3] is contact->point (0x04..0x0c), NOT contact->plane (0x10..0x18);
            // physics_shape_test_ray leaves point = origin + delta on a miss, so this is the
            // ordinary "travelled the whole way" advance. Corrected by the phase-4 pass.
            position.x = contact->point_x;
            position.y = contact->point_y;
            position.z = contact->point_z;
            goto store_and_return;
        }

        remaining_fraction = 1.0f - contact->t;
        iteration++;
        remaining_delta.i *= remaining_fraction;
        remaining_delta.j *= remaining_fraction;
        remaining_delta.k *= remaining_fraction;
        contact_point.x = contact->point_x;
        contact_point.y = contact->point_y;
        contact_point.z = contact->point_z;
        last_contact_index++;

        contact_plane.normal.i = contact->plane_i;
        contact_plane.normal.j = contact->plane_j;
        contact_plane.normal.k = contact->plane_k;
        contact_plane.d = contact->plane_d;

        new_active_count = 1;
        active_planes[0] = last_contact_index;

        // Vector rejection: slide_direction = remaining_delta - normal * dot(normal, remaining_delta)
        d = -(contact_plane.normal.i * remaining_delta.i + contact_plane.normal.j * remaining_delta.j +
              contact_plane.normal.k * remaining_delta.k);
        slide_direction.i = contact_plane.normal.i * d + remaining_delta.i;
        slide_direction.j = contact_plane.normal.j * d + remaining_delta.j;
        slide_direction.k = contact_plane.normal.k * d + remaining_delta.k;

        // Push the contact point exactly onto the plane (removes numerical drift).
        d = -((contact_point.x * contact_plane.normal.i + contact_point.y * contact_plane.normal.j +
               contact_point.z * contact_plane.normal.k) - contact_plane.d);
        position.x = contact_plane.normal.i * d + contact_point.x;
        position.y = contact_plane.normal.j * d + contact_point.y;
        position.z = contact_plane.normal.k * d + contact_point.z;

        if (active_count != 0) {
            physics_model_contact *prev0 = &contact_scratch[prev_active_planes[0]];
            real_plane3d prev0_plane;
            prev0_plane.normal.i = prev0->plane_i;
            prev0_plane.normal.j = prev0->plane_j;
            prev0_plane.normal.k = prev0->plane_k;
            prev0_plane.d = prev0->plane_d;

            if (-0.0001f <= slide_direction.i * prev0_plane.normal.i +
                             slide_direction.j * prev0_plane.normal.j +
                             slide_direction.k * prev0_plane.normal.k ||
                !plane3d_intersect_pair_to_line(&edge_direction, &contact_plane, &prev0_plane,
                    &contact_point /* UNSURE: out_point target */)) {

                active_planes[1] = prev_active_planes[1];
                if (active_count > 1) {
                    physics_model_contact *prev1 = &contact_scratch[prev_active_planes[1]];
                    real_plane3d prev1_plane;
                    prev1_plane.normal.i = prev1->plane_i;
                    prev1_plane.normal.j = prev1->plane_j;
                    prev1_plane.normal.k = prev1->plane_k;
                    prev1_plane.d = prev1->plane_d;

                    if (slide_direction.i * prev1_plane.normal.i + slide_direction.j * prev1_plane.normal.j +
                            slide_direction.k * prev1_plane.normal.k < -0.0001f &&
                        plane3d_intersect_pair_to_line(&edge_direction, &contact_plane, &prev1_plane,
                            &contact_point)) {
                        real_point3d line_origin = contact_point; // UNSURE: see file header
                        real_vector3d line_dir = edge_direction;  // local_64/60/5c

                        active_planes[1] = prev_active_planes[1];
                        d = (line_dir.i * remaining_delta.i + remaining_delta.k * line_dir.k +
                             line_dir.j * remaining_delta.j) /
                            (line_dir.k * line_dir.k + line_dir.i * line_dir.i + line_dir.j * line_dir.j);
                        slide_direction.i = line_dir.i * d;
                        slide_direction.j = line_dir.j * d;
                        slide_direction.k = d * line_dir.k;
                        point3d_project_onto_line(&contact_point, &line_dir, &line_origin, &contact_point);
                        new_active_count = 2;
                    }
                }
            } else {
                real_point3d line_origin = contact_point; // UNSURE: see file header
                real_vector3d line_dir = edge_direction;   // local_64/60/5c

                active_planes[1] = prev_active_planes[0];
                new_active_count = 2;
                d = (line_dir.i * remaining_delta.i + remaining_delta.k * line_dir.k +
                     line_dir.j * remaining_delta.j) /
                    (line_dir.k * line_dir.k + line_dir.i * line_dir.i + line_dir.j * line_dir.j);
                slide_direction.i = line_dir.i * d;
                slide_direction.j = line_dir.j * d;
                slide_direction.k = d * line_dir.k;
                point3d_project_onto_line(&contact_point, &line_dir, &line_origin, &contact_point);

                if (active_count > 1) {
                    physics_model_contact *prev1 = &contact_scratch[prev_active_planes[1]];
                    real_plane3d prev1_plane;
                    prev1_plane.normal.i = prev1->plane_i;
                    prev1_plane.normal.j = prev1->plane_j;
                    prev1_plane.normal.k = prev1->plane_k;
                    prev1_plane.d = prev1->plane_d;

                    if (slide_direction.i * prev1_plane.normal.i + slide_direction.j * prev1_plane.normal.j +
                            slide_direction.k * prev1_plane.normal.k < -0.0001f) {
                        real_point3d vertex;
                        if (plane3d_intersect_three(&contact_plane, &prev0_plane, &prev1_plane, &vertex)) {
                            active_planes[2] = prev_active_planes[1];
                            slide_direction.i = 0.0f;
                            slide_direction.j = 0.0f;
                            slide_direction.k = 0.0f;
                            position = vertex;
                            new_active_count = 3;
                        }
                    }
                }
            }
        }

        active_count = new_active_count;
        {
            // the original copies exactly new_active_count * 2 bytes (a dword loop plus a
            // trailing byte loop), leaving the higher prev_active_planes slots stale
            int16_t k;
            for (k = 0; k < (int16_t)new_active_count; k++) {
                prev_active_planes[k] = active_planes[k];
            }
        }

        if (max_iterations <= iteration) {
            break;
        }
    }

store_and_return:
    *out_position = position;
    // UNSURE: original ends with an unrecoverable indirect jump keyed on active_count; see
    // file header. Every path already stored the final position above.
    return;
}

#if 0
Original Ghidra decompilation (0x5067b0):

void FUN_005067b0(float *param_1,undefined4 param_2,float *param_3,undefined4 param_4,short param_5,
                 int param_6)

{
  float fVar1;
  short sVar2;
  char cVar3;
  float *in_EAX;
  uint uVar4;
  short sVar5;
  int iVar6;
  ushort uVar7;
  ushort uVar8;
  float *pfVar9;
  short *psVar10;
  short sVar11;
  short *psVar12;
  short local_74;
  short local_72;
  short local_6c;
  short local_6a;
  short local_68;
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
  float local_38;
  float local_34;
  float local_30;
  float local_2c;
  float local_28;
  float local_24;
  float local_20;
  float local_10;
  float local_c;
  float local_8;
  float local_4;

  local_4c = *param_1;
  local_48 = param_1[1];
  local_44 = param_1[2];
  local_40 = *in_EAX;
  local_3c = in_EAX[1];
  local_38 = in_EAX[2];
  local_58 = *param_1;
  local_54 = param_1[1];
  local_50 = param_1[2];
  uVar7 = 0;
  sVar5 = 0;
  sVar11 = -1;
  while( true ) {
    if (((ABS(local_58) < 0.0001) && (ABS(local_54) < 0.0001)) && (ABS(local_50) < 0.0001))
    goto LAB_00506c74;
    pfVar9 = (float *)(sVar5 * 0x2c + param_6);
    cVar3 = FUN_00504bb0(param_2,&local_40,&local_58,pfVar9);
    if (cVar3 == '\0') break;
    fVar1 = 1.0 - *pfVar9;
    sVar5 = sVar5 + 1;
    local_4c = local_4c * fVar1;
    local_34 = pfVar9[1];
    local_30 = pfVar9[2];
    local_2c = pfVar9[3];
    sVar11 = sVar11 + 1;
    local_48 = local_48 * fVar1;
    local_44 = local_44 * fVar1;
    pfVar9 = (float *)(sVar11 * 0x2c + 0x10 + param_6);
    local_10 = *pfVar9;
    local_c = pfVar9[1];
    local_8 = pfVar9[2];
    local_4 = pfVar9[3];
    uVar8 = 1;
    local_6c = sVar11;
    fVar1 = -(local_10 * local_4c + local_c * local_48 + local_8 * local_44);
    local_58 = local_10 * fVar1 + local_4c;
    local_54 = local_c * fVar1 + local_48;
    local_50 = local_8 * fVar1 + local_44;
    fVar1 = -((local_34 * local_10 + local_30 * local_c + local_2c * local_8) - pfVar9[3]);
    local_40 = local_10 * fVar1 + local_34;
    local_3c = local_c * fVar1 + local_30;
    local_38 = local_8 * fVar1 + local_2c;
    if (uVar7 != 0) {
      iVar6 = local_74 * 0x2c;
      if ((-0.0001 <=
           local_58 * *(float *)(iVar6 + 0x10 + param_6) +
           local_54 * *(float *)(iVar6 + 0x14 + param_6) +
           local_50 * *(float *)(iVar6 + 0x18 + param_6)) ||
         (cVar3 = plane3d_intersect_pair_to_line(), cVar3 == '\0')) {
        sVar2 = local_72;
        if (((1 < uVar7) &&
            (iVar6 = local_72 * 0x2c,
            local_58 * *(float *)(iVar6 + 0x10 + param_6) +
            local_54 * *(float *)(iVar6 + 0x14 + param_6) +
            local_50 * *(float *)(iVar6 + 0x18 + param_6) < -0.0001)) &&
           (cVar3 = plane3d_intersect_pair_to_line(), cVar3 != '\0')) {
          local_6a = sVar2;
          local_50 = (local_64 * local_4c + local_44 * local_5c + local_60 * local_48) /
                     (local_5c * local_5c + local_64 * local_64 + local_60 * local_60);
          local_58 = local_64 * local_50;
          local_54 = local_60 * local_50;
          local_50 = local_50 * local_5c;
          point3d_project_onto_line(&local_34);
          uVar8 = 2;
        }
      }
      else {
        local_6a = local_74;
        uVar8 = 2;
        local_50 = (local_64 * local_4c + local_44 * local_5c + local_60 * local_48) /
                   (local_5c * local_5c + local_64 * local_64 + local_60 * local_60);
        local_58 = local_64 * local_50;
        local_54 = local_60 * local_50;
        local_50 = local_50 * local_5c;
        point3d_project_onto_line(&local_34);
        if ((1 < uVar7) &&
           ((iVar6 = local_72 * 0x2c,
            local_58 * *(float *)(iVar6 + 0x10 + param_6) +
            local_54 * *(float *)(iVar6 + 0x14 + param_6) +
            local_50 * *(float *)(iVar6 + 0x18 + param_6) < -0.0001 &&
            (cVar3 = plane3d_intersect_three(pfVar9), cVar3 != '\0')))) {
          local_68 = local_72;
          local_58 = 0.0;
          local_54 = 0.0;
          local_50 = 0.0;
          local_40 = local_28;
          local_3c = local_24;
          local_38 = local_20;
          uVar8 = 3;
        }
      }
    }
    uVar7 = uVar8;
    psVar10 = &local_6c;
    psVar12 = &local_74;
    for (uVar4 = (uint)(int)(short)uVar7 >> 1; uVar4 != 0; uVar4 = uVar4 - 1) {
      *(undefined4 *)psVar12 = *(undefined4 *)psVar10;
      psVar10 = psVar10 + 2;
      psVar12 = psVar12 + 2;
    }
    for (iVar6 = ((int)(short)uVar7 & 1U) << 1; iVar6 != 0; iVar6 = iVar6 + -1) {
      *(char *)psVar12 = (char)*psVar10;
      psVar10 = (short *)((int)psVar10 + 1);
      psVar12 = (short *)((int)psVar12 + 1);
    }
    if (param_5 <= sVar5) {
LAB_00506c74:
      *param_3 = local_40;
      param_3[1] = local_3c;
      param_3[2] = local_38;
                    /* WARNING: Could not recover jumptable at 0x00506c92. Too many branches */
                    /* WARNING: Treating indirect jump as call */
      (*(code *)(&PTR_LAB_00506f9c)[(short)uVar7])();
      return;
    }
  }
  local_40 = pfVar9[1];
  local_3c = pfVar9[2];
  local_38 = pfVar9[3];
  goto LAB_00506c74;
}
#endif
