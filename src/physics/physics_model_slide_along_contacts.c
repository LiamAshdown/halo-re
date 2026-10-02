// physics_model_slide_along_contacts  (Ghidra: FUN_005067b0; renamed)
// address 0x5067b0, size 2028 bytes (Ghidra reported 1257; the velocity switch at 0x506c92, its
//   cases and the synthetic-contact tail run to the ret at 0x506f9a, jump table at 0x506f9c)
// name confidence: 0.4   rewrite confidence: 0.85 (step 1: rewritten from objdump -d
//   0x5067b0..0x506f9c; the draft did not cover the tail and guessed its helpers)
// evidence: moves a point from start_position by delta through a physics_model:
//   - while the step is not negligible (stops once |x|, |y| and |z| are all < 0.0001, a double
//     compare against 0x672bd8) and fewer than max_contacts are recorded, cast the step with
//     physics_shape_test_ray (0x504bb0) into contacts[count]; a miss moves to its end point.
//   - a hit scales the remaining delta by (1 - t), snaps the point onto the contact plane and
//     clips the step: against the new plane alone, along the line where it meets the first
//     remembered plane (plane3d_intersect_pair_to_line 0x4cf1e0, point3d_project_onto_line
//     0x5066e0), or to the corner of three planes (plane3d_intersect_three 0x4cf040), falling back
//     to the second remembered plane when the first does not clip (dot < -0.0001, 0x672bb8).
//   - out_velocity by active plane count (table 0x506f9c): delta; delta minus its component along
//     the last plane; delta projected onto the crease line (vector3d_project_onto_direction 0x506760);
//     zero.
//   - with two or three planes active and room left, one more contact is appended at the last
//     remembered plane's contact point, with no object or surface, whose plane is a floor built
//     from the most downward-facing active plane (lowest normal.k below 0): for a crease, the
//     cross product of that plane with the crease line (world up minus its crease component when
//     none faces down); for a corner, world up minus that plane's k-scaled normal (world up
//     itself when none faces down). A degenerate normal drops the appended contact again.
// register convention: start position in EAX, six stack arguments; the contact count in AX.
//   // blam-cc: EAX -> start_position, stack -> delta, model, out_position, out_velocity,
//   //          max_contacts, contacts

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "physics.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern double fabs(double x);
extern const real_vector3d *global_up3d_pointer; // 0x00696720

extern uint32_t physics_shape_test_ray(physics_model *model, real_point3d *origin, real_vector3d *delta,
    physics_model_contact *out_contact); // 0x504bb0
extern uint8_t plane3d_intersect_pair_to_line(real_vector3d *direction_out, real_plane3d *p2, real_plane3d *p1,
    real_point3d *point_out); // 0x4cf1e0, blam-cc: ECX direction_out, EDX p2, ESI p1, EDI point_out
extern uint8_t plane3d_intersect_three(real_plane3d *p1, real_plane3d *p2, real_plane3d *p3,
    real_point3d *out); // 0x4cf040, blam-cc: stack p1, EBX p2, EDI p3, ESI out
extern void point3d_project_onto_line(real_point3d *point, real_vector3d *direction, real_point3d *line_origin,
    real_point3d *out_result); // 0x5066e0, blam-cc: stack point, EAX direction, ECX line_origin, EDX out
extern void vector3d_project_onto_direction(real_vector3d *out, const real_vector3d *axis,
    const real_vector3d *v); // 0x506760, blam-cc: ECX out, EAX axis, EDX v
extern void vector3d_cross_product(real_vector3d *out, const real_vector3d *a, const real_vector3d *b);
    // 0x4052c0, blam-cc: EAX out, ECX a, stack b (computes b x a)
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, blam-cc: ECX v

#define CONTACT_PLANE(c) ((real_plane3d *)&(c)->plane_i)

static float dot3(const real_vector3d *a, const real_vector3d *b)
{
    return a->i * b->i + a->j * b->j + a->k * b->k;
}

