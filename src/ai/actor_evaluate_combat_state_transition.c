// actor_evaluate_combat_state_transition  (Ghidra: already named)
// address 0x40c620, size 1443 bytes
// name confidence: 0.5   rewrite confidence: 0.15
// evidence: types/ai.h actor.target_unit_index/mode/active_unit_index/unknown_15e/
//   unknown_378/unknown_1cb/unknown_375/position (0x174..0x17c); prop.distance (0x11c);
//   calls actor_consider_combat_mode, actor_set_mode, actor_set_combat_alert_flag, this
//   session's actor_get_actor_definition; phase-4 summary "decides whether the actor should
//   transition into full combat/alert state and, if so, commits the corresponding order".
//
// This is the densest remaining function in this session's range (27 distinct conditions
// across several nested gates). Kept close to the Ghidra decompilation, preserving its
// control-flow shape (gotos and all) rather than restructuring it, given the size and the
// number of unresolved tag/mode_data offsets. Confidence is low; treat as a starting point.
// UNSURE, broadly: actor+0xa0/0xa3/0xa4/0xc5/0x37c/0x380/0x388/0x470 fall inside or just past
// actor.mode_data; Actor-tag offsets read through `puVar3`/`iVar2`/`iVar10` (three different
// tag-data pointers: the actor's own Actor tag, its ActorVariant tag, and a possibly
// per-unit-overridden Actor tag from actor_get_actor_definition) are not individually named.
// weapon_get_zoom_fov and actor_has_unshielded_threat_weapon are outside this session's range.
//   Actor+0x37c/0x380/0x388 are read here as raw int32 timestamps (compared against -1
//   and against the game tick), which conflicts with types/ai.h's typing of those three
//   offsets as floats (search_wait_time/unknown_380/unknown_388, established by
//   actor_get_consideration_wait_threshold.c) -- that evidence turns out to come from an
//   Actor *tag* pointer at the same numeric offset within that function, not the runtime
//   actor struct, so the two are likely unrelated fields sharing a coincidental offset.
//   Read here as raw bytes rather than picking one interpretation.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"
#include "game.h"

extern data_array *actor_data;      // 0x00880360
extern data_array *prop_data;       // 0x008802c0
extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern game_time_globals *game_time; // 0x006f1d6c

extern uint8_t actor_consider_combat_mode(uint32_t actor_index, int16_t consideration_mode, actor_combat_consideration *out); // 0x401a60, this session
extern void actor_set_mode(datum_index actor_index, int32_t mode, void *mode_data);            // 0x40d8d0, sibling session
extern uint8_t actor_handle_death(datum_index actor_index, uint8_t param_2, uint8_t param_3);             // 0x40dd50, this session (later)
extern void * actor_get_actor_definition(datum_index actor_index);                                   // 0x40fa70, this session (later)
extern void actor_set_combat_alert_flag(void);                                                    // 0x421a40, not yet rewritten
extern uint8_t actor_has_unshielded_threat_weapon(datum_index actor_index);                                                                // 0x428370, not yet rewritten
extern real weapon_get_zoom_fov(int16_t zoom_table_index, int16_t magnification);
    // 0x46fe10, blam-cc: stack -> zoom_table_index, CX -> magnification (every caller passes the difficulty)
extern uint8_t *main_game_globals; // 0x006b0b80 game globals *, +0x0e difficulty

