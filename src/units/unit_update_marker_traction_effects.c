// unit_update_marker_traction_effects  (Ghidra: FUN_00575170; renamed from the phase2 proposal)
// address 0x575170, size 750 bytes
// name confidence: 0.35 (phase2 proposal at 0.35, matches functions.md summary)
// rewrite confidence: 0.9 (REWRITTEN from objdump 0x575170..0x57545d) -- the animation-graph node-array traversal (iVar6/local_18, its own
//   nested count+pointer sub-block at +0x68/+0x6c) and the per-physics-node record it indexes
//   into (physics_tag+0x78, stride 0x80) are not documented in any header available to this
//   module; reproduced with Ghidra's own locals rather than invented field names.
// evidence: types/units.h vehicle_data.contact_point_traction (0x4f4, "0x575170 reads and
//   rewrites entry i, 0xff meaning full traction"); types/objects.h object.position (0x05c);
//   types/tags.h Vehicle.suspension_sound (tag_id at absolute 0x3bc, per the module's
//   TagDependency-at-relative-+0xc idiom); the physics.tag_id-at-0x8c idiom (contact-point
//   count at Physics+0x74).
// VERIFIED against disassembly 0x575170..0x57545d (2026-09-30): node/graph gates, the mass point transform (0x38 / 0x50),
//   origin/delta arithmetic, the clamps, the traction byte, the largest rise and the 0x3bc sound call (constants 0.3 and 1/0.6
//   checked in the image).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "projectiles.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern void matrix4x3_from_forward_up(real_vector3d *up, real_vector3d *forward, real_matrix4x3 *out); // 0x4cb970
extern void matrix4x3_transform_point(real_point3d *out, real_point3d *point, real_matrix4x3 *m); // 0x4cbde0
extern void matrix4x3_transform_normal(real_vector3d *out, real_vector3d *normal, real_matrix4x3 *m); // 0x4cbec0
extern uint8_t collision_test_movement_segment(uint32_t flags, real_point3d *origin, real_vector3d *delta,
    uint32_t exclude_object_index, collision_result *result); // 0x505880
extern uint8_t lerp_find_threshold_byte(real lo, real hi, real threshold); // 0x4cf7a0
extern datum_index sound_start_at_object_marker(datum_index object_index, Point3D *position, Vector3D *forward,
    datum_index definition_index, int16_t node_index, float scale, uint32_t first_person_hint); // 0x543ce0, ESI, ECX, EAX, stack
extern const real_point3d *global_zero_vector3d_pointer; // 0x006966f8
extern const real_vector3d *global_forward3d_pointer;   // 0x00696718

