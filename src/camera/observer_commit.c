// observer_commit  (Ghidra: FUN_00448900; renamed for this rewrite)
// address 0x448900, size 1074 bytes
// name confidence: 0.6   rewrite confidence: 0.8
// evidence: out/phase4/camera_types_notes.md's proposed name and its own field-by-field citation
// of this address for observer_camera (position/velocity/forward/up/fov) and for the leaf/cluster
// dword store at observer_camera+0x10; every offset here (parameters, velocity, observers_camera)
// matches types/camera.h exactly. FUN_005013a0's args match its established convention.
// register convention: local player index in AX (in_AX); no other parameters.
//
// review (phase 4 gate, line by line against objdump 0x448900..0x448d31): the clamps, the
// horizontal forward frame, the collision call (EAX = &forward, stack = &position, &up,
// &distance, 0.02), the water nudge and the published fields all match. Fixes made:
//   - bsp3d_node_find_leaf(EAX = 0, ECX = collision BSP 0x00746f90, EDX = &camera->position);
//     the leaf index is masked with 0x7fffffff before indexing the leaves (0x448b8c)
//   - scenario_location_water_surface_distance takes EAX = &camera->leaf_index (a bsp_leaf_reference, 0x448bd3) and
//     EDI = &camera->position (0x448b43, dotted with the water plane inside 0x53ee00); it was
//     declared without arguments
//   - the cluster change primes ScenarioStructureBSPCluster +0x28 predicted_resources
//     (0x448bae..0x448bbe, ESI), which confirms the earlier guess
//   - observers[i].camera replaces the separate observers_camera alias of 0x006ac6d0
// Faithful oddity: the cluster store at 0x448bcd is a dword move from a stack slot whose high
// half is stale, so observer_camera.unknown_12 receives garbage; modelled as a WORD store.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "structures.h"
#include "camera.h"

extern observer observers[1];               // 0x006ac65c
extern ModelCollisionGeometryBSP *global_globals; // 0x00746f90, the structure collision BSP (types/structures.h)
extern ScenarioStructureBSP *structure_bsp;       // 0x00746f9c

extern double sqrt(double x); // FSQRT
extern double fabs(double x); // FABS

// blam-cc: EAX -> node_index, ECX -> bsp, EDX -> point
extern uint32_t bsp3d_node_find_leaf(int32_t node_index, ModelCollisionGeometryBSP *bsp,
    real_point3d *point);                                              // 0x5013a0, physics module
// blam-cc: EAX -> location, EDI -> point; result on the x87 stack. Signed depth of the point
// below the water plane of the location's cluster, FLT_MAX / a constant when there is none.
extern float scenario_location_water_surface_distance(bsp_leaf_reference *location, real_point3d *point); // 0x53ee00, structures module
// blam-cc: ESI -> resources
extern void predicted_resource_list_touch(TagReflexive *resources);           // 0x4449f0, cache module

// blam-cc: forward in EAX; position, up, distance, radius_scale = stack (this module)
extern void observer_avoid_collision(real_vector3d *forward, real_point3d *position,
    real_vector3d *up, float *distance, float radius_scale);

