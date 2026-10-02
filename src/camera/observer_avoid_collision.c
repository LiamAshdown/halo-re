// observer_avoid_collision  (Ghidra: FUN_00448d40; renamed for this rewrite)
// address 0x448d40, size 1053 bytes
// name confidence: 0.6   rewrite confidence: 0.7
// evidence: out/phase4/camera_types_notes.md's proposed name ("observer collision pushout");
// confirmed against objdump for the outer parameter/register layout (EAX -> forward, stack ->
// position, up, distance, radius_scale) and for FUN_005013a0 / scenario_location_get_water_and_weather's established
// signatures. The overall algorithm (probe up/down/left/right around a point pulled back from
// position along -forward, then bisect toward the nearest hit) is transcribed directly from
// Ghidra's own decompile, which is internally coherent even where this rewrite could not
// independently re-derive every register in the 10-iteration bisection from raw asm.
// register convention: forward in EAX (in_EAX); position, up, distance and radius_scale on the
// stack (Ghidra's recognized param_1..param_4).
//
// review (phase 4 gate, line by line against objdump 0x448d40..0x449164): the probe loop, the
// 10 step bisection and the final t selection match the binary, including the float compare
// directions (0x4490aa..0x44911f). Fixes made: bsp3d_node_find_leaf gets its real register
// arguments (EAX = 0 root node, ECX = the collision BSP at 0x00746f90, EDX = position) and the
// leaf index is masked with 0x7fffffff before indexing the 0x10 byte leaves (0x448d86); the
// leaf / cluster pair is the objects.h bsp_leaf_reference; scenario_location_get_water_and_weather takes that reference and
// a NULL int16 out pointer (its body writes a WORD through the second argument when non-NULL);
// and every collision_test_movement_segment call gets a full 0x50 byte collision_result (the
// frame reserves exactly that at esp+0x78), the earlier 0x18 byte scratch was overrun.
// UNSURE: the call to scenario_location_get_water_and_weather also loads EBX = position (0x448d9f); 0x53ed60 itself does
// not read EBX, its callees 0x53ec30 / 0x53ed10 were not checked.
// UNSURE: why the probe point is position pulled back along -forward by radius_scale + distance.
// reconciled: R05 0x00746f90 global_globals -> ModelCollisionGeometryBSP *global_collision_bsp (ScenarioStructureBSP +0xb4; global_globals is the matg globals at 0x00746fa0)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "projectiles.h"
#include "structures.h"
#include "camera.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern ModelCollisionGeometryBSP *global_collision_bsp; // 0x00746f90, the structure collision BSP (types/structures.h)
extern ScenarioStructureBSP *global_structure_bsp;       // 0x00746f9c

extern double fabs(double x); // FABS

// blam-cc: EAX -> node_index, ECX -> bsp, EDX -> point
extern uint32_t bsp3d_node_find_leaf(int32_t node_index, ModelCollisionGeometryBSP *bsp,
    real_point3d *point);                                        // 0x5013a0, physics module
extern uint8_t scenario_location_get_water_and_weather(real_point3d *point, bsp_leaf_reference *leaf,
    int16_t *weather_index_out); // 0x53ed60, EBX point, stack (leaf, weather_index_out)
extern uint8_t collision_test_movement_segment(uint32_t flags, real_point3d *origin,
    real_vector3d *delta, uint32_t exclude_object_index, collision_result *result); // 0x505880, physics module

// blam-cc: EAX -> origin, DL -> use_alternate_mask, ESI -> target; stack -> out_fraction
// (this module)
extern uint8_t observer_collision_test_ray(real_point3d *origin, uint8_t use_alternate_mask,
    real_point3d *target, float *out_fraction);

