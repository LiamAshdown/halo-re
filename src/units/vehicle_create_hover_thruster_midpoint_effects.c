// vehicle_create_hover_thruster_midpoint_effects  (Ghidra: already named)
// address 0x574bc0, size 860 bytes
// name confidence: 0.5 (cea-pdb hints via the "hover thrusters"/"midpoint" strings)
// rewrite confidence: 0.85 (REWRITTEN from objdump 0x574bc0..0x574f1c) -- mirrors vehicle_create_hover_thruster_effects.c's treatment for
//   the same reasons (register-only marker/raycast buffers); this variant additionally
//   computes a marker-to-hit "midpoint" position and passes a 4-string label set (kind == 4)
//   to effect_new_with_color.
// evidence: types/tags.h Vehicle.effect (tag_id at absolute 0x3ec); types/units.h
//   unit_data.unknown_338 ("a 0..1 scalar the vehicle lean, thruster and ground-effect routines
//   all multiply by", gating this function on being > 0, matching the functions.md summary's
//   "gated on the unit's speed (offset 0xce > 0)"); callees as in the sibling function.
// UNSURE: see vehicle_create_hover_thruster_effects.c's header; the same caveats apply to the
//   raycast result field offsets and to effect_new_with_color's exact argument shape for kind == 4.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "projectiles.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern random_seed effect_random_seed; // 0x00719cd4, passed in EDI

extern int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name,
                                                void *marker, uint32_t flags); // 0x4f6080
extern real_vector3d *vector3d_randomize_direction(real_point3d *direction, real_vector3d *out,
    void *seed, real lo, real hi); // 0x4cd1b0, EAX, EBX, EDI, stack
extern uint8_t collision_test_movement_segment(uint32_t flags, real_point3d *origin, real_vector3d *delta,
    uint32_t exclude_object_index, collision_result *result); // 0x505880
extern void effect_new_with_color(uint32_t effect, uint32_t creator, void *velocity, int32_t count,
    char **names, real_point3d *points, real_vector3d *vectors, float a_scale, float b_scale,
    int32_t color, int32_t tint, int32_t force); // 0x450980, this call site's shape

// REWRITTEN from objdump. Runs only with tag +0x3ec set and the throttle (+0x338) above 0. For each of up to
//   15 "hover thrusters" markers (stride 0x6c: +0x3c forward, +0x60 position) the forward is randomized
//   (lo 0, hi 15, seed 0x719cd4) and cast one unit from the marker (flags 0x61, excluding the unit). On a hit,
//   v = -forward.k * (1 - t) * throttle, capped at 1 and skipped unless it is above 0. The tag +0x3ec effect
//   spawns with four names: "incident", "normal" and "reflected" at the hit point, and "midpoint" halfway
//   between the marker and the hit, which is given the reflected direction; both scales are v. The draft
//   cast from an uninitialized array into a 20-byte buffer (the result is 0x50 bytes, so it overwrote the
//   stack).
// blam-cc: stack -> unit_index
void vehicle_create_hover_thruster_midpoint_effects(uint32_t unit_index)
{
    uint8_t *obj = (uint8_t *)((object_header *)object_data->data)[unit_index & 0xffff].data;
    uint8_t *tag = (uint8_t *)tag_instances[*(datum_index *)obj & 0xffff].data;
    uint8_t markers[15 * 0x6c];
    int16_t count;
    int16_t i;
    static char *names[4] = { "incident", "normal", "reflected", "midpoint" };

    if (*(int32_t *)(tag + 0x3ec) == -1 || !(((struct vehicle_object *)obj)->unit.driver_seat_power > 0.0f)) {
        return;
    }
    count = (int16_t)object_get_node_local_transform(unit_index, "hover thrusters", markers, 0xf);
    for (i = 0; i < count; i++) {
        uint8_t *marker = markers + (int32_t)i * 0x6c;
        real_point3d *marker_position = (real_point3d *)(marker + 0x60);
        real_vector3d direction;
        real_vector3d delta;
        collision_result result;
        real v;

        vector3d_randomize_direction((real_point3d *)(marker + 0x3c), &direction, &effect_random_seed, 0.0f, 15.0f);
        delta = direction;
        if (!collision_test_movement_segment(0x61, marker_position, &delta, unit_index, &result)) {
            continue;
        }
        v = -*(real *)(marker + 0x44) * (1.0f - result.t) * ((struct vehicle_object *)obj)->unit.driver_seat_power;
        if (v < 0.0f) {
            continue;
        }
        if (v > 1.0f) {
            v = 1.0f;
        } else if (!(v > 0.0f)) {
            continue;
        }
        {
            real_point3d points[4];
            real_vector3d vectors[4];
            real twice_dot;

            points[0] = result.point;
            points[1] = result.point;
            points[2] = result.point;
            points[3].x = (result.point.x + marker_position->x) * 0.5f;
            points[3].y = (result.point.y + marker_position->y) * 0.5f;
            points[3].z = (result.point.z + marker_position->z) * 0.5f;
            vectors[0].i = -direction.i;
            vectors[0].j = -direction.j;
            vectors[0].k = -direction.k;
            vectors[1] = result.plane.normal;
            twice_dot = result.plane.normal.k * direction.k + result.plane.normal.j * direction.j +
                result.plane.normal.i * direction.i;
            twice_dot = twice_dot + twice_dot;
            vectors[2].i = direction.i - result.plane.normal.i * twice_dot;
            vectors[2].j = direction.j - result.plane.normal.j * twice_dot;
            vectors[2].k = direction.k - result.plane.normal.k * twice_dot;
            vectors[3] = vectors[2];
            effect_new_with_color(*(uint32_t *)(tag + 0x3ec), 0xffffffff, 0, 4, names, points, vectors, v, v,
                0, 0, 1);
        }
    }
}

