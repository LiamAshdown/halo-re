// vehicle_create_hover_thruster_effects  (Ghidra: already named vehicle_create_hover_thruster_effects)
// address 0x574900, size 682 bytes
// name confidence: 0.55 (cea-pdb hints via the "hover thrusters"/"jet thrusters" strings)
// rewrite confidence: 0.2 -- the object_marker arrays (local_6c0/auStack_660) and
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

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name,
                                                void *marker, uint32_t flags); // 0x4f6080
extern real_vector3d *vector3d_randomize_direction(real_point3d *direction, real_vector3d *out,
    void *seed, real lo, real hi); // 0x4cd1b0, UNSURE: called here with 0 direction (register-only)
extern uint8_t collision_test_movement_segment(uint32_t mask, real_point3d *origin, real_vector3d *delta,
                             uint32_t exclude_object, void *scratch); // 0x505880
extern void effect_new_with_color(uint32_t effect, uint32_t param_2, void *param_3, int32_t kind,
                          char **labels, void *normal_block, void *incident_block,
                          float param_8, float param_9, int32_t param_10, int32_t param_11,
                          int32_t param_12); // 0x450980, this call site's variant (kind == 3)
                          // of the established 12-argument shape in
                          // src/hs/hs_effect_spawn_at_location.c; UNSURE how the two shapes
                          // reconcile into one real prototype.

// Spawns hover/jet-thruster exhaust visual effects at each "hover thrusters" and "jet
// thrusters" marker of a vehicle, raycasting downward from each and, on a hit, spawning a
// reflected damage-effect scaled by the unit's ground_lean/ground_contact_fraction fields.
// FIXED (register inputs, objdump; one stack argument remains, so no ordering question): the original never reads EAX; unit_index arrive(s) on the stack (1 stack argument(s)).
// blam-cc: stack -> unit_index
void vehicle_create_hover_thruster_effects(uint32_t unit_index)
{
    object *obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    Vehicle *tag = (Vehicle *)tag_instances[obj->definition_tag & 0xffff].data;
    vehicle_data *vehicle = (vehicle_data *)((uint8_t *)obj + k_unit_object_size);

    if (*(int32_t *)((uint8_t *)tag + 0x3ec) == -1) {
        return;
    }

    {
        uint8_t markers[1632 + 96]; // local_6c0 (96) + auStack_660 (1632), one contiguous buffer
        int16_t hover_count = (int16_t)object_get_node_local_transform(unit_index, "hover thrusters", markers, 0xf);
        int16_t jet_count = (int16_t)object_get_node_local_transform(unit_index, "jet thrusters",
                                                                      markers + hover_count * 0x6c,
                                                                      0x10 - hover_count);
        int32_t total = hover_count + jet_count;
        int32_t i;

        for (i = 0; i < total; i++) {
            real_point3d direction;
            real_vector3d scaled;
            uint8_t hit_scratch[20];
            float scale = (i < hover_count) ? vehicle->ground_lean : vehicle->ground_contact_fraction;

            vector3d_randomize_direction(0, (real_vector3d *)&direction, 0, 0.0f, 0.26179939f); // UNSURE args
            scale = scale * 6.0f + 2.0f;
            scaled.i = direction.x * scale;
            scaled.j = direction.y * scale;
            scaled.k = direction.z * scale;

            if (collision_test_movement_segment(0x61, (real_point3d *)(markers + i * 0x6c), &scaled, unit_index, hit_scratch) != 0) {
                real_vector3d incident, normal, reflected;
                char *labels[3] = { "incident", "normal", "reflected" };
                float dot;

                incident.i = -direction.x;
                incident.j = -direction.y;
                incident.k = -direction.z;

                normal.i = *(float *)(hit_scratch + 0x38); // UNSURE, see file header
                normal.j = *(float *)(hit_scratch + 0x34);
                normal.k = *(float *)(hit_scratch + 0x30);

                dot = *(float *)(hit_scratch + 0x2c) * direction.x + *(float *)(hit_scratch + 0x28) * direction.y +
                      *(float *)(hit_scratch + 0x24) * direction.z;
                dot += dot;
                reflected.i = direction.x - *(float *)(hit_scratch + 0x2c) * dot;
                reflected.j = direction.y - *(float *)(hit_scratch + 0x28) * dot;
                reflected.k = direction.z - *(float *)(hit_scratch + 0x24) * dot;

                {
                    // normal_block repeats "normal" three times and incident_block holds
                    // {incident, a second copy of normal, reflected}, matching the original's
                    // literal (redundant) field copies.
                    real_vector3d normal_block[3] = { normal, normal, normal };
                    real_vector3d incident_block[3] = { incident, normal, reflected };
                    float fraction = *(float *)(hit_scratch + 0);
                    effect_new_with_color(*(uint32_t *)((uint8_t *)tag + 0x3ec), 0xffffffff, 0, 3, labels,
                                 normal_block, incident_block, 1.0f - fraction, 1.0f - fraction, 0, 0, 1);
                }
            }
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