// blam-cc: EAX -> forward; position, up, distance, radius_scale = stack
// Shrinks *distance to keep the third person camera out of nearby geometry: probes up/down/
// left/right around a point pulled back from position along -forward, and if any probe hits
// something, bisects toward the collision boundary and blends *distance towards it.
void observer_avoid_collision(real_vector3d *forward, real_point3d *position, real_vector3d *up,
    float *distance, float radius_scale)
{
    bsp_leaf_reference location;
    uint8_t use_alternate_mask;
    float probe_length;
    real_point3d pullback_point;
    float unobstructed_fraction; // local_98
    float best_fraction;         // local_90
    float offsets[6];            // [0..2] = up * radius, [3..5] = cross(up, forward) * radius
    int32_t winning_group;       // -1 if no probe hit; else 0 (up axis) or 1 (right axis)
    float winning_sign;          // local_a8 while scanning
    int32_t k;
    collision_result collision;

    unobstructed_fraction = 1.0f;
    location.leaf_index = (int32_t)bsp3d_node_find_leaf(0, global_collision_bsp, position);
    if (location.leaf_index == -1) {
        location.cluster_index = -1;
    } else {
        location.cluster_index = (int16_t)((ScenarioStructureBSPLeaf *)global_structure_bsp->leaves.pointer)
            [location.leaf_index & 0x7fffffff].cluster;
    }
    use_alternate_mask = scenario_location_get_water_and_weather(position, &location, 0); // 0x448d98: EBX = ebp = position

    probe_length = radius_scale + *distance;
    pullback_point.x = position->x - probe_length * forward->i;
    pullback_point.y = position->y - probe_length * forward->j;
    pullback_point.z = position->z - probe_length * forward->k;

    observer_collision_test_ray(position, use_alternate_mask, &pullback_point,
        &unobstructed_fraction);

    {
        float scale = *distance * 0.174f;

        offsets[0] = up->i * scale;
        offsets[1] = up->j * scale;
        offsets[2] = up->k * scale;
        offsets[3] = (up->j * forward->k - forward->j * up->k) * scale;
        offsets[4] = (forward->i * up->k - up->i * forward->k) * scale;
        offsets[5] = (forward->j * up->i - up->j * forward->i) * scale;
    }

    best_fraction = unobstructed_fraction;
    winning_group = -1;
    winning_sign = 0.0f;

    for (k = 0; k < 4; k++) {
        float sign = (k & 2) ? 1.0f : -1.0f;
        int32_t group = k & 1;
        float *offset = &offsets[group * 3];
        real_point3d probe;
        uint32_t mask;
        uint8_t hit;
        float hit_fraction;

        probe.x = sign * offset[0] + pullback_point.x;
        probe.y = sign * offset[1] + pullback_point.y;
        probe.z = sign * offset[2] + pullback_point.z;

        mask = use_alternate_mask ? 0x40a1 : 0x40e1;
        {
            real_vector3d delta;

            delta.i = probe.x - position->x;
            delta.j = probe.y - position->y;
            delta.k = probe.z - position->z;
            hit = collision_test_movement_segment(mask, position, &delta, 0xffffffff, &collision);
        }
        if (hit) {
            hit_fraction = collision.t;
            if (hit_fraction < best_fraction) {
                best_fraction = hit_fraction;
                winning_group = group;
                winning_sign = sign;
            }
        }
    }

    if (winning_group == -1) {
        *distance = unobstructed_fraction * *distance;
        return;
    }

    {
        float *offset = &offsets[winning_group * 3];
        float clear_t = 0.0f;                       // local_ac, as a float bisection bound
        float blocked_t = winning_sign;              // local_b8
        float last_clear_result = unobstructed_fraction; // local_94
        float last_blocked_fraction = best_fraction; // local_a8, reused from the scan above
        int32_t iterations_left = 10;
        uint8_t converged_this_time;
        float mid;
        float selector;

        do {
            real_point3d probe;
            real_vector3d delta;
            uint32_t mask;
            uint8_t hit;
            float hit_fraction;

            converged_this_time = 0;
            mid = (blocked_t + clear_t) * 0.5f;

            probe.x = mid * offset[0] + pullback_point.x;
            probe.y = mid * offset[1] + pullback_point.y;
            probe.z = mid * offset[2] + pullback_point.z;
            mask = use_alternate_mask ? 0x40a1 : 0x40e1;
            delta.i = probe.x - position->x;
            delta.j = probe.y - position->y;
            delta.k = probe.z - position->z;
            hit = collision_test_movement_segment(mask, position, &delta, 0xffffffff, &collision);

            if (!hit) {
mark_clear:
                clear_t = mid;
                last_clear_result = converged_this_time ? hit_fraction : 1.0f;
            } else {
                hit_fraction = collision.t;
                converged_this_time = 1;
                if (0.1 <= fabs((double)(hit_fraction - last_blocked_fraction))) {
                    goto mark_clear;
                }
                last_blocked_fraction = hit_fraction;
                blocked_t = mid;
            }
            iterations_left--;
        } while (iterations_left != 0);

        selector = clear_t;
        if (last_blocked_fraction <= last_clear_result) {
            selector = (0.0f <= blocked_t) ? 1.0f : 0.0f;
        }
        if (selector == 0.0f) {
            if (last_clear_result < last_blocked_fraction) {
                blocked_t = clear_t;
            }
            blocked_t = -blocked_t;
        } else if (last_clear_result < last_blocked_fraction) {
            blocked_t = clear_t;
        }

        *distance = (blocked_t * unobstructed_fraction + (1.0f - blocked_t) * best_fraction) *
            *distance;
    }
}

#if 0
Original Ghidra decompilation (0x448d40):

void FUN_00448d40(float *param_1,float *param_2,float *param_3,float param_4)