char actor_evaluate_combat_state_transition(uint32_t actor_index)
{
    actor *a = &((actor *)actor_data->data)[actor_index & 0xffff];
    uint8_t *actor_base = (uint8_t *)a;
    Actor *actor_def = (Actor *)tag_instances[a->actor_definition_tag & 0xffff].data;
    ActorVariant *variant = (ActorVariant *)tag_instances[a->actor_variant_tag & 0xffff].data;
    Actor *unit_def = actor_get_actor_definition(actor_index);
    char result = 0;
    prop *threat = 0;
    float threat_distance = 3.4028235e+38f;
    uint8_t consideration[132];
    (void)variant;

    if (a->target_unit_index != (datum_index)k_datum_index_none) {
        threat = &((prop *)prop_data->data)[a->target_unit_index & 0xffff];
        threat_distance = threat->distance;

        if (a->mode == 10 && *(int16_t *)(a->mode_data + (0xa0 - 0x9c)) == 1 &&
            (((uint8_t *)threat)[0x74] != 0 ||
             (((uint8_t *)threat)[0x12f] != 0 && ((uint8_t *)threat)[0x121] < 2) ||
             (*(float *)((uint8_t *)actor_def + 0x328) > 0.0f && // UNSURE: puVar3[0xca] offset guessed
              (int16_t)(*(float *)((uint8_t *)actor_def + 0x328) * 30.0f) <= *(int16_t *)(actor_base + 0xc2))) &&
            (threat_distance <= unit_def->attacking_evasion_threshold - 0x50 /* UNSURE: iVar10+0xa0 offset guessed */ ||
             (result = actor_handle_death(actor_index, 0, 0)) == 0)) {
            actor_set_combat_alert_flag();
        }

        if ((actor_has_unshielded_threat_weapon(actor_index) == 0 || (threat->relationship_object_index == -1 && threat->has_parent == 0)) &&
            (a->mode != 10 || (*(int16_t *)(a->mode_data + (0xa0 - 0x9c)) != 2 && *(int16_t *)(a->mode_data + (0xa0 - 0x9c)) != 3)) &&
            result == 0 && a->swarm == 0 && a->active_unit_index == (datum_index)k_datum_index_none && a->unknown_5f2 != 2) {
            int32_t now = game_time->game_time;
            uint8_t use_alt = a->unknown_378;

            if (actor_has_unshielded_threat_weapon(actor_index) == 0 && (actor_def->flags & 0x20000) == 0) {
                use_alt = 1;
            }
            {
                float reaction_base = (a->unknown_378 == 0) ? *(float *)((uint8_t *)actor_def + 0x378) /* UNSURE: puVar3[0xde] */ : 0.0f;
                float sample_a = weapon_get_zoom_fov(0x15, *(int16_t *)(main_game_globals + 0x0e));
                float sample_b = weapon_get_zoom_fov(0x14, *(int16_t *)(main_game_globals + 0x0e));
                float wait = use_alt ? *(float *)((uint8_t *)unit_def + 0x170) : *(float *)((uint8_t *)unit_def + 0x160);

                if ((*(int32_t *)(actor_base + 0x37c) == -1 || *(int32_t *)(actor_base + 0x37c) + 10 < now) && threat_distance <= wait) {
                    if (a->unknown_1cb != 0) {
                        float berserk = *(float *)((uint8_t *)actor_def + 0x37c) /* UNSURE: puVar3[0xdf] */;
                        if (berserk < 0.0f) berserk = 0.0f;
                        if (berserk + 0.8f < threat_distance) {
                            goto after_alert_check;
                        }
                    }
                    if (*(int32_t *)(actor_base + 0x380) == -1 ||
                        (float)now >= (sample_b + sample_a * reaction_base) * 30.0f + (float)*(int32_t *)(actor_base + 0x380)) {
                        actor_has_unshielded_threat_weapon(actor_index);
                        *(int32_t *)(actor_base + 0x37c) = now;
                        if (actor_consider_combat_mode(actor_index, 2, (actor_combat_consideration *)consideration) != 0) {
                            actor_set_mode(actor_index, 10, consideration);
                            result = 1;
                        }
                    }
                }
            }
        }

    after_alert_check:
        if (a->mode == 10 || a->unknown_1cb != 0) {
            if (result != 0) {
                return result;
            }
        } else {
            if (result != 0) {
                return result;
            }
            if (a->unknown_15e > 0) {
                object *unit_obj = ((object_header *)object_data->data)[a->active_unit_index & 0xffff].data;
                Actor *unit_actor_def = (Actor *)tag_instances[unit_obj->definition_tag & 0xffff].data;

                if ((*(int32_t *)(actor_base + 0x388) == -1 ||
                     *(float *)((uint8_t *)unit_actor_def + 0x390) * 30.0f + (float)*(int32_t *)(actor_base + 0x388) < (float)game_time->game_time) &&
                    a->unknown_15e == 4 &&
                    unit_def->attacking_evasion_threshold /* UNSURE: iVar10+0x160 offset guessed */ < threat_distance &&
                    threat->unknown_38 == 0 &&
                    actor_consider_combat_mode(actor_index, 4, (actor_combat_consideration *)consideration) != 0) {
                    actor_set_mode(actor_index, 10, consideration);
                    return 1;
                }
            }
        }
    }

    {
        uint8_t bVar6 = (a->unknown_375 != 0 && a->unknown_1cb == 0);
        uint8_t bVar5 = 0;

        if (a->unknown_1cb == 0 && actor_has_unshielded_threat_weapon(actor_index) == 0 && (actor_def->flags & 0x1000000) != 0) {
            bVar6 = 1;
        }

        if (a->mode == 10) {
            int16_t sub = *(int16_t *)(a->mode_data + (0xa0 - 0x9c));
            if (sub == 2 || sub == 3) {
                if (a->mode_data[0xa3 - 0x9c] != 0 || a->mode_data[0xa4 - 0x9c] != 0 || a->mode_data[0xc5 - 0x9c] != 0) {
                    bVar5 = 1;
                    goto combined_gate;
                }
            } else {
                if (a->unknown_1cb != 0) {
                    goto finish_mode3;
                }
                if (sub != 4 && sub != 5) {
                    goto combined_gate_entry;
                }
                if (a->mode_data[0xc5 - 0x9c] != 0 || a->unknown_15e < 2) {
                    bVar5 = 1;
                    goto combined_gate;
                }
                if (sub == 4) {
                    uint8_t direct_hit = (a->movement_completed != 0 && a->active_movement.type == 5 &&
                                           *(int32_t *)((uint8_t *)&a->active_movement + 4) == (int32_t)a->target_unit_index);
                    Actor *unit_actor_def2;
                    {
                        object *unit_obj2 = ((object_header *)object_data->data)[a->active_unit_index & 0xffff].data;
                        unit_actor_def2 = (Actor *)tag_instances[unit_obj2->definition_tag & 0xffff].data;
                    }
                    if (direct_hit ||
                        threat_distance < *(float *)((uint8_t *)unit_actor_def2 + 0x394) ||
                        (threat_distance < 2.0f * *(float *)((uint8_t *)unit_actor_def2 + 0x394) &&
                         // REVIEW FIX: these three were read as uint8_t. The original reads
                         // them as floats (prop+0xe0..0xe8, a direction), and it dots them
                         // against actor.facing.
                         (*(float *)((uint8_t *)threat + 0xe0) * a->facing.i +
                          a->facing.j * *(float *)((uint8_t *)threat + 0xe4) +
                          a->facing.k * *(float *)((uint8_t *)threat + 0xe8) < 0.5f))) {
                        goto finish_mode3;
                    }
                }
            }
        finish_mode3_label:;
        combined_gate_entry:
            if (a->mode == 10) {
                return result;
            }
        } else {
        combined_gate:
            if (!bVar6) {
                goto finish_mode3;
            }
            if (!bVar5) {
                goto combined_gate_entry;
            }
        }

        {
            char changed = actor_consider_combat_mode(actor_index, 0, (actor_combat_consideration *)consideration);
            if (changed != 0) {
                actor_set_mode(actor_index, 10, consideration);
                return 1;
            }
        }
    finish_mode3:
        if (a->mode == 3) {
            return result;
        }
        consideration[0] = 0;
        actor_set_mode(actor_index, 3, consideration);
        return 1;
    }
}