// blam-cc: EAX -> start_position, stack -> delta, model, out_position, out_velocity, max_contacts, contacts
// Slides a point along the world's collision proxies and reports every surface it touched.
int16_t physics_model_slide_along_contacts(real_point3d *start_position, real_vector3d *delta,
    physics_model *model, real_point3d *out_position, real_vector3d *out_velocity, int16_t max_contacts,
    physics_model_contact *contacts)
{
    const double epsilon = (double)0.0001f;         // 0x672bd8
    real_vector3d remaining = *delta;
    real_point3d position = *start_position;
    real_vector3d step = *delta;
    int16_t contact_count = 0;
    int16_t last_contact = -1;
    int16_t planes[3];
    int16_t plane_count = 0;
    real_plane3d plane;                              // the last contact's plane
    real_vector3d line_direction;                    // the active crease
    real_point3d line_point;
    real_point3d corner;

    for (;;) {
        physics_model_contact *contact;
        real_point3d hit_point;
        int16_t new_planes[3];
        int16_t new_count;
        float scale;
        float along;
        int16_t i;

        if (fabs(step.i) < epsilon && fabs(step.j) < epsilon && fabs(step.k) < epsilon) {
            break;
        }
        contact = &contacts[contact_count];
        if (!physics_shape_test_ray(model, &position, &step, contact)) {
            position.x = contact->point_x;
            position.y = contact->point_y;
            position.z = contact->point_z;
            break;
        }
        contact_count++;
        scale = 1.0f - contact->t;
        remaining.i *= scale;
        remaining.j *= scale;
        remaining.k *= scale;
        hit_point.x = contact->point_x;
        hit_point.y = contact->point_y;
        hit_point.z = contact->point_z;

        last_contact++;
        plane = *CONTACT_PLANE(&contacts[last_contact]);
        new_planes[0] = last_contact;
        new_count = 1;

        along = -dot3(&plane.normal, &remaining);
        step.i = plane.normal.i * along + remaining.i;
        step.j = plane.normal.j * along + remaining.j;
        step.k = plane.normal.k * along + remaining.k;
        along = -((plane.normal.i * hit_point.x + plane.normal.j * hit_point.y + plane.normal.k * hit_point.z) -
                  plane.d);
        position.x = plane.normal.i * along + hit_point.x;
        position.y = plane.normal.j * along + hit_point.y;
        position.z = plane.normal.k * along + hit_point.z;

        if (plane_count > 0) {
            real_plane3d *plane0 = CONTACT_PLANE(&contacts[planes[0]]);
            real_plane3d *new_plane = CONTACT_PLANE(&contacts[last_contact]);
            uint8_t creased = 0;

            if (dot3(&step, &plane0->normal) < -0.0001f &&
                plane3d_intersect_pair_to_line(&line_direction, plane0, new_plane, &line_point)) {
                creased = 1;
                new_planes[1] = planes[0];
                new_count = 2;
                along = dot3(&line_direction, &remaining) / dot3(&line_direction, &line_direction);
                step.i = line_direction.i * along;
                step.j = line_direction.j * along;
                step.k = line_direction.k * along;
                point3d_project_onto_line(&hit_point, &line_direction, &line_point, &position);
                if (plane_count > 1) {
                    real_plane3d *plane1 = CONTACT_PLANE(&contacts[planes[1]]);
                    if (dot3(&step, &plane1->normal) < -0.0001f &&
                        plane3d_intersect_three(new_plane, plane0, plane1, &corner)) {
                        new_planes[2] = planes[1];
                        new_count = 3;
                        step.i = 0.0f;
                        step.j = 0.0f;
                        step.k = 0.0f;
                        position = corner;
                    }
                }
            }
            if (!creased && plane_count > 1) {
                real_plane3d *plane1 = CONTACT_PLANE(&contacts[planes[1]]);
                if (dot3(&step, &plane1->normal) < -0.0001f &&
                    plane3d_intersect_pair_to_line(&line_direction, plane1, new_plane, &line_point)) {
                    new_planes[1] = planes[1];
                    new_count = 2;
                    along = dot3(&line_direction, &remaining) / dot3(&line_direction, &line_direction);
                    step.i = line_direction.i * along;
                    step.j = line_direction.j * along;
                    step.k = line_direction.k * along;
                    point3d_project_onto_line(&hit_point, &line_direction, &line_point, &position);
                }
            }
        }

        for (i = 0; i < new_count; i++) {
            planes[i] = new_planes[i];
        }
        plane_count = new_count;
        if (contact_count >= max_contacts) {
            break;
        }
    }

    *out_position = position;
    switch (plane_count) {
    case 0:
        *out_velocity = *delta;
        break;
    case 1: {
        float along = -(plane.normal.i * delta->i + plane.normal.k * delta->k + plane.normal.j * delta->j);
        out_velocity->i = plane.normal.i * along + delta->i;
        out_velocity->j = plane.normal.j * along + delta->j;
        out_velocity->k = plane.normal.k * along + delta->k;
        break;
    }
    case 2:
        vector3d_project_onto_direction(out_velocity, &line_direction, delta);
        break;
    default: // 3
        out_velocity->i = 0.0f;
        out_velocity->j = 0.0f;
        out_velocity->k = 0.0f;
        break;
    }

    if (plane_count > 1 && contact_count < max_contacts) {
        physics_model_contact *source = &contacts[planes[plane_count - 1]];
        physics_model_contact *floor = &contacts[contact_count];
        real_vector3d *normal = (real_vector3d *)&floor->plane_i;
        int16_t lowest = -1;
        float lowest_k = 0.0f;
        int16_t i;

        floor->t = source->t;
        floor->point_x = source->point_x;
        floor->point_y = source->point_y;
        floor->point_z = source->point_z;
        contact_count++;
        floor->object_index = 0xffffffff;
        floor->surface_index = -1;
        floor->surface_flags = 0;
        floor->breakable_surface_index = 0;
        floor->material_type = -1;

        for (i = 0; i < plane_count; i++) {
            float k = contacts[planes[i]].plane_k;
            if (lowest_k > k) {
                lowest = i;
                lowest_k = k;
            }
        }

        if (plane_count == 2) {
            if (lowest == -1) {
                float along = -(line_direction.k / (line_direction.j * line_direction.j +
                    line_direction.i * line_direction.i + line_direction.k * line_direction.k));
                normal->i = line_direction.i * along + global_up3d_pointer->i;
                normal->j = line_direction.j * along + global_up3d_pointer->j;
                normal->k = line_direction.k * along + global_up3d_pointer->k;
            } else if (lowest == 0) {
                // 0x506e18: EAX = normal, ECX = &plane.normal, push &line: line x plane
                vector3d_cross_product(normal, &CONTACT_PLANE(&contacts[planes[0]])->normal, &line_direction);
            } else {
                // 0x506e2c: EAX = normal, ECX = &line, push &plane.normal: plane x line
                vector3d_cross_product(normal, &line_direction, &CONTACT_PLANE(&contacts[planes[lowest]])->normal);
            }
            if (vector3d_normalize_with_length(normal) == 0.0f) {
                return (int16_t)(contact_count - 1);
            }
            floor->plane_d = line_point.y * normal->j + line_point.z * normal->k + line_point.x * normal->i;
        } else {
            if (lowest == -1) {
                *normal = *global_up3d_pointer;
            } else {
                real_plane3d *low = CONTACT_PLANE(&contacts[planes[lowest]]);
                float along = -low->normal.k;
                normal->i = along * low->normal.i + global_up3d_pointer->i;
                normal->j = along * low->normal.j + global_up3d_pointer->j;
                normal->k = along * low->normal.k + global_up3d_pointer->k;
                if (vector3d_normalize_with_length(normal) == 0.0f) {
                    return (int16_t)(contact_count - 1);
                }
            }
            floor->plane_d = corner.y * normal->j + corner.z * normal->k + corner.x * normal->i;
        }
    }
    return contact_count;
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
