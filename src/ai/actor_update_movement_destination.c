// actor_update_movement_destination  (Ghidra: actor_update_movement_destination, renamed)
// address 0x403180, size 946 bytes
// name confidence: 0.4   rewrite confidence: 0.85
// REWRITTEN from objdump 0x403180..0x403531 (the fight mode's +0x0c process; the draft zeroed memory past its
//   own buffer and called the firing-position helpers without their register arguments). Always returns 0.
//   - nothing unless the actor wants a path (+0x4c) and has not committed to an order (+0x160);
//   - an actor with a target (+0x358) whose tag allows it (flag 0x20) and whose lead point (+0x168, surface +0x164)
//     is reachable (0x412960 after 0x429570) keeps moving there: it drops its firing position (+0x3b8 = -1 and
//     stops, 0x417570) when it is not yet at that position (accuracy radius from 0x429620) -- or, while it is
//     moving on its own (+0x504) and not suppressed (+0x1fc), when its target prop is within the definition's
//     +0xa0 range;
//   - otherwise it picks and claims a firing position (goal kind 0: 0x413e50, 0x414060) and, on a new one, waits
//     random(tag +0x3c0, +0x3c4) seconds (capped by a vehicle's +0x3a8) before the next pick (+0x9c, ticks);
//   - at combat status (+0x268) 7 or more it marks its target (+0x270) engaged (0x41fa80), except when it has an
//     unshielded threat weapon and the target is inside +0x608, or is inside +0x608 of the actor's firing position
//     while the actor is away from it.
// blam-cc: stack -> actor_index

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
extern Scenario *global_scenario;

extern real vector3d_distance_squared(real_point3d *a, real_point3d *b); // 0x401020, EAX, ECX
extern real random_real_range(real min, real max); // 0x401050
extern void *actor_get_actor_definition(datum_index actor_index); // 0x40fa70, EAX
extern uint8_t actor_firing_position_near_point(datum_index actor_index, real_point3d *point,
    int32_t start_surface_index, int16_t kind); // 0x412960, EDX, stack
extern int16_t actor_select_firing_position(datum_index actor_index, actor_firing_position_query *query,
    actor_firing_position_candidate *out_candidate, uint32_t *out_previous_owner, path_find_context *path_context,
    uint8_t *out_path_ok); // 0x413e50, stack, EBX query, EDI candidate
extern int16_t actor_claim_firing_position(datum_index actor_index, datum_index previous_owner,
    path_find_context *path_context, int16_t firing_position_index, uint8_t path_ok); // 0x414060, stack, CX, AL
extern void actor_movement_action_stop(datum_index actor_index); // 0x417570, EDX
extern void actor_target_mark_engaged(datum_index target_prop_index, datum_index actor_index, uint8_t mark_engaged); // 0x41fa80, EAX, EBX, stack
extern uint8_t actor_has_unshielded_threat_weapon(datum_index actor_index); // 0x428370, EAX
extern void actor_update_target_lead_position(datum_index actor_index); // 0x429570, EAX
extern float actor_compute_accuracy_scale(datum_index actor_index); // 0x429620, EAX

#define A_B(o) (actor[(o)])
#define A_W(o) (*(int16_t *)(actor + (o)))
#define A_D(o) (*(uint32_t *)(actor + (o)))
#define A_F(o) (*(float *)(actor + (o)))

static real_point3d *actor_held_firing_position(uint8_t *actor)
{
    uint8_t *encounter = (uint8_t *)global_scenario->encounters.pointer + (A_D(0x34) & 0xffff) * 0xb0;

    return (real_point3d *)(*(uint8_t **)(encounter + 0x9c) + A_W(0x3b8) * 0x18);
}

