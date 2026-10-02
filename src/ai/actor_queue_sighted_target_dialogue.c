// actor_queue_sighted_target_dialogue  (Ghidra: actor_queue_sighted_target_dialogue, renamed)
// address 0x421c20, size 1104 bytes
// name confidence: 0.4   rewrite confidence: 0.4
// evidence: types/ai.h actor.awareness_level/vocalization_*/mode/mode_data,
//   actor.vocalization_unknown_3e8, actor.facing/facing_unknown_180/facing_unknown_18c
//   (0x174/0x178/0x17c, dotted against the target direction as the visibility-cone test);
//   prop.is_vault/is_unit/is_parented/unknown_54/58/5c/distance/unknown_12f/unknown_32/
//   unknown_e0/object_index; types/tags.h Actor.event_look_time_modifier[2] (0xd4/0xd8, same
//   pair as actor_queue_directional_reaction_event @0x422270) and Actor.surprise_distance
//   (0x2b0, confirmed by hand-counting the Actor struct's fields up to that offset). Calls
//   actor_record_look_at_point (0x421bc0), actor_queue_search_position (0x421af0, both
//   already rewritten in this module), datum_get (0x4d0680) and ai_communication_broadcast
//   (0x42d340). Unlike most functions in this range, Ghidra already resolved this one's three
//   parameters as ordinary (stack) parameters rather than hidden registers, and objdump
//   against bin/halo.exe (0x421c20..0x421fbd) confirms all three really do arrive on the
//   stack at [esp+4]/[esp+8]/[esp+0xc] with no register arguments at all -- an unusual, but
//   directly verified, plain-cdecl convention for this particular function.
//   UNSURE: two already-completed files elsewhere in this module
//   (src/ai/actor_target_data_release.c, src/ai/actor_target_reset_combat_flags.c) declare
//   externs for this function with only one or zero visible arguments; per the tail-jump at
//   the end of actor_target_reset_combat_flags (0x41baf0, outside this rewrite's range),
//   those declarations look incomplete, but that file was not touched here.
// register convention: stack -> actor_index, target_prop_index, already_noticed.
//   // blam-cc: stack -> actor_index, target_prop_index, already_noticed

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "game.h"
#include "objects.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *actor_data;      // 0x00880360
extern data_array *prop_data;       // 0x008802c0
extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern game_time_globals *game_time; // 0x006f1d6c
extern uint8_t ai_debug_gate_87abc6; // 0x0087abc6, UNSURE: no name established elsewhere

extern real random_real_range(real min, real max); // 0x401050
extern void actor_record_look_at_point(datum_index actor_index, const uint32_t *point, int16_t priority, uint32_t data); // 0x421bc0
extern void actor_queue_search_position(datum_index actor_index, real_point3d *position, int16_t priority,
                                        real_vector3d *velocity, uint32_t unknown_324, uint32_t unknown_328,
                                        uint32_t unknown_33c, uint32_t unknown_340, uint32_t unknown_344,
                                        uint8_t unknown_348); // 0x421af0
extern void * datum_get(datum_index handle, data_array *array); // 0x4d0680
extern void ai_communication_broadcast(int32_t event_code, datum_index unit_index, datum_index object_a, int32_t reason, datum_index object_b, datum_index object_c, uint32_t *extra_data); // 0x42d340

// Variant table paired with actor_dialogue_variant_table_b @0x00655638; indexed the same way
// (combat_status >= 4).
extern int16_t actor_dialogue_variant_table_a[]; // 0x00655638

