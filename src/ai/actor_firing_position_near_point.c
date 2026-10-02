// actor_firing_position_near_point  (Ghidra: actor_firing_position_near_point, renamed)
// address 0x412960, size 554 bytes
// name confidence: 0.55  rewrite confidence: 0.6
// evidence: it resolves a group mask with actor_get_firing_position_group_mask @0x412880,
//   walks ScenarioEncounter.firing_positions of the actor own encounter and keeps only the
//   positions whose group_index bit is in that mask and that sit within 4 world units of
//   the probe point; the 0x48-byte block it builds is the path_find_request types/ai.h
//   describes, and the 0x4023-dword zero is path_find_context.
// register convention: actor_index in EDX; the point, the start surface index and the mask
//   kind are the three Ghidra-recognized stack parameters.
//
// UNSURE: actor_get_firing_position_group_mask, path_find_run and 0x43a310 / 0x43a0a0 are
// all invoked with part of their arguments in registers that Ghidra dropped. The argument
// lists below are what the live values in this frame allow, not what the call sites show.
// reconciled: R06 0x00746f9c is ScenarioStructureBSP *global_structure_bsp (was extern int32_t bsp_generation); ai.h path_find_context/actor_movement_context bsp_generation -> structure_bsp, bsp_index -> collision_bsp

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "ai.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *actor_data;      // 0x00880360
extern tag_instance *tag_instances; // 0x0087bc14
extern Scenario *global_scenario;   // 0x00746f8c
extern ScenarioStructureBSP *global_structure_bsp; // 0x00746f9c, scenario.h (formerly bsp_generation)

extern uint32_t actor_get_firing_position_group_mask(datum_index actor_index, int16_t kind, int16_t search_override); // 0x412880, this module
extern uint8_t path_find_test_direct_reachability(const real_point3d *point_a, const real_point3d *point_b,
    real_point3d *out_position, void *context, uint8_t *out_success); // 0x43a0a0, EAX, ECX, ESI, stack
extern uint8_t path_find_compute_heuristic(path_find_context *context, uint32_t vertex_id, real_point3d *point,
    float *out_distance, float *out_secondary, real_vector3d *out_direction); // 0x43a310, EDI, EAX, stack
extern uint8_t path_find_run(path_find_context *context);       // 0x43a8b0, not yet rewritten

