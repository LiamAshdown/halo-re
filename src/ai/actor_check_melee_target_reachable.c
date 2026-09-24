// actor_check_melee_target_reachable  (Ghidra: actor_check_melee_target_reachable, renamed)
// address 0x403f00, size 717 bytes
// name confidence: 0.4   rewrite confidence: 0.2
// evidence: types/ai.h actor.actor_definition_tag/unit_index; types/tags.h
//   Actor.pathfinding_radius (0x8c)/max_seek_cover_distance (0x320, already-named fields);
//   types/ai.h path_find_context (0x1008c bytes, confirmed here: the zero loop clears
//   exactly 0x4023 dwords) and its "leading 0x48 bytes are the caller's request block"
//   note (confirmed here too: the request fields are zeroed as exactly 0x12 dwords before
//   path_find_run runs); phase-4 summary "computes a path to the actor's melee target and
//   validates whether the melee attack is currently reachable".
//
// Given the size and the very deep, largely unnamed stack layout Ghidra produced (a request
// block whose exact field-by-field byte offsets could not be independently confirmed here),
// this keeps close to the decompiled locals (by name) rather than asserting a byte-precise
// struct for the request block. See UNSURE notes below.
// UNSURE, broadly:
//  - `order` (unaff_EBX) is the same family of order/aim record seen elsewhere in this
//    session (e.g. actor_build_order_grenade_or_melee.c calls this function against such a
//    record); its offsets are used as raw shorts/bytes, not backed by a shared struct.
//  - The internal path-find request block's exact field layout (radius, goal point, unit
//    index, and a handful of flags) is transcribed by write order rather than confirmed byte
//    offsets; actor_get_firing_position_group_mask, actor_find_best_firing_position, actor_claim_firing_position, path_find_find_unobstructed_ancestor and path_find_run are
//    all outside this session's range.
//  - prop+0xf0/0xf4/0xf8/0xec (a target's aim-offset-adjacent floats) and prop+0x110
//    (relationship_object_index, already named) are read as a point; the exact split
//    between prop.unknown_ec/unknown_f8 noted elsewhere applies here too.
// reconciled: R06 0x00746f9c is ScenarioStructureBSP *global_structure_bsp (was extern int32_t bsp_generation); ai.h path_find_context/actor_movement_context bsp_generation -> structure_bsp, bsp_index -> collision_bsp

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include <string.h>

extern data_array *actor_data; // 0x00880360
extern data_array *prop_data;  // 0x008802c0
extern tag_instance *tag_instances; // 0x0087bc14
extern ScenarioStructureBSP *global_structure_bsp; // 0x00746f9c, scenario.h (formerly bsp_generation)

extern uint32_t actor_get_firing_position_group_mask(); // SIGNATURE-CONFLICT: this call site and the rewrite of actor_get_firing_position_group_mask at 0x412880
                 // disagree on the argument list; Ghidra drops the register arguments
                 // here. Left unprototyped so the conflict is visible. See src/ai/README.md.
extern uint32_t actor_find_best_firing_position(); // SIGNATURE-CONFLICT: this call site and the rewrite of actor_find_best_firing_position at 0x412ba0
                 // disagree on the argument list; Ghidra drops the register arguments
                 // here. Left unprototyped so the conflict is visible. See src/ai/README.md.
extern int16_t actor_claim_firing_position(); // SIGNATURE-CONFLICT: this call site and the rewrite of actor_claim_firing_position at 0x414060
                 // disagree on the argument list; Ghidra drops the register arguments
                 // here. Left unprototyped so the conflict is visible. See src/ai/README.md.
extern void actor_target_get_relationship_object(datum_index target_prop_index); // 0x41f3a0, this module,
                                                 // blam-cc: EAX -> target_prop_index
extern uint8_t path_find_find_unobstructed_ancestor(void *request, uint8_t *flag, void *out); // 0x43a220, not yet rewritten
extern uint8_t path_find_run(path_find_context *context); // 0x43a8b0

