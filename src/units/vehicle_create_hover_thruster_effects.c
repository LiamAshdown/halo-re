// vehicle_create_hover_thruster_effects  (Ghidra: already named vehicle_create_hover_thruster_effects)
// address 0x574900, size 682 bytes
// name confidence: 0.55 (cea-pdb hints via the "hover thrusters"/"jet thrusters" strings)
// rewrite confidence: 0.85 (REWRITTEN from objdump 0x574900..0x574bb0) -- the object_marker arrays (local_6c0/auStack_660) and
//   collision_test_movement_segment's scratch output (local_710) are carried as raw byte buffers; only the
//   fields this function actually reads from them are named.
// evidence: types/tags.h Vehicle.effect (tag_id at absolute 0x3ec); types/units.h
//   vehicle_data.ground_lean (0x4ec), .ground_contact_fraction (0x4f0); callees
//   object_get_node_local_transform (established), effect_new_with_color (established 12-argument form
//   in src/hs/hs_effect_spawn_at_location.c, though this call site's position/forward slots
//   instead carry a 3-vector incident/normal/reflected triple -- reproduced with void*).
// UNSURE: the marker-transform arrays' exact per-marker stride (0x6c, smaller than
//   object_marker's 0x6c... coincidentally equal) and collision_test_movement_segment's raycast-result field
//   offsets (+0x6f8/0x6f4/0x6f0 normal, +0x6ec/0x6e8/0x6e4 a second normal-shaped vector,
//   +0x6fc a fraction) are inferred purely from their use here.

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

// REWRITTEN from objdump. Up to 15 "hover thrusters" markers then 16 - n "jet thrusters" markers
//   (marker stride 0x6c: +0x3c forward, +0x60 position). For each one the forward is randomized by up to
//   0.2618 rad (seed 0x719cd4) and cast (flags 0x61, excluding the unit) for (lean * 6 + 2) world units,
//   where lean is +0x4ec for hover markers and +0x4f0 for jets. On a hit, the tag +0x3ec effect spawns with
//   three named vectors, "incident" (-direction), "normal" (the plane normal) and "reflected", all at the hit
//   point, scaled by 1 - t. The draft cast into a 20-byte buffer (the result is 0x50 bytes, so it overwrote
//   the stack) and read the normal from the wrong offsets.
// blam-cc: stack -> unit_index
void vehicle_create_hover_thruster_effects(uint32_t unit_index)
{
    uint8_t *obj = (uint8_t *)((object_header *)object_data->data)[unit_index & 0xffff].data;
    uint8_t *tag = (uint8_t *)tag_instances[*(datum_index *)obj & 0xffff].data;
    uint8_t markers[16 * 0x6c];
    int16_t hover_count;
    int16_t total;
    int16_t i;
    static char *names[3] = { (char *)"incident", (char *)"normal", (char *)"reflected" };

    if (*(int32_t *)(tag + 0x3ec) == -1) {
        return;
    }
    hover_count = (int16_t)object_get_node_local_transform(unit_index, (char *)"hover thrusters", markers, 0xf);
    total = (int16_t)(hover_count + (int16_t)object_get_node_local_transform(unit_index, (char *)"jet thrusters",
        markers + hover_count * 0x6c, 0x10 - hover_count));

    for (i = 0; i < total; i++) {
        uint8_t *marker = markers + (int32_t)i * 0x6c;
        real_vector3d direction;
        real_vector3d delta;
        collision_result result;
        real length;

        vector3d_randomize_direction((real_point3d *)(marker + 0x3c), &direction, &effect_random_seed, 0.0f,
            0.2617994f);
        length = (i < hover_count ? ((struct vehicle_object *)obj)->vehicle.ground_lean : ((struct vehicle_object *)obj)->vehicle.ground_contact_fraction) * 6.0f + 2.0f;
        delta.i = direction.i * length;
        delta.j = direction.j * length;
        delta.k = direction.k * length;
        if (collision_test_movement_segment(0x61, (real_point3d *)(marker + 0x60), &delta, unit_index, &result)) {
            real_point3d points[3];
            real_vector3d vectors[3];
            real twice_dot;
            float scale;

            points[0] = result.point;
            points[1] = result.point;
            points[2] = result.point;
            vectors[0].i = -direction.i;
            vectors[0].j = -direction.j;
            vectors[0].k = -direction.k;
            vectors[1] = result.plane.normal;
            twice_dot = result.plane.normal.i * direction.i + result.plane.normal.j * direction.j +
                result.plane.normal.k * direction.k;
            twice_dot = twice_dot + twice_dot;
            vectors[2].i = direction.i - result.plane.normal.i * twice_dot;
            vectors[2].j = direction.j - result.plane.normal.j * twice_dot;
            vectors[2].k = direction.k - result.plane.normal.k * twice_dot;
            scale = 1.0f - result.t;
            effect_new_with_color(*(uint32_t *)(tag + 0x3ec), 0xffffffff, 0, 3, names, points, vectors,
                scale, scale, 0, 0, 1);
        }
    }
}