// REWRITTEN from objdump. Needs the animation graph (tag +0x44) with a node array (graph +0x24 count, +0x28
//   pointer). The object basis (matrix4x3_from_forward_up(up +0x80, forward +0x74) with position +0x5c) places
//   each suspension entry (array +0x68 count, +0x6c pointer, stride 0x14: +0 contact index, +2 node, +4/+8 the
//   extension range). Its physics mass point (physics +0x78, stride 0x80: +0x38 point, +0x50 normal) is
//   transformed, and a segment (flags 0xc0a0, excluding the object) is cast along the normal from
//   point + normal * (range_lo - physics +0x14 - (range_hi - range_lo)) for 2 * (range_hi - range_lo). The
//   compression v = clamp((1 - t) * 2, 0, 1); the per-contact traction byte (+0x4f4 + i, 0xff = 1.0) becomes
//   lerp_find_threshold_byte(0, 1, (v + old) / 2), and the largest rise v - old drives the suspension sound
//   (tag +0x3bc) at clamp((rise - 0.3) * 1.6667, 0, 1) once it passes 0.3; returns 1 when the sound starts.
//   The draft transformed the wrong mass-point fields, ignored physics +0x14 and the cast's result (t was
//   always 0), and cast into a 20-byte buffer (the result is 0x50 bytes, so it overwrote the stack).
// blam-cc: stack -> object_index
uint32_t unit_update_marker_traction_effects(uint32_t object_index)
{
    uint8_t *obj = (uint8_t *)((object_header *)object_data->data)[object_index & 0xffff].data;
    uint8_t *tag = (uint8_t *)tag_instances[*(datum_index *)obj & 0xffff].data;
    uint8_t *graph;
    uint8_t *node_array;
    uint8_t *physics;
    real_matrix4x3 basis;
    real max_rise = 0.0f;
    int16_t i;

    if (*(int32_t *)&((Unit *)tag)->base.animation_graph.tag_id == -1) {
        return 0;
    }
    graph = (uint8_t *)tag_instances[*(uint32_t *)&((Unit *)tag)->base.animation_graph.tag_id & 0xffff].data;
    if (*(int32_t *)&((ModelAnimations *)graph)->vehicles.count == 0) {
        return 0;
    }
    node_array = *(uint8_t **)&((ModelAnimations *)graph)->vehicles.pointer;
    if (node_array == 0) {
        return 0;
    }
    physics = (uint8_t *)tag_instances[*(uint32_t *)&((Unit *)tag)->base.physics.tag_id & 0xffff].data;
    matrix4x3_from_forward_up((real_vector3d *)(obj + 0x80), (real_vector3d *)(obj + 0x74), &basis);
    basis.position = *(real_point3d *)&((unit_object *)obj)->base.position.x;

    for (i = 0; (int32_t)i < *(int32_t *)(node_array + 0x68); i++) {
        uint8_t *entry = *(uint8_t **)(node_array + 0x6c) + (int32_t)i * 0x14;
        int16_t contact_index = *(int16_t *)entry;
        uint8_t *mass_point;
        uint8_t old_byte;
        real old, range, offset, v, rise;
        real_point3d point;
        real_vector3d normal;
        real_point3d origin;
        real_vector3d delta;
        collision_result result;

        if (contact_index < 0 || (int32_t)contact_index >= *(int32_t *)(physics + 0x74) ||
            *(int16_t *)(entry + 2) == -1) {
            continue;
        }
        mass_point = *(uint8_t **)(physics + 0x78) + (int32_t)contact_index * 0x80;
        old_byte = obj[0x4f4 + i];
        old = old_byte == 0xff ? 1.0f : (real)old_byte * 0.003921569f;
        matrix4x3_transform_point(&point, (real_point3d *)(mass_point + 0x38), &basis);
        matrix4x3_transform_normal(&normal, (real_vector3d *)(mass_point + 0x50), &basis);
        range = *(real *)(entry + 4) - *(real *)(entry + 8);
        offset = *(real *)(entry + 8) - *(real *)(physics + 0x14) - range;
        origin.x = normal.i * offset + point.x;
        origin.y = normal.j * offset + point.y;
        origin.z = normal.k * offset + point.z;
        range = range + range;
        delta.i = normal.i * range;
        delta.j = normal.j * range;
        delta.k = normal.k * range;
        collision_test_movement_segment(0xc0a0, &origin, &delta, object_index, &result);
        v = (1.0f - result.t) + (1.0f - result.t);
        if (!(v >= 0.0f)) {
            v = 0.0f;
        } else if (!(v <= 1.0f)) {
            v = 1.0f;
        }
        rise = v - old;
        if (rise > max_rise) {
            max_rise = rise;
        }
        obj[0x4f4 + i] = lerp_find_threshold_byte(0.0f, 1.0f, (v + old) * 0.5f);
    }

    if (*(int32_t *)(tag + 0x3bc) != -1 && max_rise > 0.3f) {
        real scale = (max_rise - 0.3f) * 1.6666667f;
        if (!(scale >= 0.0f)) {
            scale = 0.0f;
        } else if (!(scale <= 1.0f)) {
            scale = 1.0f;
        }
        // 0x575424: ESI the object (stack arg), ECX *0x006966f8, EAX *0x00696718
        sound_start_at_object_marker(object_index, (Point3D *)global_zero_vector3d_pointer,
            (Vector3D *)global_forward3d_pointer, *(datum_index *)(tag + 0x3bc), -1, scale, 0);
        return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x575170):

undefined4 FUN_00575170(uint param_1)

{
  byte bVar1;
  short sVar2;
  uint *puVar3;
  float fVar4;
  undefined1 uVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  undefined1 local_e0 [20];
  float local_cc;
  undefined1 local_90 [40];
  uint local_68;
  uint local_64;
  uint local_60;
  float local_54;
  float local_50;
  float local_4c;
  float local_48;
  float local_44;
  float local_40;
  float local_3c;
  float local_38;
  float local_34;
  int local_30;
  int local_2c;
  float local_28;
  float local_24;
  float local_20;
  int local_1c;
  int local_18;
  int local_14;
  float local_10;
  float local_c;

  puVar3 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  iVar8 = *(int *)((*puVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if (((*(uint *)(iVar8 + 0x44) == 0xffffffff) ||
      (iVar6 = *(int *)((*(uint *)(iVar8 + 0x44) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14),
      *(int *)(iVar6 + 0x24) == 0)) || (iVar6 = *(int *)(iVar6 + 0x28), iVar6 == 0)) {
    return 0;
  }
  local_14 = *(int *)((*(uint *)(iVar8 + 0x8c) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  local_10 = 0.0;
  local_2c = iVar8;
  local_18 = iVar6;
  matrix4x3_from_forward_up(local_90);
  local_68 = puVar3[0x17];
  local_64 = puVar3[0x18];
  local_60 = puVar3[0x19];
  local_1c = 0;
  if (0 < *(int *)(iVar6 + 0x68)) {
    iVar7 = 0;
    do {
      sVar2 = *(short *)(*(int *)(iVar6 + 0x6c) + iVar7 * 0x14);
      iVar8 = *(int *)(iVar6 + 0x6c) + iVar7 * 0x14;
      if (((-1 < sVar2) && ((int)sVar2 < *(int *)(local_14 + 0x74))) &&
         (*(short *)(iVar8 + 2) != -1)) {
        bVar1 = *(byte *)(iVar7 + 0x4f4 + (int)puVar3);
        local_30 = sVar2 * 0x80 + *(int *)(local_14 + 0x78);
        if (bVar1 == 0xff) {
          local_c = 1.0;
        }
        else {
          local_c = (float)bVar1 * 0.003921569;
        }
        matrix4x3_transform_point(local_90);
        matrix4x3_transform_normal(local_90);
        local_34 = *(float *)(iVar8 + 4) - *(float *)(iVar8 + 8);
        fVar4 = (*(float *)(iVar8 + 8) - *(float *)(local_14 + 0x14)) - local_34;
        local_48 = local_28 * fVar4 + local_54;
        local_44 = local_24 * fVar4 + local_50;
        local_40 = local_20 * fVar4 + local_4c;
        local_34 = local_34 + local_34;
        local_3c = local_28 * local_34;
        local_38 = local_24 * local_34;
        local_34 = local_20 * local_34;
        FUN_00505880(0xc0a0,&local_48,&local_3c,param_1,local_e0);
        fVar4 = (1.0 - local_cc) + (1.0 - local_cc);
        if (0.0 <= fVar4) {
          if (1.0 < fVar4) {
            fVar4 = 1.0;
          }
        }
        else {
          fVar4 = 0.0;
        }
        if (local_10 < fVar4 - local_c) {
          local_10 = fVar4 - local_c;
        }
        uVar5 = lerp_find_threshold_byte(0,0x3f800000,(fVar4 + local_c) * 0.5);
        *(undefined1 *)(iVar7 + 0x4f4 + (int)puVar3) = uVar5;
      }
      local_1c = local_1c + 1;
      iVar7 = (int)(short)local_1c;
      iVar6 = local_18;
      iVar8 = local_2c;
    } while (iVar7 < *(int *)(local_18 + 0x68));
  }
  if ((*(int *)(iVar8 + 0x3bc) != -1) && (0.3 < local_10)) {
    local_c = (local_10 - 0.3) * 1.6666667;
    if (0.0 <= local_c) {
      if (1.0 < local_c) {
        local_c = 1.0;
      }
    }
    else {
      local_c = 0.0;
    }
    FUN_00543ce0(*(int *)(iVar8 + 0x3bc),0xffffffff,local_c,0);
    return 1;
  }
  return 0;
}
#endif