#if 0
Original Ghidra decompilation (0x574bc0):

void vehicle_create_hover_thruster_midpoint_effects(uint param_1)

{
  uint *puVar1;
  int iVar2;
  float fVar3;
  char cVar4;
  short sVar5;
  short sVar6;
  int iVar7;
  float local_7b4;
  float local_7b0;
  float local_7ac;
  float local_7a4;
  undefined1 local_78c [12];
  char *local_780;
  char *local_77c;
  char *local_778;
  char *local_774;
  float local_770;
  float local_76c;
  float local_768;
  float local_764;
  float local_760;
  float local_75c;
  float local_758;
  float local_754;
  float local_750;
  float local_74c;
  float local_748;
  float local_744;
  float local_740;
  float local_73c;
  float local_738;
  float local_734;
  float local_730;
  float local_72c;
  float local_728;
  float local_724;
  float local_720;
  float local_71c;
  float local_718;
  float local_714;
  undefined1 local_710 [20];
  float local_6fc;
  float local_6f8;
  float local_6f4;
  float local_6f0;
  float local_6ec;
  float local_6e8;
  float local_6e4;
  undefined1 local_6c0 [68];
  float local_67c [7];
  float local_660 [408];

  puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  iVar2 = *(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if ((*(int *)(iVar2 + 0x3ec) != -1) && (0.0 < (float)puVar1[0xce])) {
    sVar5 = object_get_node_local_transform(param_1,"hover thrusters",local_6c0,0xf);
    sVar6 = 0;
    if (0 < sVar5) {
      iVar7 = 0;
      do {
        vector3d_randomize_direction(0,0x41700000);
        cVar4 = FUN_00505880(0x61,local_660 + iVar7 * 0x1b,local_78c,param_1,local_710);
        if ((cVar4 != '\0') &&
           (local_7a4 = (1.0 - local_6fc) * -local_67c[iVar7 * 0x1b] * (float)puVar1[0xce],
           0.0 <= local_7a4)) {
          if (local_7a4 <= 1.0) {
            if (local_7a4 <= 0.0) goto LAB_00574f01;
          }
          else {
            local_7a4 = 1.0;
          }
          local_770 = local_6f8;
          local_74c = (local_6f8 + local_660[iVar7 * 0x1b]) * 0.5;
          local_76c = local_6f4;
          local_768 = local_6f0;
          local_764 = local_6f8;
          local_760 = local_6f4;
          local_75c = local_6f0;
          local_758 = local_6f8;
          local_754 = local_6f4;
          local_748 = (local_6f4 + local_660[iVar7 * 0x1b + 1]) * 0.5;
          local_750 = local_6f0;
          local_780 = "incident";
          local_734 = local_6ec;
          local_744 = (local_6f0 + local_660[iVar7 * 0x1b + 2]) * 0.5;
          local_730 = local_6e8;
          local_77c = "normal";
          local_740 = -local_7b4;
          local_72c = local_6e4;
          local_778 = "reflected";
          local_774 = "midpoint";
          local_73c = -local_7b0;
          local_738 = -local_7ac;
          fVar3 = local_6ec * local_7b4 + local_6e8 * local_7b0 + local_6e4 * local_7ac;
          fVar3 = fVar3 + fVar3;
          local_728 = local_7b4 - local_6ec * fVar3;
          local_724 = local_7b0 - local_6e8 * fVar3;
          local_720 = local_7ac - local_6e4 * fVar3;
          local_71c = local_7b4 - local_6ec * fVar3;
          local_718 = local_7b0 - local_6e8 * fVar3;
          local_714 = local_7ac - local_6e4 * fVar3;
          FUN_00450980(*(undefined4 *)(iVar2 + 0x3ec),0xffffffff,0,4,&local_780,&local_770,
                       &local_740,local_7a4,local_7a4,0,0,1);
        }
LAB_00574f01:
        sVar6 = sVar6 + 1;
        iVar7 = (int)sVar6;
      } while (iVar7 < sVar5);
    }
  }
  return;
}
#endif