#if 0
Original Ghidra decompilation (0x40c620):

/* WARNING: Removing unreachable block (ram,0x0040cc29) */

char actor_evaluate_combat_state_transition(uint param_1)

{
  float fVar1;
  int iVar2;
  uint *puVar3;
  int iVar4;
  bool bVar5;
  bool bVar6;
  char cVar7;
  char cVar8;
  short sVar9;
  int iVar10;
  int iVar11;
  float10 fVar12;
  float10 fVar13;
  char local_a2;
  float local_a0;
  float local_9c;
  int local_8c;
  undefined4 local_84 [33];
  
  iVar2 = *(int *)(DAT_00880360 + 0x34);
  iVar10 = (param_1 & 0xffff) * 0x724;
  iVar11 = iVar10 + iVar2;
  puVar3 = *(uint **)((*(uint *)(iVar10 + 0x58 + iVar2) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  iVar2 = *(int *)((*(uint *)(iVar10 + 0x5c + iVar2) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  iVar10 = actor_get_actor_definition();
  local_a2 = '\0';
  local_8c = 0;
  local_a0 = 3.4028235e+38;
  if (*(uint *)(iVar11 + 0x270) != 0xffffffff) {
    local_8c = (*(uint *)(iVar11 + 0x270) & 0xffff) * 0x138 + *(int *)(DAT_008802c0 + 0x34);
    local_a0 = *(float *)(local_8c + 0x11c);
    if ((((*(short *)(iVar11 + 0x6c) == 10) && (*(short *)(iVar11 + 0xa0) == 1)) &&
        (((*(char *)(local_8c + 0x74) != '\0' ||
          ((*(char *)(local_8c + 0x12f) != '\0' && (*(char *)(local_8c + 0x121) < '\x02')))) ||
         ((0.0 < (float)puVar3[0xca] && (sVar9 = __ftol(), sVar9 <= *(short *)(iVar11 + 0xc2)))))))
       && ((local_a0 <= *(float *)(iVar10 + 0xa0) ||
           (local_a2 = FUN_0040dd50(param_1,0,0), local_a2 == '\0')))) {
      actor_set_combat_alert_flag();
    }
    cVar7 = FUN_00428370();
    if (((cVar7 == '\0') ||
        ((*(int *)(local_8c + 0x110) == -1 && (*(char *)(local_8c + 0x14) == '\0')))) &&
       (((*(short *)(iVar11 + 0x6c) != 10 ||
         ((*(short *)(iVar11 + 0xa0) != 2 && (*(short *)(iVar11 + 0xa0) != 3)))) &&
        ((local_a2 == '\0' &&
         (((*(char *)(iVar11 + 6) == '\0' && (*(int *)(iVar11 + 0x158) == -1)) &&
          (*(short *)(iVar11 + 0x5f2) != 2)))))))) {
      iVar4 = *(int *)(DAT_006f1d6c + 0xc);
      cVar7 = *(char *)(iVar11 + 0x378);
      cVar8 = FUN_00428370();
      if ((cVar8 == '\0') && ((*puVar3 & 0x20000) == 0)) {
        cVar7 = '\x01';
      }
      if (*(char *)(iVar11 + 0x378) == '\0') {
        local_9c = (float)puVar3[0xde];
      }
      else {
        local_9c = 0.0;
      }
      fVar12 = (float10)FUN_0046fe10(0x15);
      fVar13 = (float10)FUN_0046fe10(0x14);
      if (cVar7 == '\0') {
        fVar1 = *(float *)(iVar2 + 0x160);
      }
      else {
        fVar1 = *(float *)(iVar2 + 0x170);
      }
      if (((*(int *)(iVar11 + 0x37c) == -1) || (*(int *)(iVar11 + 0x37c) + 10 < iVar4)) &&
         (local_a0 <= fVar1)) {
        if (*(char *)(iVar11 + 0x1cb) != '\0') {
          if (0.0 <= (float)puVar3[0xdf]) {
            fVar1 = (float)puVar3[0xdf];
          }
          else {
            fVar1 = 0.0;
          }
          if (fVar1 + 0.8 < local_a0) goto LAB_0040c946;
        }
        if ((*(int *)(iVar11 + 0x380) == -1) ||
           (fVar1 = (float)(fVar13 + (float10)(float)(fVar12 * (float10)local_9c)) * 30.0 +
                    (float)*(int *)(iVar11 + 0x380), (float)iVar4 < fVar1 == ((float)iVar4 == fVar1)
           )) {
          FUN_00428370();
          *(int *)(iVar11 + 0x37c) = iVar4;
          cVar7 = actor_consider_combat_mode(param_1,2,local_84);
          if (cVar7 != '\0') {
            actor_set_mode(param_1,10,local_84);
            local_a2 = '\x01';
          }
        }
      }
    }
LAB_0040c946:
    if ((*(short *)(iVar11 + 0x6c) == 10) || (*(char *)(iVar11 + 0x1cb) != '\0')) {
      if (local_a2 != '\0') {
        return local_a2;
      }
    }
    else {
      if (local_a2 != '\0') {
        return local_a2;
      }
      if ((0 < *(short *)(iVar11 + 0x15e)) &&
         (((((*(int *)(iVar11 + 0x388) == -1 ||
             (*(float *)(*(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                              (*(uint *)(iVar11 + 0x158) & 0xffff) * 0xc) & 0xffff)
                                  * 0x20 + 0x14 + DAT_0087bc14) + 0x390) * 30.0 +
              (float)*(int *)(iVar11 + 0x388) < (float)*(int *)(DAT_006f1d6c + 0xc))) &&
            (*(short *)(iVar11 + 0x15e) == 4)) &&
           ((*(float *)(iVar10 + 0x160) < local_a0 && (*(short *)(local_8c + 0x38) == 0)))) &&
          (cVar7 = actor_consider_combat_mode(param_1,4,local_84), cVar7 != '\0')))) {
        actor_set_mode(param_1,10,local_84);
        return '\x01';
      }
    }
  }
  if ((*(char *)(iVar11 + 0x375) == '\0') || (bVar6 = true, *(char *)(iVar11 + 0x1cb) != '\0')) {
    bVar6 = false;
  }
  bVar5 = false;
  if (((*(char *)(iVar11 + 0x1cb) == '\0') && (cVar7 = FUN_00428370(), cVar7 == '\0')) &&
     ((*puVar3 & 0x1000000) != 0)) {
    bVar6 = true;
  }
  if (*(short *)(iVar11 + 0x6c) == 10) {
    sVar9 = *(short *)(iVar11 + 0xa0);
    if ((sVar9 == 2) || (sVar9 == 3)) {
      if ((*(char *)(iVar11 + 0xa3) != '\0') ||
         ((*(char *)(iVar11 + 0xa4) != '\0' || (*(char *)(iVar11 + 0xc5) != '\0')))) {
LAB_0040cbe0:
        bVar5 = true;
        goto LAB_0040cbe2;
      }
    }
    else {
      if (*(char *)(iVar11 + 0x1cb) != '\0') goto LAB_0040cc2d;
      if ((sVar9 != 4) && (sVar9 != 5)) goto LAB_0040cbe2;
      if ((*(char *)(iVar11 + 0xc5) != '\0') || (*(short *)(iVar11 + 0x15e) < 2)) goto LAB_0040cbe0;
      if ((sVar9 == 4) &&
         ((((iVar2 = *(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                          (*(uint *)(iVar11 + 0x158) & 0xffff) * 0xc) & 0xffff) *
                              0x20 + 0x14 + DAT_0087bc14), *(char *)(iVar11 + 0x484) != '\0' &&
            (*(short *)(iVar11 + 0x46c) == 5)) &&
           (*(int *)(iVar11 + 0x470) == *(int *)(iVar11 + 0x270))) ||
          ((local_a0 < *(float *)(iVar2 + 0x394) ||
           ((fVar1 = *(float *)(iVar2 + 0x394), local_a0 < fVar1 + fVar1 &&
            (*(float *)(local_8c + 0xe0) * *(float *)(iVar11 + 0x174) +
             *(float *)(iVar11 + 0x178) * *(float *)(local_8c + 0xe4) +
             *(float *)(iVar11 + 0x17c) * *(float *)(local_8c + 0xe8) < 0.5))))))))
      goto LAB_0040cc2d;
    }
LAB_0040cbee:
    if (*(short *)(iVar11 + 0x6c) == 10) {
      return local_a2;
    }
  }
  else {
LAB_0040cbe2:
    if (!bVar6) goto LAB_0040cc2d;
    if (!bVar5) goto LAB_0040cbee;
  }
  cVar7 = actor_consider_combat_mode(param_1,0,local_84);
  if (cVar7 != '\0') {
    actor_set_mode(param_1,10,local_84);
    return 1;
  }
LAB_0040cc2d:
  if (*(short *)(iVar11 + 0x6c) == 3) {
    return local_a2;
  }
  local_84[0] = 0;
  actor_set_mode(param_1,3,local_84);
  return '\x01';
}
#endif