// blam-cc: stack -> actor_index, target_prop_index, already_noticed
// The main "did I just notice/see this target" handler: if the target prop is not a vault,
// queues category-4 "sighted" dialogue with a randomized duration (scaled by the actor's
// vitality grade and the Actor tag's event_look_time_modifier range), throttled by a
// per-target re-notice cooldown of 600 ticks read/written on the *validated* prop record.
// Independently of that gate, if the target is a unit, tests whether it lies within roughly a
// 60-degree cone of the actor's facing and, if not (or if it is far beyond the actor's
// surprise distance), records a look-at point toward it and queues a matching search
// position. Finally, if the actor is not yet actively engaged and the target is close enough
// and the actor controls a unit, broadcasts a "target sighted" squad event.
void actor_queue_sighted_target_dialogue(datum_index actor_index, datum_index target_prop_index, uint8_t already_noticed)
{
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];
    prop *target = &((prop *)prop_data->data)[target_prop_index & 0xffff];
    Actor *actor_tag;
    prop *validated;

    if (target->dead != 0) {
        goto broadcast_check;
    }

    actor_tag = (Actor *)(tag_instances[self->actor_definition_tag & 0xffff].data);

    if (self->awareness_level > 1 && self->vocalization_line < 5 &&
        (self->mode != 11 || self->mode_data.raw[3] != 0)) {
        int16_t recent = self->vocalization_unknown_3e8;

        validated = (prop *)datum_get(target_prop_index, prop_data);
        if (validated != 0) {
            if ((validated->enemy == 0 && validated->dead == 0) ||
                (validated->dead != 0 && self->awareness_level > 2)) {
                if (recent <= 6) {
                    if (validated->is_parented == 0 && validated->last_attention_time != -1 &&
                        (int32_t)game_time->game_time >= validated->last_attention_time + 600) {
                        validated->last_attention_time = (int32_t)game_time->game_time;
                        validated->interest_satisfied = (validated->interest_satisfied <= validated->interest)
                                                     ? validated->interest
                                                     : validated->interest_satisfied;
                    }
                }
            }
            if (recent <= 6) {
                float wait_scale = (self->awareness_level < 3 || self->combat_status == 0) ? 1.8f : 0.9f;

                if (actor_tag->event_look_time_modifier[0] != 0.0f || actor_tag->event_look_time_modifier[1] != 0.0f) {
                    float min_scale = (actor_tag->event_look_time_modifier[0] <= 0.5f) ? 0.5f : actor_tag->event_look_time_modifier[0];
                    float max_scale = (actor_tag->event_look_time_modifier[1] <= 2.0f) ? actor_tag->event_look_time_modifier[1] : 2.0f;
                    wait_scale = random_real_range(min_scale, max_scale) * wait_scale;
                }

                int32_t ticks = (int32_t)(wait_scale * 30.0f + 0.5f); // ROUND
                if (ticks > 0x7fff) {
                    ticks = 0x7fff;
                }

                self->vocalization_line = 4;
                self->vocalization_state = (int16_t)ticks;
                self->vocalization_unknown_54c = 1; // kind = 1 (explicit target)
                self->vocalization_variant = actor_dialogue_variant_table_a[self->combat_status >= 4];
                self->vocalization_unknown_550 = target_prop_index;
                self->vocalization_unknown_554 = 0;
                self->vocalization_unknown_558 = 0;
            }
        }
    }

    // This block is a literal transliteration of LAB_00421e8f..LAB_00421fbd (kept as goto/
    // labels rather than restructured, since the branch nest is easy to get subtly wrong):
    // param_3 (already_noticed here) is reused by the original as scratch storage exactly as
    // shown, and priority (bVar16 in the original) likewise.
    if (target->enemy != 0) {
        // UNSURE: threshold derivation ("< 0.5" on the forward-facing dot product) is kept
        // literal; roughly a 60-degree half-cone.
        float facing_dot = target->direction.z * self->facing.k
                          + target->direction.y * self->facing.j
                          + target->direction.x * self->facing.i;
        int outside_cone = facing_dot < 0.5f; // bVar8
        int priority = 0;                     // bVar16

        if (self->combat_status == 0) {
            already_noticed = 0;
            if (self->awareness_level < 3 && target->shooting != 0 &&
                target->distance < actor_tag->surprise_distance && priority < 4) {
                priority = 3;
            }
            goto shared_check;
        } else if (self->combat_status < 5 || outside_cone) {
            if (already_noticed == 0) {
                goto shared_check;
            }
            goto notify_unit;
        } else {
            already_noticed = 1;
            goto notify_unit;
        }

    shared_check:
        if (target->shooting == 0 || !(target->distance < actor_tag->surprise_distance)) {
            if (priority == 0) goto notify_unit;
        } else if (outside_cone) {
            if (priority > 7) goto notify_unit;
        } else if (priority > 6) {
            goto notify_unit;
        }

        actor_record_look_at_point(actor_index, (const uint32_t *)&target->direction, (int16_t)priority, target_prop_index);

    notify_unit:
        if (self->combat_status < 3 && already_noticed == 0 &&
            target->visual_perception < 2 && self->unit_index != (datum_index)k_datum_index_none) {
            ai_communication_broadcast(6, self->unit_index, target->object_index, 3,
                                       (datum_index)k_datum_index_none, (datum_index)k_datum_index_none, 0);
        }
    }