#if 0
Original Ghidra decompilation (0x574900):

void vehicle_create_hover_thruster_effects(uint param_1)

{
  float fVar1;
  uint *puVar2;
  int iVar3;
  char cVar4;
  short sVar5;
  short sVar6;
  int iVar7;
  int iVar8;
  float local_788;
  float local_784;
  float local_780;
  float local_770;
  float local_76c;
  float local_768;
  char *local_764;
  char *local_760;
  char *local_75c;
  undefined4 local_758;
  undefined4 local_754;
  undefined4 local_750;
  undefined4 local_74c;
  undefined4 local_748;
  undefined4 local_744;
  undefined4 local_740;
  undefined4 local_73c;
  undefined4 local_738;
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
  undefined4 local_6f8;
  undefined4 local_6f4;
  undefined4 local_6f0;
  float local_6ec;
  float local_6e8;
  float local_6e4;
  undefined1 local_6c0 [96];
  undefined1 auStack_660 [1632];

  puVar2 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  iVar3 = *(int *)((*puVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if (*(int *)(iVar3 + 0x3ec) != -1) {
    iVar7 = object_get_node_local_transform(param_1,"hover thrusters",local_6c0,0xf);
    sVar5 = (short)iVar7;
    sVar6 = object_get_node_local_transform
                      (param_1,"jet thrusters",local_6c0 + sVar5 * 0x6c,0x10 - iVar7);
    iVar7 = (int)sVar6 + (int)sVar5;
    sVar6 = 0;
    if (0 < iVar7) {
      iVar8 = 0;
      do {
        vector3d_randomize_direction(0,0x3e860a92);
        if (sVar6 < sVar5) {
          fVar1 = (float)puVar2[0x13b];
        }
        else {
          fVar1 = (float)puVar2[0x13c];
        }
        local_768 = fVar1 * 6.0 + 2.0;
        local_770 = local_788 * local_768;
        local_76c = local_784 * local_768;
        local_768 = local_780 * local_768;
        cVar4 = FUN_00505880(0x61,auStack_660 + iVar8 * 0x6c,&local_770,param_1,local_710);
        if (cVar4 != '\0') {
          local_734 = -local_788;
          local_730 = -local_784;
          local_72c = -local_780;
          local_758 = local_6f8;
          local_74c = local_6f8;
          local_740 = local_6f8;
          local_754 = local_6f4;
          local_748 = local_6f4;
          local_73c = local_6f4;
          local_750 = local_6f0;
          local_744 = local_6f0;
          local_738 = local_6f0;
          local_728 = local_6ec;
          fVar1 = local_6ec * local_788 + local_6e8 * local_784 + local_6e4 * local_780;
          local_724 = local_6e8;
          local_764 = "incident";
          fVar1 = fVar1 + fVar1;
          local_760 = "normal";
          local_720 = local_6e4;
          local_75c = "reflected";
          local_71c = local_788 - local_6ec * fVar1;
          local_718 = local_784 - local_6e8 * fVar1;
          local_714 = local_780 - local_6e4 * fVar1;
          FUN_00450980(*(undefined4 *)(iVar3 + 0x3ec),0xffffffff,0,3,&local_764,&local_758,
                       &local_734,1.0 - local_6fc,1.0 - local_6fc,0,0,1);
        }
        sVar6 = sVar6 + 1;
        iVar8 = (int)sVar6;
      } while (iVar8 < iVar7);
    }
  }
  return;
}
#endif
