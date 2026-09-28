// actor_check_melee_target_reachable  (Ghidra: actor_check_melee_target_reachable, renamed)
// address 0x403f00, size 717 bytes
// name confidence: 0.35   rewrite confidence: 0.85
// REWRITTEN from objdump 0x403f00..0x4041cc (the draft passed the firing-position helpers the wrong arguments).
//   Picks a firing position for an order (EBX, an actor_order): an order with a target (+0x0c > 0) searches goal
//   kind 1 (random fallback when +0x00 > 0, avoiding the target at radius 10 weight 6), otherwise goal kind 2 with
//   the order's +0x05 as the search mode and the actor tag's +0x320 (else 6) as the search radius; +0x04 asks for
//   the alternate aim point. The pick is claimed into +0x08, +0x0a notes a claim without a found path. With a claim
//   and a target prop (+0x1c) a path is run from the target (its surface +0xec/+0xf0, ignoring its relationship
//   object or object, the actor's unit too) to the claimed position, and the nearest unobstructed ancestor (0x43a220)
//   of the position fills +0x24 with +0x20 its result. +0x06 clears.
// blam-cc: EBX -> order, stack -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include <string.h>

extern data_array *actor_data;      // 0x00880360
extern data_array *prop_data;       // 0x008802c0
extern tag_instance *tag_instances; // 0x0087bc14
extern ScenarioStructureBSP *global_structure_bsp;

extern uint32_t actor_get_firing_position_group_mask(datum_index actor_index, int16_t kind, int16_t search_override); // 0x412880, EAX, SI, stack
extern uint32_t actor_find_best_firing_position(datum_index actor_index, actor_firing_position_query *query,
    actor_firing_position_candidate *out_candidate, uint32_t *out_previous_owner, path_find_context *path_context,
    uint8_t *out_path_ok); // 0x412ba0
extern int16_t actor_claim_firing_position(datum_index actor_index, datum_index previous_owner,
    path_find_context *path_context, int16_t firing_position_index, uint8_t path_ok); // 0x414060, stack, CX, AL
extern void actor_target_get_relationship_object(datum_index target_prop_index); // 0x41f3a0, EAX
extern uint8_t path_find_run(path_find_context *context); // 0x43a8b0, EAX
extern uint8_t path_find_find_unobstructed_ancestor(path_find_context *context, uint32_t vertex_id, real_point3d *point,
    uint8_t *out_used_start, real_point3d *out_position); // 0x43a220, ECX, EAX, stack

void actor_check_melee_target_reachable(uint32_t actor_index, int16_t *order)
{
    uint8_t *record = (uint8_t *)order;
    uint8_t *actor = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
    uint8_t *actor_tag = (uint8_t *)tag_instances[*(datum_index *)(actor + 0x58) & 0xffff].data;
    static actor_firing_position_query query;
    static path_find_context path_context;
    actor_firing_position_candidate candidate;
    uint32_t previous_owner = 0xffffffff;
    uint8_t path_ok = 0;
    int16_t found;
    int16_t claimed;

    memset(&query, 0, sizeof(query));
    memset(&candidate, 0, sizeof(candidate));
    query.unknown_41 = record[4];
    if (*(int16_t *)(record + 0xc) > 0) {
        query.goal_kind = 1;
        if (*(int16_t *)record > 0) {
            query.collect_all = 1;
            query.allow_random_fallback = 1;
        }
        *((uint8_t *)&query + 0x36) = 1;
        *(float *)((uint8_t *)&query + 0x38) = 10.0f;
        *(float *)((uint8_t *)&query + 0x3c) = 6.0f;
    } else {
        query.goal_kind = 2;
        *((uint8_t *)&query + 0x8) = record[5];
        query.search_radius = *(float *)(actor_tag + 0x320) > 0.0f ? *(float *)(actor_tag + 0x320) : 6.0f;
    }
    query.group_mask = actor_get_firing_position_group_mask(actor_index, query.goal_kind, 0);
    found = (int16_t)actor_find_best_firing_position(actor_index, &query, &candidate, &previous_owner, &path_context,
        &path_ok);
    *(int16_t *)(record + 8) = found;
    claimed = actor_claim_firing_position(actor_index, previous_owner, &path_context, found, path_ok);
    *(int16_t *)(record + 8) = claimed;
    record[0xa] = (claimed != -1 && path_ok == 0) ? 1 : 0;
    record[0x20] = 0;

    if (claimed != -1 && *(datum_index *)(record + 0x1c) != k_datum_index_none) {
        datum_index prop_index = *(datum_index *)(record + 0x1c);
        uint8_t *target = (uint8_t *)prop_data->data + (prop_index & 0xffff) * 0x138;
        uint32_t ignore_object;
        uint32_t request[0x12];
        uint8_t *goal = (uint8_t *)candidate.position;
        static path_find_context target_context;

        if (*(int16_t *)(target + 0x24) >= 2 && *(int16_t *)(target + 0x24) <= 3) {
            actor_target_get_relationship_object(prop_index);
        }
        ignore_object = *(uint32_t *)(target + 0x110);
        if (ignore_object == 0xffffffff) {
            ignore_object = *(uint32_t *)(target + 0x18);
        }
        actor = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
        memset(request, 0, sizeof(request));
        request[0] = *(uint32_t *)(actor_tag + 0x8c);           // pathfinding radius
        ((uint8_t *)request)[4] = 0;                              // ignores glass
        request[2] = ignore_object;
        request[3] = *(uint32_t *)(actor + 0x18);                 // the actor's unit
        ((uint8_t *)request)[0x10] = 1;                           // have start
        *(real_point3d *)&request[5] = *(real_point3d *)(target + 0xf0);
        request[8] = *(uint32_t *)(target + 0xec);
        memset(&target_context, 0, sizeof(target_context));
        target_context.structure_bsp = (uint32_t)global_structure_bsp;
        memcpy(&target_context, request, sizeof(request));
        target_context.unknown_48 = 0;
        target_context.have_goal = 1;
        target_context.goal_position = *(real_point3d *)goal;
        target_context.goal_vertex_id = *(uint32_t *)(goal + 0x14);
        *(uint32_t *)((uint8_t *)&target_context + 0x60) = 0;
        if (path_find_run(&target_context)) {
            uint8_t used_start = 0;

            record[0x20] = path_find_find_unobstructed_ancestor(&target_context, *(uint32_t *)(goal + 0x14),
                (real_point3d *)goal, &used_start, (real_point3d *)(record + 0x24));
        }
    }
    record[6] = 0;
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