// order is the caller's short-indexed order/aim record (see UNSURE above): order[0] a
// distance-like field, order[2] a byte, order+5 a byte, order[6] a maximum-distance-ish
// short gating "close melee" vs "ranged melee", order[4] the resolved waypoint, order+0x10 a
// success byte, order+0xe a target prop index (dword), order+0x12 an output record for
// path_find_find_unobstructed_ancestor, order+3 a byte cleared unconditionally at the end.
void actor_check_melee_target_reachable(uint32_t actor_index, int16_t *order)
{
    actor *a = &((actor *)actor_data->data)[actor_index & 0xffff];
    Actor *actor_def = (Actor *)tag_instances[a->actor_definition_tag & 0xffff].data;
    uint8_t request[0x48];
    uint32_t formation_selector[16];
    path_find_context context;
    uint8_t scratch_context[65684]; // UNSURE: see actor_update_path_if_needed.c
    uint32_t out_waypoint;
    uint8_t reached_exactly;
    int16_t waypoint;
    uint8_t success;

    memset(request, 0, sizeof(request));
    request[0x1b] = *((uint8_t *)order + 4); // UNSURE: uStack_2074f, written unconditionally before the branch below; slot within `request` guessed by write order
    if (order[6] < 1) {
        request[0x1c] = *((uint8_t *)order + 5);
        *(int16_t *)(request + 0) = 2; // "kind" style selector, by analogy with other requests
        {
            float radius = actor_def->max_seek_cover_distance;
            *(float *)(request + 0x14) = radius > 0.0f ? radius : 6.0f;
        }
    } else {
        *(int16_t *)(request + 0) = 1;
        if (*order > 0) {
            request[4] = 1;
            request[5] = 1;
        }
        request[0x1a] = 1;
        *(float *)(request + 0x18) = 10.0f;
        *(float *)(request + 0x1c) = 6.0f;
    }

    memset(formation_selector, 0, sizeof(formation_selector));
    formation_selector[0] = (uint32_t)actor_get_firing_position_group_mask(0);
    waypoint = actor_find_best_firing_position(actor_index, formation_selector, request, &out_waypoint, scratch_context, &reached_exactly);
    order[4] = waypoint;
    waypoint = actor_claim_firing_position(actor_index, out_waypoint, scratch_context);
    order[4] = waypoint;
    success = (waypoint != -1 && reached_exactly == 0);
    *((uint8_t *)order + 5) = success;
    *((uint8_t *)order + 0x10) = 0;

    if (waypoint != -1 && *(uint32_t *)(order + 0xe) != 0xffffffff) {
        datum_index target_prop_index = *(uint32_t *)(order + 0xe);
        prop *p = &((prop *)prop_data->data)[target_prop_index & 0xffff];

        if (p->kind > 1 && p->kind < 4) {
            // 0x40407a leaves EAX holding this same prop handle.
            actor_target_get_relationship_object(target_prop_index);
        }
        {
            int32_t anchor_object = (p->relationship_object_index != -1) ? p->relationship_object_index : (int32_t)p->object_index;
            uint8_t goal_request[0x48];

            memset(goal_request, 0, sizeof(goal_request));
            *(int32_t *)(goal_request + 4) = anchor_object;
            *(float *)(goal_request + 0) = actor_def->pathfinding_radius;
            *(float *)(goal_request + 0x14) = *(float *)((uint8_t *)p + 0xf4);
            *(float *)(goal_request + 0x18) = *(float *)((uint8_t *)p + 0xf0);
            *(float *)(goal_request + 0x10) = *(float *)((uint8_t *)p + 0xf8);
            *(int32_t *)(goal_request + 0x2c /* uStack_2080c */) = a->unit_index;
            *(float *)(goal_request + 0x24 /* uStack_207f8 */) = *(float *)((uint8_t *)p + 0xec);

            memset(&context, 0, sizeof(context));
            context.structure_bsp = (uint32_t)global_structure_bsp;
            goal_request[8] = 0;
            goal_request[0x10] = 1;
            memcpy(&context, goal_request, sizeof(goal_request));
            *(uint32_t *)((uint8_t *)&context + 0x48) = *(uint32_t *)request;
            *(uint32_t *)((uint8_t *)&context + 0x4c) = ((uint32_t *)request)[1];
            *(uint32_t *)((uint8_t *)&context + 0x50) = ((uint32_t *)request)[2];
            *(uint32_t *)((uint8_t *)&context + 0x54) = ((uint32_t *)request)[5];
            *(uint32_t *)((uint8_t *)&context + 0x58) = 0;
            *(uint32_t *)((uint8_t *)&context + 0x5c) = 1;
            *(uint32_t *)((uint8_t *)&context + 0x60) = 0;

            if (path_find_run(&context) != 0) {
                *((uint8_t *)order + 0x10) = path_find_find_unobstructed_ancestor(request, &reached_exactly, order + 0x12);
            }
        }
    }
    *((uint8_t *)order + 3) = 0;
}

#if 0
Original Ghidra decompilation (0x403f00):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

void FUN_00403f00(uint param_1)

