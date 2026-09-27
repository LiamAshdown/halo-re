// actor_update_movement_destination  (Ghidra: actor_update_movement_destination, renamed)
// address 0x403180, size 946 bytes
// name confidence: 0.4   rewrite confidence: 0.2
// evidence: types/ai.h actor.needs_new_path/order_committed/firing_position_index/
//   encounter_index/unknown_164/unknown_168/unknown_504/unknown_1fc/target_unit_index/
//   vitality_wait_time/target_combat_status; types/tags.h Scenario.encounters (pointer at
//   0x430) and ScenarioEncounter.firing_positions (TagReflexive at 0x98, pointer at 0x9c,
//   stride 0x18 matching ScenarioFiringPosition's declared size); phase-4 summary "decides
//   whether the actor needs a new movement destination based on target visibility and
//   morale, and if so issues a pathfinding request for one".
//
// Kept close to the Ghidra decompilation (original labels/variable names preserved) given
// its size and the number of unresolved out-of-range callees; see UNSURE notes below.
// UNSURE, broadly:
//  - actor+0x358 (a byte gating a "use lead position" path) and actor+0x9c/0x164/0x168 (a
//    lead-position record) are used as raw/named-where-possible offsets; 0x164/0x168 are
//    already named in types/ai.h (as opaque unknown_164/unknown_168) but their true shape as
//    a lead-position record is not confirmed here.
//  - actor_firing_position_near_point, actor_compute_accuracy_scale (returns a float via ST0), vector3d_distance_squared (returns a float via
//    ST0), actor_has_unshielded_threat_weapon, actor_target_mark_engaged and actor_update_target_lead_position are all outside
//    this session's range; declared with the signature each call site implies.
//  - Several float NaN/ordering comparisons in the original (the `CONCAT22(...NAN...)`
//    patterns) are SSE/x87 flag byproducts of a single `<`/`==` comparison; simplified here
//    to the equivalent plain comparison.
//  - The request/scratch buffers passed to actor_select_firing_position/actor_claim_firing_position mirror
//    actor_update_path_if_needed.c's treatment (opaque byte buffers, not claimed to be
//    path_find_context).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"
#include <string.h>

extern data_array *actor_data;      // 0x00880360
extern data_array *prop_data;       // 0x008802c0
extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern Scenario *global_scenario;   // 0x00746f8c

extern real vector3d_distance_squared(real_point3d *a, real_point3d *b); // 0x401020, src/math; blam-cc: EAX a, ECX b
extern real random_real_range(real min, real max);  // 0x401050
extern void * actor_get_actor_definition(datum_index actor_index); // 0x40fa70, this session (later)
extern uint8_t actor_firing_position_near_point(); // SIGNATURE-CONFLICT: this call site and the rewrite of actor_firing_position_near_point at 0x412960
                 // disagree on the argument list; Ghidra drops the register arguments
                 // here. Left unprototyped so the conflict is visible. See src/ai/README.md.
extern int16_t actor_select_firing_position(); // SIGNATURE-CONFLICT: this call site and the rewrite of actor_select_firing_position at 0x413e50
                 // disagree on the argument list; Ghidra drops the register arguments
                 // here. Left unprototyped so the conflict is visible. See src/ai/README.md.
extern int16_t actor_claim_firing_position(); // SIGNATURE-CONFLICT: this call site and the rewrite of actor_claim_firing_position at 0x414060
                 // disagree on the argument list; Ghidra drops the register arguments
                 // here. Left unprototyped so the conflict is visible. See src/ai/README.md.
extern void actor_movement_action_stop(datum_index actor_index); // 0x417570, this module,
                                                                 // blam-cc: EDX -> actor_index
extern uint8_t actor_target_mark_engaged(uint8_t want_new_destination); // 0x41fa80, not yet rewritten
extern uint8_t actor_has_unshielded_threat_weapon(datum_index actor_index);                       // 0x428370, not yet rewritten (same gate as actor_get_consideration_wait_threshold.c)
extern void actor_update_target_lead_position(datum_index actor_index);     // 0x429570, not yet rewritten
extern float actor_compute_accuracy_scale(datum_index actor_index);                         // 0x429620, not yet rewritten