// blam-cc: AX -> local_player_index
// Publishes observers[local_player_index]'s eased parameters to observers_camera[local_player_index]:
// clamps distance/fov/position, offsets the focus point by focus_offset (rotated into the
// horizontal forward frame) minus distance*forward, pushes the result out of nearby geometry
// unless the command forbids it, resolves the new leaf/cluster (priming that cluster's predicted
// resources on a change), nudges the position out of shallow water, clamps again, and copies
// velocity/forward/up/fov into observers_camera.
void observer_commit(int16_t local_player_index)
{
    observer *o = &observers[local_player_index];
    observer_camera *camera = &observers[local_player_index].camera;
    real_point3d position;
    float distance;
    float fov;
    float forward_i, forward_j;
    float horizontal_magnitude;
    int32_t leaf_index;
    float water_depth;

    position = *(real_point3d *)&o->parameters.position;

    if (0.0f <= o->parameters.distance) {
        distance = (o->parameters.distance <= 3.4028235e+38f) ? o->parameters.distance :
            3.4028235e+38f;
    } else {
        distance = 0.0f;
    }

    if (0.001 <= o->parameters.field_of_view) {
        fov = (o->parameters.field_of_view <= 1.5707964f) ? o->parameters.field_of_view :
            1.5707964f;
    } else {
        fov = 0.001f;
    }
    o->parameters.field_of_view = fov;

    if (position.x < -5000.0f) position.x = -5000.0f;
    else if (5000.0f < position.x) position.x = 5000.0f;
    if (position.y < -5000.0f) position.y = -5000.0f;
    else if (5000.0f < position.y) position.y = 5000.0f;
    if (position.z < -5000.0f) position.z = -5000.0f;
    else if (5000.0f < position.z) position.z = 5000.0f;
    if (distance < 0.0f) distance = 0.0f;
    else if (5000.0f < distance) distance = 5000.0f;

    forward_i = o->parameters.forward.i;
    forward_j = o->parameters.forward.j;
    horizontal_magnitude = (float)sqrt((double)(forward_i * forward_i + forward_j * forward_j));
    if (0.0001 <= fabs((double)horizontal_magnitude)) {
        float inv = 1.0f / horizontal_magnitude;
        forward_i = inv * forward_i;
        forward_j = inv * forward_j;
    }

    position.x = forward_i * o->parameters.focus_offset.i + forward_j * o->parameters.focus_offset.j
        + position.x;
    position.y = (forward_j * o->parameters.focus_offset.i - forward_i * o->parameters.focus_offset.j)
        + position.y;
    position.z = position.z + o->parameters.focus_offset.k;

    if ((o->current_command.flags & _observer_command_no_collision_bit) == 0 && distance != 0.0f) {
        observer_avoid_collision((real_vector3d *)&o->parameters.forward, &position,
            (real_vector3d *)&o->parameters.up, &distance, 0.02f);
    }

    camera->position.x = position.x - distance * o->parameters.forward.i;
    camera->position.y = position.y - distance * o->parameters.forward.j;
    camera->position.z = position.z - distance * o->parameters.forward.k;

    leaf_index = (int32_t)bsp3d_node_find_leaf(0, global_globals, (real_point3d *)&camera->position);
    if (leaf_index != -1) {
        int16_t new_cluster =
            ((ScenarioStructureBSPLeaf *)structure_bsp->leaves.pointer)[leaf_index & 0x7fffffff].cluster;

        if (new_cluster != -1) {
            if (new_cluster != camera->cluster_index) {
                predicted_resource_list_touch(
                    &((ScenarioStructureBSPCluster *)structure_bsp->clusters.pointer)[new_cluster]
                        .predicted_resources);
            }
            camera->leaf_index = leaf_index;
            camera->cluster_index = new_cluster;
        }
    }

    water_depth = scenario_location_water_surface_distance((bsp_leaf_reference *)&camera->leaf_index, (real_point3d *)&camera->position);
    if (fabs((double)water_depth) < 0.05000000074505806) {
        if (water_depth <= 0.0f) {
            camera->position.z = water_depth + camera->position.z + 0.05f;
        } else {
            camera->position.z = camera->position.z - (0.05f - water_depth);
        }
    }

    if (camera->position.x < -5000.0f) camera->position.x = -5000.0f;
    else if (5000.0f < camera->position.x) camera->position.x = 5000.0f;
    if (camera->position.y < -5000.0f) camera->position.y = -5000.0f;
    else if (5000.0f < camera->position.y) camera->position.y = 5000.0f;
    if (camera->position.z < -5000.0f) camera->position.z = -5000.0f;
    else if (5000.0f < camera->position.z) camera->position.z = 5000.0f;

    camera->velocity.i = -o->velocity.position.i;
    camera->velocity.j = -o->velocity.position.j;
    camera->velocity.k = -o->velocity.position.k;
    camera->forward = o->parameters.forward;
    camera->up = o->parameters.up;
    camera->field_of_view = fov;
}