uint8_t actor_update_movement_destination(uint32_t actor_index)
{
    uint8_t *actor = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
    uint8_t *actor_tag;
    uint8_t *definition;

    if (A_B(0x4c) == 0) {
        return 0;
    }
    actor_tag = (uint8_t *)tag_instances[A_D(0x58) & 0xffff].data;
    definition = (uint8_t *)actor_get_actor_definition(actor_index);

    if (A_B(0x160) == 0) {
        uint8_t follow_lead = 0;

        if (A_B(0x358) != 0 && (actor_tag[0] & 0x20) != 0) {
            actor_update_target_lead_position(actor_index);
            follow_lead = actor_firing_position_near_point(actor_index, (real_point3d *)(actor + 0x168),
                (int32_t)A_D(0x164), 0);
        }
        if (follow_lead) {
            uint8_t at_position = 0;
            uint8_t drop = 0;

            if (A_D(0x34) != 0xffffffff && A_W(0x3b8) != -1) {
                float radius = actor_compute_accuracy_scale(actor_index);

                if (radius * radius > vector3d_distance_squared(actor_held_firing_position(actor),
                        (real_point3d *)(actor + 0x12c))) {
                    at_position = 1;
                }
            }
            if (A_B(0x504) != 0 && A_B(0x1fc) == 0) {
                if (A_D(0x270) != 0xffffffff) {
                    uint8_t *target = (uint8_t *)prop_data->data + (A_D(0x270) & 0xffff) * 0x138;

                    drop = ((prop *)target)->distance < *(float *)(definition + 0xa0);
                }
            } else {
                drop = !at_position;
            }
            if (drop) {
                A_W(0x3b8) = -1;
                actor_movement_action_stop(actor_index);
            }
        } else {
            // 0x403312
            static actor_firing_position_query query;
            static path_find_context path_context;
            actor_firing_position_candidate candidate;
            uint32_t previous_owner = 0xffffffff;
            uint8_t path_ok = 0;
            int16_t previous = A_W(0x3b8);
            int16_t selected;
            int16_t claimed;

            memset(&query, 0, sizeof(query));
            memset(&candidate, 0, sizeof(candidate));
            selected = actor_select_firing_position(actor_index, &query, &candidate, &previous_owner, &path_context,
                &path_ok);
            claimed = actor_claim_firing_position(actor_index, previous_owner, &path_context, selected, path_ok);
            actor = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
            if (claimed == -1) {
                A_W(0x9c) = 0;
            } else if (claimed != previous) {
                float wait = random_real_range(*(float *)(actor_tag + 0x3c0), *(float *)(actor_tag + 0x3c4));

                if (A_W(0x15e) > 0) {
                    uint8_t *vehicle = (uint8_t *)((object_header *)object_data->data)[A_D(0x158) & 0xffff].data;
                    uint8_t *vehicle_tag = (uint8_t *)tag_instances[*(datum_index *)vehicle & 0xffff].data;
                    float cap = *(float *)(vehicle_tag + 0x3a8);

                    if (cap > 0.0f && wait > cap) {
                        wait = cap;
                    }
                }
                A_W(0x9c) = (int16_t)(int32_t)(wait * 30.0f); // __ftol
            }
        }
    }

    // 0x40341f
    if (A_W(0x268) < 7) {
        return 0;
    }
    {
        uint8_t *target = (uint8_t *)prop_data->data + (A_D(0x270) & 0xffff) * 0x138;
        uint8_t engaged = 1;

        if (actor_has_unshielded_threat_weapon(actor_index)) {
            if (((prop *)target)->distance < A_F(0x608)) {
                engaged = 0;
            } else if (A_D(0x34) != 0xffffffff && A_W(0x3b8) != -1) {
                real_point3d *held = actor_held_firing_position(actor);
                float radius = actor_compute_accuracy_scale(actor_index);

                if (radius * radius < vector3d_distance_squared(held, (real_point3d *)(actor + 0x12c))) {
                    float range = A_F(0x608);

                    if (range * range > vector3d_distance_squared((real_point3d *)(target + 0xbc), held)) {
                        engaged = 0;
                    }
                }
            }
        }
        actor_target_mark_engaged(A_D(0x270), actor_index, engaged);
    }
    return 0;
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