// Decides whether the actor needs a new movement destination. See the file header for scope.
uint8_t actor_update_movement_destination(uint32_t actor_index)
{
    actor *a = &((actor *)actor_data->data)[actor_index & 0xffff];
    uint8_t *actor_base = (uint8_t *)a;
    uint8_t result = 0;

    if (a->needs_new_path == 0) {
        return 0;
    }

    {
        Actor *actor_def = (Actor *)tag_instances[a->actor_definition_tag & 0xffff].data;
        Actor *unit_actor_def;

        if (a->order_committed != 0) {
            result = a->order_committed;
            goto check_distance_gate;
        }

        unit_actor_def = actor_get_actor_definition(actor_index);
        if (*(actor_base + 0x358) == 0 || (((uint8_t *)actor_def)[0] & 0x20) == 0) {
        use_direct_path:;
            {
                uint32_t request_block[16];
                uint8_t scratch_context[65684];
                uint8_t reached_exactly;
                int16_t prior_firing_position = a->firing_position_index;
                int32_t waypoint;

                memset((uint8_t *)request_block + sizeof(request_block), 0, 0x199 * 4);
                request_block[1] = 0;
                waypoint = actor_select_firing_position(actor_index, request_block, scratch_context, &reached_exactly);
                (void)waypoint;
                waypoint = actor_claim_firing_position(actor_index, request_block[0], scratch_context);
                if ((int16_t)waypoint == -1) {
                    *(int16_t *)(a->mode_data) = 0;
                } else if ((int16_t)waypoint != prior_firing_position) {
                    float delay = random_real_range(*(float *)((uint8_t *)unit_actor_def + 0x3c0), *(float *)((uint8_t *)unit_actor_def + 0x3c4));
                    *(int16_t *)(a->mode_data) = (int16_t)delay;
                }
            }
        } else {
            char lead_ok;

            actor_update_target_lead_position(actor_index);
            lead_ok = actor_firing_position_near_point(actor_base + 0x168, *(uint32_t *)(actor_base + 0x164), 0);
            if (lead_ok == 0) {
                goto use_direct_path;
            }
            {
                uint8_t close_enough = 0;

                if (a->encounter_index != (datum_index)k_datum_index_none && a->firing_position_index != -1) {
                    ScenarioEncounter *encounters = (ScenarioEncounter *)global_scenario->encounters.pointer;
                    ScenarioFiringPosition *held = &((ScenarioFiringPosition *)
                        encounters[a->encounter_index & 0xffff].firing_positions.pointer)[a->firing_position_index];
                    float lead_speed = actor_compute_accuracy_scale(actor_index);
                    // 0x40324f..0x403281: EAX = the held firing position, ECX = &body_position
                    float distance_sq = vector3d_distance_squared((real_point3d *)held, &a->body_position);
                    if (lead_speed * lead_speed < distance_sq) {
                        close_enough = 1;
                    }
                }
                // FIXED: Ghidra reads actor+0x1fc as a BYTE (`*(char *)(iVar8 + 0x1fc) != '0'`), not as a
                // datum handle; 0x1fc is one of the byte counters inside actor.tally.
                if (a->unknown_504 == 0 || a->tally.threat_class_5 != 0) {
                    if (close_enough == 0) {
                    reset_firing_position:
                        a->firing_position_index = -1;
                        // UNSURE: Ghidra writes this call result into the return slot, but
                        // actor_movement_action_stop is void; the EAX it leaves is the leftover
                        // of its own tail call to actor_movement_action_resolve. Not modelled,
                        // so result keeps its previous value here.
                        actor_movement_action_stop(actor_index);
                    }
                } else if (a->target_unit_index != (datum_index)k_datum_index_none) {
                    prop *p = &((prop *)prop_data->data)[a->target_unit_index & 0xffff];
                    if (p->distance < unit_actor_def->pathfinding_radius) { // UNSURE: field guess, see below
                        goto reset_firing_position;
                    }
                }
            }
        }
    }

check_distance_gate:
    if (a->target_combat_status < 7) {
        return result;
    }
    if (a->target_unit_index != (datum_index)k_datum_index_none) {
        prop *p = &((prop *)prop_data->data)[a->target_unit_index & 0xffff];
        uint8_t want = 1;

        if (actor_has_unshielded_threat_weapon(actor_index) != 0) {
            if (p->distance < a->vitality_wait_time) {
                want = 0;
            } else if (a->encounter_index != (datum_index)k_datum_index_none && a->firing_position_index != -1) {
                ScenarioEncounter *encounters = (ScenarioEncounter *)global_scenario->encounters.pointer;
                ScenarioFiringPosition *firing_positions = (ScenarioFiringPosition *)encounters[a->encounter_index & 0xffff].firing_positions.pointer;
                ScenarioFiringPosition *fp = &firing_positions[a->firing_position_index];
                float lead_speed;
                float distance_sq;

                lead_speed = actor_compute_accuracy_scale(actor_index);
                // 0x4034c3..0x4034d1: EAX = fp, ECX = &body_position
                distance_sq = vector3d_distance_squared((real_point3d *)fp, &a->body_position);
                if (lead_speed * lead_speed < distance_sq) {
                    float vitality_distance_sq;

                    distance_sq = a->vitality_wait_time;
                    // 0x4034f1..0x4034fb: EAX = prop + 0xbc (last_known_position), ECX = fp
                    vitality_distance_sq = vector3d_distance_squared(&p->last_known_position, (real_point3d *)fp);
                    if (vitality_distance_sq < distance_sq * distance_sq) {
                        want = 0;
                    }
                }
            }
        }
        result = actor_target_mark_engaged(want);
    } else {
        result = actor_target_mark_engaged(1);
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x403180):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

uint FUN_00403180(uint param_1)

{
  float fVar1;
  float fVar2;
  byte *pbVar3;
  char cVar4;
  undefined4 uVar5;
  undefined2 extraout_var;
  uint uVar6;
  int iVar7;
  int iVar8;
  undefined4 *puVar9;
  float10 fVar10;
  float10 fVar11;
  char cStack_10749;
  byte *pbStack_10748;
  float fStack_10744;
  int aiStack_10740 [16];
  undefined4 uStack_10700;
  undefined2 uStack_106fc;
  undefined1 auStack_10098 [65684];

  iVar8 = (param_1 & 0xffff) * 0x724;
  cVar4 = *(char *)(iVar8 + 0x4c + *(int *)(DAT_00880360 + 0x34));
  uVar6 = CONCAT31((int3)((uint)DAT_00880360 >> 8),cVar4);
  iVar8 = iVar8 + *(int *)(DAT_00880360 + 0x34);
  if (cVar4 == '\0') goto LAB_00403529;
  pbVar3 = *(byte **)((*(uint *)(iVar8 + 0x58) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  pbStack_10748 = pbVar3;
  aiStack_10740[0] = actor_get_actor_definition();
  uVar6 = CONCAT31((int3)((uint)aiStack_10740[0] >> 8),*(char *)(iVar8 + 0x160));
  if (*(char *)(iVar8 + 0x160) != '\0') goto LAB_0040341f;
  if ((*(char *)(iVar8 + 0x358) == '\0') || ((*pbVar3 & 0x20) == 0)) {
LAB_00403312:
    fStack_10744 = (float)CONCAT22(fStack_10744._2_2_,*(undefined2 *)(iVar8 + 0x3b8));
    puVar9 = &uStack_10700;
    for (iVar7 = 0x199; iVar7 != 0; iVar7 = iVar7 + -1) {
      *puVar9 = 0;
      puVar9 = puVar9 + 1;
    }
    uStack_106fc = 0;
    FUN_00413e50(param_1,aiStack_10740,auStack_10098,&cStack_10749);
    uVar6 = FUN_00414060(param_1,aiStack_10740[0],auStack_10098);
    if ((short)uVar6 == -1) {
      *(undefined2 *)(iVar8 + 0x9c) = 0;
    }
    else if ((short)uVar6 != fStack_10744._0_2_) {
      random_real_range(*(float *)(pbStack_10748 + 0x3c0),*(float *)(pbStack_10748 + 0x3c4));
      uVar6 = __ftol();
      *(short *)(iVar8 + 0x9c) = (short)uVar6;
    }
  }
  else {
    actor_update_target_lead_position();
    uVar5 = FUN_00412960(iVar8 + 0x168,*(undefined4 *)(iVar8 + 0x164),0);
    if ((char)uVar5 == '\0') goto LAB_00403312;
    cStack_10749 = '\0';
    if ((*(int *)(iVar8 + 0x34) != -1) && (*(short *)(iVar8 + 0x3b8) != -1)) {
      fVar10 = (float10)FUN_00429620();
      pbStack_10748 = (byte *)(float)fVar10;
      fVar10 = (float10)FUN_00401020();
      fVar11 = (float10)(float)pbStack_10748 * (float10)(float)pbStack_10748;
      uVar5 = CONCAT22(extraout_var,
                       (ushort)(fVar11 < fVar10) << 8 | (ushort)(NAN(fVar11) || NAN(fVar10)) << 10 |
                       (ushort)(fVar11 == fVar10) << 0xe);
      if (fVar11 < fVar10 == 0 && (fVar11 == fVar10) == 0) {
        cStack_10749 = '\x01';
      }
    }
    if ((*(char *)(iVar8 + 0x504) == '\0') || (*(char *)(iVar8 + 0x1fc) != '\0')) {
      uVar6 = CONCAT31((int3)((uint)uVar5 >> 8),cStack_10749);
      if (cStack_10749 == '\0') {
LAB_004032fd:
        *(undefined2 *)(iVar8 + 0x3b8) = 0xffff;
        uVar6 = actor_movement_action_stop();
      }
    }
    else {
      uVar6 = *(uint *)(iVar8 + 0x270);
      if (uVar6 != 0xffffffff) {
        fVar1 = *(float *)((uVar6 & 0xffff) * 0x138 + 0x11c + *(int *)(DAT_008802c0 + 0x34));
        fVar2 = *(float *)(aiStack_10740[0] + 0xa0);
        uVar6 = CONCAT22((short)((uint)aiStack_10740[0] >> 0x10),
                         (ushort)(fVar1 < fVar2) << 8 | (ushort)(NAN(fVar1) || NAN(fVar2)) << 10 |
                         (ushort)(fVar1 == fVar2) << 0xe);
        if (fVar1 < fVar2) goto LAB_004032fd;
      }
    }
  }
LAB_0040341f:
  if (*(short *)(iVar8 + 0x268) < 7) goto LAB_00403529;
  uVar6 = *(uint *)(iVar8 + 0x270);
  iVar7 = *(int *)(DAT_008802c0 + 0x34);
  pbStack_10748 = (byte *)CONCAT31(pbStack_10748._1_3_,1);
  cVar4 = FUN_00428370();
  if (cVar4 != '\0') {
    if (*(float *)((uVar6 & 0xffff) * 0x138 + iVar7 + 0x11c) < *(float *)(iVar8 + 0x608)) {
LAB_00403511:
      pbStack_10748 = (byte *)((uint)pbStack_10748 & 0xffffff00);
    }
    else if ((*(uint *)(iVar8 + 0x34) != 0xffffffff) && (*(short *)(iVar8 + 0x3b8) != -1)) {
      aiStack_10740[0] =
           *(int *)((*(uint *)(iVar8 + 0x34) & 0xffff) * 0xb0 + 0x9c +
                   *(int *)(global_scenario + 0x430)) + *(short *)(iVar8 + 0x3b8) * 0x18;
      fVar10 = (float10)FUN_00429620();
      fStack_10744 = (float)fVar10;
      fVar10 = (float10)FUN_00401020();
      if ((float10)fStack_10744 * (float10)fStack_10744 < fVar10) {
        fStack_10744 = *(float *)(iVar8 + 0x608);
        fVar10 = (float10)FUN_00401020();
        if (fVar10 < (float10)fStack_10744 * (float10)fStack_10744) goto LAB_00403511;
      }
    }
  }
  uVar6 = FUN_0041fa80(pbStack_10748);
LAB_00403529:
  return uVar6 & 0xffffff00;
}
#endif