broadcast_check:
    // UNSURE: the sentinel actor.type != 15 and the global gate byte at 0x0087abc6 (no name
    // established elsewhere) both come straight from the disassembly; ai_types_notes.md does
    // not attribute either. actor+0x4 is actor.type (types/ai.h).
    if (target->is_parented != 0 && target->enemy != 0 && target->dead == 0 &&
        self->type != 15 && ai_debug_gate_87abc6 != 0) {
        if (self->swarm == 0) {
            datum_index unit_index = self->unit_index;
            object_header *header = &((object_header *)object_data->data)[unit_index & 0xffff];
            ((uint8_t *)header->data + 0x106)[0] |= 0x20;
        } else {
            datum_index cluster_index = self->cluster_unit_index;
            while (cluster_index != (datum_index)k_datum_index_none) {
                object_header *header = &((object_header *)object_data->data)[cluster_index & 0xffff];
                struct object *unit_object = header->data;
                ((struct object *)unit_object)->vitality_flags |= 0x20;
                cluster_index = *(datum_index *)((uint8_t *)unit_object + 0x1fc); // unit.swarm_next_unit_index
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x421c20):

void FUN_00421c20(uint param_1,uint param_2,char param_3)

{
  ushort *puVar1;
  byte *pbVar2;
  undefined4 uVar3;
  short sVar4;
  short sVar5;
  undefined2 uVar6;
  int iVar7;
  bool bVar8;
  char cVar9;
  int iVar10;
  int iVar11;
  uint uVar12;
  int iVar13;
  int iVar14;
  int iVar15;
  byte bVar16;
  float fVar17;
  float local_24;
  float local_1c;
  float local_18;
  undefined4 local_10;
  undefined4 local_8;
  undefined4 local_4;

  iVar14 = (param_1 & 0xffff) * 0x724;
  iVar11 = *(int *)(DAT_00880360 + 0x34) + iVar14;
  iVar7 = *(int *)((*(uint *)(iVar11 + 0x58) & 0xffff) * 0x20 + 0x14
                  + DAT_0087bc14);
  iVar15 = (param_2 & 0xffff) * 0x138 + *(int *)(DAT_008802c0 + 0x34);
  if (*(char *)(iVar15 + 0x127) != '\0') goto LAB_00421fbd;
  iVar13 = *(int *)((*(uint *)(iVar11 + 0x58) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  sVar4 = *(short *)(iVar11 + 0x6a);
  local_10 = CONCAT22(local_10._2_2_,1);
  if ((((1 < sVar4) && (*(short *)(iVar11 + 0x544) < 5)) &&
      ((sVar5 = *(short *)(iVar11 + 1000), *(short *)(iVar11 + 0x6c) != 0xb ||
       (*(char *)(iVar11 + 0x9f) != '\0')))) && (iVar10 = datum_get(), iVar10 != 0)) {
    if (((*(char *)(iVar10 + 0x60) == '\0') && (*(char *)(iVar10 + 0x127) == '\0')) ||
       ((*(char *)(iVar10 + 0x127) != '\0' && (2 < sVar4)))) {
      if ((6 < sVar5) ||
         (((*(char *)(iVar10 + 0x12e) == '\0' && (*(int *)(iVar10 + 0x5c) != -1)) &&
          (*(int *)(DAT_006f1d6c + 0xc) < *(int *)(iVar10 + 0x5c) + 600)))) goto LAB_00421e8f;
      *(int *)(iVar10 + 0x5c) = *(int *)(DAT_006f1d6c + 0xc);
      if (*(float *)(iVar10 + 0x58) <= *(float *)(iVar10 + 0x54)) {
        uVar3 = *(undefined4 *)(iVar10 + 0x54);
      }
      else {
        uVar3 = *(undefined4 *)(iVar10 + 0x58);
      }
      *(undefined4 *)(iVar10 + 0x58) = uVar3;
    }
    local_24 = 0.9;
    if ((*(short *)(iVar11 + 0x6a) < 3) || (*(short *)(iVar11 + 0x6e) == 0)) {
      local_24 = 1.8;
    }
    if ((*(float *)(iVar13 + 0xd4) != 0.0) || (*(float *)(iVar13 + 0xd8) != 0.0)) {
      if (*(float *)(iVar13 + 0xd4) <= 0.5) {
        local_18 = 0.5;
      }
      else {
        local_18 = *(float *)(iVar13 + 0xd4);
      }
      if (*(float *)(iVar13 + 0xd8) <= 2.0) {
        local_1c = *(float *)(iVar13 + 0xd8);
      }
      else {
        local_1c = 2.0;
      }
      fVar17 = random_real_range(local_18,local_1c);
      local_24 = fVar17 * local_24;
    }
    iVar13 = (int)ROUND(local_24 * 30.0);
    if (0x7fff < iVar13) {
      iVar13 = 0x7fff;
    }
    uVar6 = *(undefined2 *)(&DAT_00655638 + (uint)(3 < *(short *)(iVar11 + 0x6e)) * 2);
    *(undefined2 *)(iVar11 + 0x544) = 4;
    *(short *)(iVar11 + 0x548) = (short)iVar13;
    *(undefined4 *)(iVar11 + 0x54c) = local_10;
    *(undefined2 *)(iVar11 + 0x546) = uVar6;
    *(uint *)(iVar11 + 0x550) = param_2;
    *(undefined4 *)(iVar11 + 0x554) = local_8;
    *(undefined4 *)(iVar11 + 0x558) = local_4;
  }
LAB_00421e8f:
  if (*(char *)(iVar15 + 0x60) == '\0') goto LAB_00421fbd;
  bVar16 = 0;
  bVar8 = *(float *)(iVar15 + 0xe0) * *(float *)(iVar11 + 0x174) +
          *(float *)(iVar11 + 0x178) * *(float *)(iVar15 + 0xe4) +
          *(float *)(iVar11 + 0x17c) * *(float *)(iVar15 + 0xe8) < 0.5;
  if (*(short *)(iVar11 + 0x6e) == 0) {
    param_3 = '\0';
    cVar9 = '\0';
    if (((*(short *)(iVar11 + 0x6a) < 3) &&
        (bVar16 = *(char *)(iVar15 + 0x12f) != '\0', cVar9 = param_3,
        *(float *)(iVar15 + 0x11c) < *(float *)(iVar7 + 0x2b0))) && (bVar16 < 4)) {
      bVar16 = 3;
    }
LAB_00421f33:
    param_3 = cVar9;
    if ((*(char *)(iVar15 + 0x12f) == '\0') ||
       (*(float *)(iVar7 + 0x2b0) <= *(float *)(iVar15 + 0x11c))) {
LAB_00421f72:
      if (bVar16 == 0) goto LAB_00421f88;
    }
    else if (bVar8) {
      if (7 < bVar16) goto LAB_00421f72;
    }
    else if (6 < bVar16) goto LAB_00421f72;
    FUN_00421bc0(param_2);
  }
  else if ((*(short *)(iVar11 + 0x6e) < 5) || (bVar8)) {
    cVar9 = param_3;
    if (param_3 == '\0') goto LAB_00421f33;
  }
  else {
    param_3 = '\x01';
  }
LAB_00421f88:
  if (((*(short *)(iVar11 + 0x6e) < 3) && (param_3 == '\0')) &&
     ((*(short *)(iVar15 + 0x32) < 2 && (*(int *)(iVar11 + 0x18) != -1)))) {
    ai_communication_broadcast
              (6,*(int *)(iVar11 + 0x18),*(undefined4 *)(iVar15 + 0x18),3,0xffffffff,0xffffffff,0);
  }
LAB_00421fbd:
  iVar7 = DAT_008603b0;
  if ((((*(char *)(iVar15 + 0x12e) != '\0') && (*(char *)(iVar15 + 0x60) != '\0')) &&
      (*(char *)(iVar15 + 0x127) == '\0')) &&
     ((*(short *)(iVar11 + 4) != 0xf && (DAT_0087abc6 != '\0')))) {
    iVar11 = *(int *)(DAT_00880360 + 0x34) + iVar14;
    if (*(char *)(*(int *)(DAT_00880360 + 0x34) + 6 + iVar14) == '\0') {
      pbVar2 = (byte *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                (*(uint *)(iVar11 + 0x18) & 0xffff) * 0xc) + 0x106);
      *pbVar2 = *pbVar2 | 0x20;
    }
    else {
      uVar12 = *(uint *)(iVar11 + 0x24);
      if (uVar12 != 0xffffffff) {
        do {
          iVar11 = *(int *)(*(int *)(iVar7 + 0x34) + 8 + (uVar12 & 0xffff) * 0xc);
          puVar1 = (ushort *)(iVar11 + 0x106);
          *puVar1 = *puVar1 | 0x20;
          uVar12 = *(uint *)(iVar11 + 0x1fc);
        } while (uVar12 != 0xffffffff);
        return;
      }
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