#if 0
Original Ghidra decompilation (0x448900):

void FUN_00448900(void)

{
  float *pfVar1;
  float fVar2;
  float fVar3;
  undefined4 uVar4;
  short sVar5;
  undefined4 uVar6;
  float fVar7;
  short in_AX;
  int iVar8;
  int iVar9;
  int iVar10;
  float10 fVar11;
  float local_14;
  undefined4 local_10;
  float local_c;
  float local_8;
  float local_4;

  iVar9 = (int)in_AX;
  iVar10 = iVar9 * 0x29c;
  local_c = *(float *)(&DAT_006ac70c + iVar10);
  local_8 = *(float *)(&DAT_006ac710 + iVar10);
  local_4 = *(float *)(&DAT_006ac714 + iVar10);
  if (0.0 <= *(float *)(&DAT_006ac724 + iVar10)) {
    if (*(float *)(&DAT_006ac724 + iVar10) <= 3.4028235e+38) {
      local_14 = *(float *)(&DAT_006ac724 + iVar10);
    }
    else {
      local_14 = 3.4028235e+38;
    }
  }
  else {
    local_14 = 0.0;
  }
  if (0.001 <= *(float *)(&DAT_006ac728 + iVar10)) {
    if (*(float *)(&DAT_006ac728 + iVar10) <= 1.5707964) {
      uVar4 = *(undefined4 *)(&DAT_006ac728 + iVar10);
    }
    else {
      uVar4 = 0x3fc90fdb;
    }
  }
  else {
    uVar4 = 0x3a83126f;
  }
  *(undefined4 *)(&DAT_006ac728 + iVar10) = uVar4;
  if (-5000.0 <= local_c) {
    if (5000.0 < local_c) {
      local_c = 5000.0;
    }
  }
  else {
    local_c = -5000.0;
  }
  if (-5000.0 <= local_8) {
    if (5000.0 < local_8) {
      local_8 = 5000.0;
    }
  }
  else {
    local_8 = -5000.0;
  }
  if (-5000.0 <= local_4) {
    if (5000.0 < local_4) {
      local_4 = 5000.0;
    }
  }
  else {
    local_4 = -5000.0;
  }
  if (0.0 <= local_14) {
    if (5000.0 < local_14) {
      local_14 = 5000.0;
    }
  }
  else {
    local_14 = 0.0;
  }
  fVar2 = *(float *)(&DAT_006ac72c + iVar10);
  fVar3 = *(float *)(&DAT_006ac730 + iVar10);
  fVar7 = SQRT(fVar2 * fVar2 + fVar3 * fVar3);
  if (0.0001 <= ABS(fVar7)) {
    fVar7 = 1.0 / fVar7;
    fVar2 = fVar7 * fVar2;
    fVar3 = fVar7 * fVar3;
  }
  local_c = fVar2 * *(float *)(&DAT_006ac718 + iVar10) + fVar3 * *(float *)(&DAT_006ac71c + iVar10)
            + local_c;
  local_8 = (fVar3 * *(float *)(&DAT_006ac718 + iVar10) - fVar2 * *(float *)(&DAT_006ac71c + iVar10)
            ) + local_8;
  local_4 = local_4 + *(float *)(&DAT_006ac720 + iVar10);
  if ((((&DAT_006ac664)[iVar10] & 0x10) == 0) && (local_14 != 0.0)) {
    FUN_00448d40(&local_c,&DAT_006ac738 + iVar10,&local_14,0x3ca3d70a);
  }
  pfVar1 = (float *)(&DAT_006ac6d0 + iVar9 * 0xa7);
  *pfVar1 = local_c - local_14 * *(float *)(&DAT_006ac72c + iVar10);
  (&DAT_006ac6d4)[iVar9 * 0xa7] = local_8 - local_14 * *(float *)(&DAT_006ac730 + iVar10);
  (&DAT_006ac6d8)[iVar9 * 0xa7] = local_4 - local_14 * *(float *)(&DAT_006ac734 + iVar10);
  iVar8 = FUN_005013a0();
  if (iVar8 != -1) {
    sVar5 = *(short *)(iVar8 * 0x10 + 8 + *(int *)(DAT_00746f9c + 0xe4));
    local_10 = CONCAT22(local_10._2_2_,sVar5);
    if (sVar5 != -1) {
      if (sVar5 != *(short *)(&DAT_006ac6e0 + iVar9 * 0xa7)) {
        predicted_resource_list_touch();
      }
      *(int *)(&DAT_006ac6dc + iVar10) = iVar8;
      (&DAT_006ac6e0)[iVar9 * 0xa7] = local_10;
    }
  }
  fVar11 = (float10)FUN_0053ee00();
  if (ABS(fVar11) < (float10)0.05000000074505806) {
    if (fVar11 <= (float10)0.0) {
      (&DAT_006ac6d8)[iVar9 * 0xa7] =
           (float)(fVar11 + (float10)(float)(&DAT_006ac6d8)[iVar9 * 0xa7] + (float10)0.05);
    }
    else {
      (&DAT_006ac6d8)[iVar9 * 0xa7] =
           (float)((float10)(float)(&DAT_006ac6d8)[iVar9 * 0xa7] - ((float10)0.05 - fVar11));
    }
  }
  if (-5000.0 <= *pfVar1) {
    if (*pfVar1 <= 5000.0) {
      fVar2 = *pfVar1;
    }
    else {
      fVar2 = 5000.0;
    }
  }
  else {
    fVar2 = -5000.0;
  }
  *pfVar1 = fVar2;
  if (-5000.0 <= (float)(&DAT_006ac6d4)[iVar9 * 0xa7]) {
    if ((float)(&DAT_006ac6d4)[iVar9 * 0xa7] <= 5000.0) {
      uVar4 = (&DAT_006ac6d4)[iVar9 * 0xa7];
    }
    else {
      uVar4 = 0x459c4000;
    }
  }
  else {
    uVar4 = 0xc59c4000;
  }
  (&DAT_006ac6d4)[iVar9 * 0xa7] = uVar4;
  if (-5000.0 <= (float)(&DAT_006ac6d8)[iVar9 * 0xa7]) {
    if ((float)(&DAT_006ac6d8)[iVar9 * 0xa7] <= 5000.0) {
      uVar4 = (&DAT_006ac6d8)[iVar9 * 0xa7];
    }
    else {
      uVar4 = 0x459c4000;
    }
  }
  else {
    uVar4 = 0xc59c4000;
  }
  uVar6 = *(undefined4 *)(&DAT_006ac72c + iVar10);
  (&DAT_006ac6d8)[iVar9 * 0xa7] = uVar4;
  *(undefined4 *)(&DAT_006ac6f0 + iVar10) = uVar6;
  *(float *)(&DAT_006ac6e4 + iVar10) = -*(float *)(&DAT_006ac744 + iVar10);
  *(undefined4 *)(&DAT_006ac6f4 + iVar10) = *(undefined4 *)(&DAT_006ac730 + iVar10);
  *(undefined4 *)(&DAT_006ac6f8 + iVar10) = *(undefined4 *)(&DAT_006ac734 + iVar10);
  *(float *)(&DAT_006ac6e8 + iVar10) = -*(float *)(&DAT_006ac748 + iVar10);
  *(float *)(&DAT_006ac6ec + iVar10) = -*(float *)(&DAT_006ac74c + iVar10);
  *(undefined4 *)(&DAT_006ac6fc + iVar10) = *(undefined4 *)(&DAT_006ac738 + iVar10);
  *(undefined4 *)(&DAT_006ac700 + iVar10) = *(undefined4 *)(&DAT_006ac73c + iVar10);
  *(undefined4 *)(&DAT_006ac704 + iVar10) = *(undefined4 *)(&DAT_006ac740 + iVar10);
  *(undefined4 *)(&DAT_006ac708 + iVar10) = *(undefined4 *)(&DAT_006ac728 + iVar10);
  return;
}
#endif