// blam-cc: EDX -> actor_index, stack -> point, start_surface_index, kind
// Answers whether the actor has a firing position of the requested class within four world
// units of the probe point that it can actually get to. Ground actors run one pathfind from
// the probe point first and then accept any matching position the path reaches within four
// units; flying actors skip the pathfind and use the straight-line test instead.
uint8_t actor_firing_position_near_point(datum_index actor_index, real_point3d *point,
                                         int32_t start_surface_index, int16_t kind)
{
    actor *self;
    Actor *actor_definition;
    ScenarioEncounter *encounter_definition;
    ScenarioFiringPosition *firing_positions;
    path_find_request request;
    path_find_context context;
    uint32_t group_mask;
    float path_distance;
    float dx, dy, dz;
    int32_t i;
    uint32_t *clear;
    int32_t n;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    actor_definition = (Actor *)tag_instances[self->actor_definition_tag & 0xffff].data;

    if ((self->flying == 0 && start_surface_index == -1) ||
        self->encounter_index == (datum_index)0xffffffff) {
        return 0;
    }

    encounter_definition = &((ScenarioEncounter *)global_scenario->encounters.pointer)
                                [self->encounter_index & 0xffff];
    group_mask = actor_get_firing_position_group_mask(actor_index, kind, 0);

    if (self->flying == 0) {
        clear = (uint32_t *)&request;
        for (n = 0; n < 0x12; n++) {
            clear[n] = 0;
        }
        request.pathfinding_radius = actor_definition->pathfinding_radius;
        request.ignores_glass = 1;
        request.exclude_object_index_a = (datum_index)0xffffffff;
        request.exclude_object_index_b = (datum_index)0xffffffff;
        request.have_start = 1;
        request.start_position.x = point->x;
        request.start_position.y = point->y;
        request.start_position.z = point->z;
        request.start_surface_index = (uint32_t)start_surface_index;
        request.have_limit = 1;
        request.limit_distance = 4.0f;

        clear = (uint32_t *)&context;
        for (n = 0; n < 0x4023; n++) {
            clear[n] = 0;
        }
        context.structure_bsp = (uint32_t)global_structure_bsp;
        for (n = 0; n < 0x12; n++) {
            ((uint32_t *)&context)[n] = ((uint32_t *)&request)[n];
        }
        context.obstacle_cache = 0;
        path_find_run(&context);
    }

    firing_positions = (ScenarioFiringPosition *)encounter_definition->firing_positions.pointer;
    for (i = 0; i < (int32_t)encounter_definition->firing_positions.count; i++) {
        ScenarioFiringPosition *fp = &firing_positions[i];
        if ((group_mask & (1u << (((uint8_t)fp->group_index) & 0x1f))) == 0) {
            continue;
        }
        dx = fp->position.x - point->x;
        dy = fp->position.y - point->y;
        dz = fp->position.z - point->z;
        if (dx * dx + dy * dy + dz * dz >= 16.0f) {
            continue;
        }
        if (self->flying == 0) {
            // 0x412b43: EAX = the firing position's surface (+0x14), EDI = the context
            path_find_compute_heuristic(&context, ((struct ScenarioFiringPosition *)fp)->surface_index, (real_point3d *)fp, &path_distance, 0, 0);
            if (path_distance < 4.0f) {
                return 1;
            }
        } else {
            // 0x412b24: EAX = the firing position, ECX = the point, ESI = no output, stack: the bsp, no flag
            if (path_find_test_direct_reachability((real_point3d *)fp, point, 0, global_structure_bsp, 0) != 0) {
                return 1;
            }
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x412960):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

undefined4 FUN_00412960(float *param_1,int param_2,undefined4 param_3)

{
  float *pfVar1;
  char cVar2;
  short sVar3;
  uint uVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  uint in_EDX;
  int iVar8;
  undefined4 *puVar9;
  undefined4 *puVar10;
  float fStack_100e4;
  undefined4 uStack_100e0;
  undefined1 uStack_100dc;
  undefined4 uStack_100d8;
  undefined4 uStack_100d4;
  undefined1 uStack_100d0;
  float fStack_100cc;
  float fStack_100c8;
  float fStack_100c4;
  int iStack_100c0;
  undefined1 uStack_100a0;
  undefined4 uStack_1009c;
  undefined4 auStack_10098 [18];
  undefined4 uStack_10050;
  undefined4 uStack_10034;

  iVar8 = (in_EDX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  iVar7 = *(int *)((*(uint *)(iVar8 + 0x58) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if (((*(char *)(iVar8 + 0x99) != '\0') || (param_2 != -1)) &&
     (*(uint *)(iVar8 + 0x34) != 0xffffffff)) {
    iVar5 = (*(uint *)(iVar8 + 0x34) & 0xffff) * 0xb0 + *(int *)(global_scenario + 0x430);
    uVar4 = FUN_00412880(param_3);
    if (*(char *)(iVar8 + 0x99) == '\0') {
      puVar9 = &uStack_100e0;
      for (iVar6 = 0x12; iVar6 != 0; iVar6 = iVar6 + -1) {
        *puVar9 = 0;
        puVar9 = puVar9 + 1;
      }
      uStack_100e0 = *(undefined4 *)(iVar7 + 0x8c);
      uStack_100d8 = 0xffffffff;
      uStack_100d4 = 0xffffffff;
      fStack_100c8 = param_1[1];
      fStack_100c4 = param_1[2];
      fStack_100cc = *param_1;
      puVar9 = auStack_10098;
      for (iVar7 = 0x4023; iVar7 != 0; iVar7 = iVar7 + -1) {
        *puVar9 = 0;
        puVar9 = puVar9 + 1;
      }
      uStack_10034 = DAT_00746f9c;
      iStack_100c0 = param_2;
      uStack_100dc = 1;
      uStack_100d0 = 1;
      uStack_100a0 = 1;
      uStack_1009c = 0x40800000;
      puVar9 = &uStack_100e0;
      puVar10 = auStack_10098;
      for (iVar7 = 0x12; iVar7 != 0; iVar7 = iVar7 + -1) {
        *puVar10 = *puVar9;
        puVar9 = puVar9 + 1;
        puVar10 = puVar10 + 1;
      }
      uStack_10050 = 0;
      path_find_run();
    }
    iVar7 = 0;
    sVar3 = 0;
    if (0 < *(int *)(iVar5 + 0x98)) {
      do {
        pfVar1 = (float *)(*(int *)(iVar5 + 0x9c) + iVar7 * 0x18);
        if (((uVar4 & 1 << (*(byte *)(pfVar1 + 3) & 0x1f)) != 0) &&
           ((*pfVar1 - *param_1) * (*pfVar1 - *param_1) +
            (pfVar1[1] - param_1[1]) * (pfVar1[1] - param_1[1]) +
            (pfVar1[2] - param_1[2]) * (pfVar1[2] - param_1[2]) < 16.0)) {
          if (*(char *)(iVar8 + 0x99) == '\0') {
            FUN_0043a310(pfVar1,&fStack_100e4,0,0);
            if (fStack_100e4 < 4.0) {
              return 1;
            }
          }
          else {
            cVar2 = FUN_0043a0a0(DAT_00746f9c);
            if (cVar2 != '\0') {
              return 1;
            }
          }
        }
        sVar3 = sVar3 + 1;
        iVar7 = (int)sVar3;
      } while (iVar7 < *(int *)(iVar5 + 0x98));
    }
  }
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
