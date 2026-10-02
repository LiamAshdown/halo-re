// physics_point_walk_toward_target  (Ghidra: FUN_005070d0; renamed)
// address 0x5070d0, size 143 bytes
// name confidence: 0.3   rewrite confidence: 0.95
// evidence: out/phase4/physics_functions.md summary ("Walks a physics point step-by-step toward
//   a target position while collision-testing each step, stopping as soon as it becomes
//   blocked"); object_collision_test_cluster_group's own signature (uint flags, undefined4) confirmed from its own pack
//   (out/phase2/physics/00.md) matching this function's forwarded param_1/param_3; cross-checked
//   against FUN_00507170 (this module, higher half), which forwards the identical value to
//   object_collision_test_cluster_group, this function and physics_model_build_from_sphere_query's own
//   exclude_object_index parameter, confirming the type as a plain datum_index rather than a
//   pointer.
// register convention: in_EAX -> state (a caller-owned {float t; real_point3d position;} pair;
//   position starts at the target and t at the full step distance), unaff_EBX -> start_position
//   (real_point3d *, the fallback origin). param_1/param_2/param_3 are Ghidra's own recognized
//   parameters (flags forwarded to object_collision_test_cluster_group, step direction, exclude_object_index also
//   forwarded to object_collision_test_cluster_group).
//   // blam-cc: EAX -> state, EBX -> start_position, stack -> flags, step_direction,
//   //          exclude_object_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "physics.h"

// physics_point_walk_state now lives in types/physics.h.

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint8_t object_collision_test_cluster_group(uint32_t flags, real_point3d *position, uint32_t exclude_object_index); // 0x505490, EDI position, stack

// Tests state->position (initially the desired target) against nearby objects via
// object_collision_test_cluster_group; while it is blocked and state->t > 0, backs it off by a fixed 0.03125 step along
// -step_direction (recomputed from start_position each time) and retests. Stops as soon as a
// clear position is found. If the whole step distance is consumed without ever clearing, snaps
// state->position back to exactly *start_position.
void physics_point_walk_toward_target(physics_point_walk_state *state, real_point3d *start_position,
    uint32_t flags, real_vector3d *step_direction, uint32_t exclude_object_index)
{
    if (0.0f < state->t) {
        do {
            // 0x5070e8: EDI = &state->position (the draft tested nothing in particular)
            if (!object_collision_test_cluster_group(flags, &state->position, exclude_object_index)) {
                break;
            }
            state->t -= 0.03125f;
            state->position.x = state->t * step_direction->i + start_position->x;
            state->position.y = state->t * step_direction->j + start_position->y;
            state->position.z = state->t * step_direction->k + start_position->z;
        } while (0.0f < state->t);
    }

    if (state->t <= 0.0f) {
        state->position = *start_position;
    }
}

#if 0
Original Ghidra decompilation (0x5070d0):

void FUN_005070d0(undefined4 param_1,float *param_2,undefined4 param_3)

{
  float fVar1;
  char cVar2;
  float *in_EAX;
  float *unaff_EBX;

  if (0.0 < *in_EAX) {
    do {
      cVar2 = FUN_00505490(param_1,param_3);
      if (cVar2 == '\0') break;
      fVar1 = *in_EAX - 0.03125;
      *in_EAX = fVar1;
      in_EAX[1] = fVar1 * *param_2 + *unaff_EBX;
      in_EAX[2] = fVar1 * param_2[1] + unaff_EBX[1];
      in_EAX[3] = fVar1 * param_2[2] + unaff_EBX[2];
    } while (0.0 < *in_EAX);
  }
  if (*in_EAX < 0.0 != (*in_EAX == 0.0)) {
    in_EAX[1] = *unaff_EBX;
    in_EAX[2] = unaff_EBX[1];
    in_EAX[3] = unaff_EBX[2];
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