{
  char cVar1;
  undefined1 uVar2;
  short sVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  short *unaff_EBX;
  undefined4 *puVar7;
  undefined4 *puVar8;
  char cStack_20825;
  int iStack_20824;
  int iStack_20820;
  undefined4 uStack_2081c;
  undefined4 uStack_20818;
  undefined1 uStack_20814;
  int iStack_20810;
  undefined4 uStack_2080c;
  undefined1 uStack_20808;
  undefined4 uStack_20804;
  undefined4 uStack_20800;
  undefined4 uStack_207fc;
  undefined4 uStack_207f8;
  undefined4 *apuStack_207cc [15];
  undefined4 uStack_20790;
  undefined2 uStack_2078c;
  undefined1 uStack_20788;
  undefined1 uStack_2077c;
  undefined1 uStack_2077b;
  undefined4 uStack_20774;
  undefined1 uStack_2075a;
  undefined4 uStack_20758;
  undefined4 uStack_20754;
  undefined1 uStack_2074f;
  undefined4 auStack_20128 [18];
  undefined4 uStack_200e0;
  undefined1 uStack_200dc;
  undefined4 uStack_200d8;
  undefined4 uStack_200d4;
  undefined4 uStack_200d0;
  undefined4 uStack_200cc;
  undefined4 uStack_200c8;
  undefined4 uStack_200c4;
  undefined1 auStack_10098 [65684];

  iStack_20820 = (param_1 & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  iStack_20824 = *(int *)((*(uint *)(iStack_20820 + 0x58) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  sVar3 = unaff_EBX[6];
  puVar7 = &uStack_20790;
  for (iVar4 = 0x199; iVar4 != 0; iVar4 = iVar4 + -1) {
    *puVar7 = 0;
    puVar7 = puVar7 + 1;
  }
  uStack_2074f = (undefined1)unaff_EBX[2];
  if (sVar3 < 1) {
    uStack_20788 = *(undefined1 *)((int)unaff_EBX + 5);
    uStack_2078c = 2;
    if (*(float *)(iStack_20824 + 800) <= 0.0) {
      uStack_20774 = 0x40c00000;
    }
    else {
      uStack_20774 = *(undefined4 *)(iStack_20824 + 800);
    }
  }
  else {
    uStack_2078c = 1;
    if (0 < *unaff_EBX) {
      uStack_2077c = 1;
      uStack_2077b = 1;
    }
    uStack_2075a = 1;
    uStack_20758 = 0x41200000;
    uStack_20754 = 0x40c00000;
  }
  uStack_20790 = FUN_00412880(0);
  sVar3 = FUN_00412ba0(param_1,&uStack_20790,apuStack_207cc,&uStack_2081c,auStack_10098,
                       &cStack_20825);
  unaff_EBX[4] = sVar3;
  sVar3 = FUN_00414060(param_1,uStack_2081c,auStack_10098);
  unaff_EBX[4] = sVar3;
  if ((sVar3 == -1) || (cStack_20825 != '\0')) {
    uVar2 = 0;
  }
  else {
    uVar2 = 1;
  }
  *(undefined1 *)(unaff_EBX + 5) = uVar2;
  *(undefined1 *)(unaff_EBX + 0x10) = 0;
  if ((sVar3 != -1) && (*(uint *)(unaff_EBX + 0xe) != 0xffffffff)) {
    iVar4 = (*(uint *)(unaff_EBX + 0xe) & 0xffff) * 0x138;
    sVar3 = *(short *)(iVar4 + 0x24 + *(int *)(DAT_008802c0 + 0x34));
    iVar4 = iVar4 + *(int *)(DAT_008802c0 + 0x34);
    if ((1 < sVar3) && (sVar3 < 4)) {
      actor_target_get_relationship_object();
    }
    iVar6 = *(int *)(iVar4 + 0x110);
    if (iVar6 == -1) {
      iVar6 = *(int *)(iVar4 + 0x18);
    }
    puVar7 = &uStack_20818;
    for (iVar5 = 0x12; iVar5 != 0; iVar5 = iVar5 + -1) {
      *puVar7 = 0;
      puVar7 = puVar7 + 1;
    }
    iStack_20810 = iVar6;
    uStack_20818 = *(undefined4 *)(iStack_20824 + 0x8c);
    uStack_20800 = *(undefined4 *)(iVar4 + 0xf4);
    uStack_20804 = *(undefined4 *)(iVar4 + 0xf0);
    uStack_207fc = *(undefined4 *)(iVar4 + 0xf8);
    uStack_2080c = *(undefined4 *)(iStack_20820 + 0x18);
    uStack_207f8 = *(undefined4 *)(iVar4 + 0xec);
    puVar7 = auStack_20128;
    for (iVar4 = 0x4023; iVar4 != 0; iVar4 = iVar4 + -1) {
      *puVar7 = 0;
      puVar7 = puVar7 + 1;
    }
    uStack_200c4 = DAT_00746f9c;
    uStack_20814 = 0;
    uStack_20808 = 1;
    puVar7 = &uStack_20818;
    puVar8 = auStack_20128;
    for (iVar4 = 0x12; iVar4 != 0; iVar4 = iVar4 + -1) {
      *puVar8 = *puVar7;
      puVar7 = puVar7 + 1;
      puVar8 = puVar8 + 1;
    }
    uStack_200d8 = *apuStack_207cc[0];
    uStack_200d4 = apuStack_207cc[0][1];
    uStack_200d0 = apuStack_207cc[0][2];
    uStack_200cc = apuStack_207cc[0][5];
    uStack_200e0 = 0;
    uStack_200dc = 1;
    uStack_200c8 = 0;
    cVar1 = path_find_run();
    if (cVar1 != '\0') {
      uVar2 = FUN_0043a220(apuStack_207cc[0],&cStack_20825,unaff_EBX + 0x12);
      *(undefined1 *)(unaff_EBX + 0x10) = uVar2;
    }
  }
  *(undefined1 *)(unaff_EBX + 3) = 0;
  return;
}
#endif