{
  float fVar1;
  float *pfVar2;
  bool bVar3;
  char cVar4;
  char cVar5;
  undefined2 uVar6;
  float *in_EAX;
  uint uVar7;
  undefined4 uVar8;
  ushort uVar9;
  uint uVar10;
  int iVar11;
  float local_b8;
  uint local_b0;
  float *local_ac;
  float local_a8;
  float local_a4;
  float local_a0;
  float local_9c;
  float local_98;
  float local_94;
  float local_90;
  float local_8c;
  float local_88;
  float local_84;
  float local_80 [4];
  float local_70;
  float local_6c;
  float local_68;
  float local_64;
  float local_60;
  float local_54;
  undefined1 local_50 [20];
  float local_3c;

  local_98 = 1.0;
  local_a4 = (float)FUN_005013a0();
  if (local_a4 == -NAN) {
    uVar6 = 0xffff;
  }
  else {
    uVar6 = *(undefined2 *)((int)local_a4 * 0x10 + 8 + *(int *)(DAT_00746f9c + 0xe4));
  }
  local_a0 = (float)CONCAT22(local_a0._2_2_,uVar6);
  cVar4 = FUN_0053ed60(&local_a4,0);
  param_4 = param_4 + *param_3;
  local_a0 = -(param_4 * in_EAX[1]);
  local_9c = -(param_4 * in_EAX[2]);
  local_8c = -(param_4 * *in_EAX) + *param_1;
  local_88 = local_a0 + param_1[1];
  local_84 = local_9c + param_1[2];
  FUN_00449170(&local_98);
  fVar1 = *param_3 * 0.174;
  local_90 = local_98;
  local_ac = (float *)0x0;
  local_80[0] = *param_2 * fVar1;
  local_80[1] = param_2[1] * fVar1;
  local_80[2] = param_2[2] * fVar1;
  uVar9 = 0;
  uVar10 = 0;
  local_80[3] = (param_2[1] * in_EAX[2] - in_EAX[1] * param_2[2]) * fVar1;
  local_70 = (*in_EAX * param_2[2] - *param_2 * in_EAX[2]) * fVar1;
  local_6c = (in_EAX[1] * *param_2 - param_2[1] * *in_EAX) * fVar1;
  do {
    fVar1 = (float)(int)((-(uint)((uVar9 & 2) != 0) & 2) - 1);
    uVar7 = uVar10 & 1;
    uVar8 = 0x40e1;
    local_60 = fVar1 * local_80[uVar7 * 3 + 2] + local_84;
    if (cVar4 != '\0') {
      uVar8 = 0x40a1;
    }
    local_a4 = (fVar1 * local_80[uVar7 * 3] + local_8c) - *param_1;
    local_a0 = (fVar1 * local_80[uVar7 * 3 + 1] + local_88) - param_1[1];
    local_9c = local_60 - param_1[2];
    cVar5 = FUN_00505880(uVar8,param_1,&local_a4,0xffffffff,local_50);
    if ((cVar5 != '\0') && (local_3c < local_90)) {
      local_90 = local_3c;
      local_ac = local_80 + uVar7 * 3;
      local_a8 = fVar1;
    }
    pfVar2 = local_ac;
    uVar9 = uVar9 + 1;
    uVar10 = uVar10 + 1;
  } while ((short)uVar9 < 4);
  if (local_ac == (float *)0x0) {
    *param_3 = local_98 * *param_3;
    return;
  }
  local_ac = (float *)0x0;
  local_b8 = local_a8;
  local_94 = local_98;
  local_a8 = local_90;
  iVar11 = 10;
  do {
    bVar3 = false;
    uVar8 = 0x40e1;
    fVar1 = (local_b8 + (float)local_ac) * 0.5;
    local_54 = fVar1 * pfVar2[2] + local_84;
    if (cVar4 != '\0') {
      uVar8 = 0x40a1;
    }
    local_68 = (fVar1 * *pfVar2 + local_8c) - *param_1;
    local_64 = (fVar1 * pfVar2[1] + local_88) - param_1[1];
    local_60 = local_54 - param_1[2];
    cVar5 = FUN_00505880(uVar8,param_1,&local_68,0xffffffff,local_50);
    if (cVar5 == '\0') {
LAB_00449085:
      local_ac = (float *)fVar1;
      if (bVar3) {
        local_94 = local_a4;
      }
      else {
        local_94 = 1.0;
      }
    }
    else {
      local_a4 = local_3c;
      bVar3 = true;
      if (0.1 <= ABS(local_3c - local_a8)) goto LAB_00449085;
      local_a8 = local_3c;
      local_b8 = fVar1;
    }
    iVar11 = iVar11 + -1;
    if (iVar11 == 0) {
      pfVar2 = local_ac;
      if (local_a8 <= local_94) {
        local_b0 = (uint)(0.0 <= local_b8);
        pfVar2 = (float *)(float)local_b0;
      }
      if ((float)pfVar2 == 0.0) {
        if (local_94 < local_a8) {
          local_b8 = (float)local_ac;
        }
        local_b8 = -local_b8;
      }
      else if (local_94 < local_a8) {
        local_b8 = (float)local_ac;
      }
      *param_3 = (local_b8 * local_98 + (1.0 - local_b8) * local_90) * *param_3;
      return;
    }
  } while( true );
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
