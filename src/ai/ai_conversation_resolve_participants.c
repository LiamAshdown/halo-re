// ai_conversation_resolve_participants  (Ghidra: ai_conversation_resolve_participants; named for this rewrite)
// address 0x430fc0, size 1471 bytes
// name confidence: 0.4   rewrite confidence: 0.85 (VERIFIED end to end against 0x430fc0 (both participant passes with the stack-slot roles, readiness and trigger checks, nearest-player and player-looking scans, apply loop))
// evidence: out/phase4/ai_types_notes.md's misattribution table places the
// 0x430830..0x431e70 block on Scenario.ai_conversations; this function clears and refills
// ai_conversation.participant_mask / .participant_actor[8] / the int16 variant array at
// +0x18, once per participant, by calling ai_conversation_resolve_participant (0x431680),
// and then gates "is this conversation ready to play" on the definition's own flag word
// (ScenarioAIConversationFlags at ScenarioAIConversation+0x20). Every flag bit it tests
// lines up with a named bit of that enum: 0x10 player_must_be_visible, 0x20
// stop_other_actions, 0x40 keep_trying_to_play, 0x80 player_must_be_looking. Its two callers
// are ai_conversation_activate (0x4307c0) and ai_conversation_update (0x430a70), which
// already carry `extern int8_t ai_conversation_resolve_participants(datum_index, uint8_t *)`.
// register convention: plain __cdecl, two stack arguments.
// blam-cc: stack -> conversation_index, out_keep_trying
//
// UNSURE:
//  - ai_conversation.unknown_10 is the object index of the player the conversation is
//    waiting on (set from the nearest player's unit on the player_must_be_visible path,
//    and forced back to none on every pass). Named `waiting_on_object_index` in the
//    TYPES-GAP note but left as unknown_10 here.
//  - ai_conversation_get_run_to_player_range (0x402cf0) is shown by Ghidra with no arguments at all and returns its
//    result in AL; the two values it consumes are not recovered. Declared void-arg.
//  - unit_point_within_look_cone is called with the single literal 0x3f060a92 == 0.5235988f, i.e. 30
//    degrees in radians, which is consistent with a "is the player looking within this
//    cone" test; the object/actor arguments Ghidra dropped are not recovered.
//  - Ghidra reuses its `local_a4` float local as an object pointer in the apply loop and
//    its `local_a0` mask local as that loop's counter; both are split into properly typed
//    locals below.
//  - object+0x204 bit 8 is cleared whenever the unit's variant index (object+0xbe) changes;
//    the same +0x204 flag word is already used this way in
//    src/ai/actor_apply_unit_definition_properties.c.
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "ai.h"
#include <stdint.h>

extern data_array *ai_conversation_data; // 0x008802d4
extern data_array *actor_data;           // 0x00880360
extern data_array *object_data;          // 0x008603b0
extern data_array *prop_data;            // 0x008802c0
extern data_array *player_data;          // 0x0087a480, stride 0x200
extern Scenario *global_scenario;        // 0x00746f8c
extern datum_index *object_names_to_objects; // 0x006b8cb8, 0x200 entries

extern void * data_iterator_next(data_iterator *iterator); // 0x4d05d0
extern void actor_set_mode(datum_index actor_index, int32_t mode, void *mode_data); // 0x40d8d0
extern datum_index actor_find_prop_for_object(datum_index object_index, datum_index actor_index); // 0x43ea80, stack, ECX
extern int32_t ai_conversation_get_run_to_player_range(ai_conversation_range_lookup *out, uint32_t conversation_index); // 0x402cf0, EDX, ESI
extern uint8_t unit_point_within_look_cone(float cone_angle, uint32_t unit_index, real_point3d *world_point); // 0x56c100, stack, ECX, EDI
extern int8_t ai_conversation_resolve_participant(int16_t participant_index, uint8_t *out_resolved,
                                                  uint8_t *out_wants_alternate,
                                                  uint8_t *out_blocked_by_player,
                                                  float *inout_minimum_distance,
                                                  datum_index conversation_index); // 0x431680

// blam-cc: stack -> conversation_index, out_keep_trying
// Rebuilds the whole participant table of a running conversation from scratch: resolves
// every non-alternate participant, then (only if one of them asked for it) every alternate
// participant, then decides whether the conversation can actually run -- every required
// participant must be filled, a player must be close enough when player_must_be_visible is
// set, and looking when player_must_be_looking is set. When it can run, each participant's
// unit is stamped with its chosen dialogue variant, optionally renamed into the scenario
// object-name table, and optionally switched into the conversation mode. Returns 1 when the
// conversation is ready; *out_keep_trying reports the definition's keep_trying_to_play bit.
uint8_t ai_conversation_resolve_participants(datum_index conversation_index,
                                             uint8_t *out_keep_trying)
{
    ai_conversation *instance;
    ScenarioAIConversation *definition;
    ScenarioAIConversationParticipant *participants;
    int16_t *variant_slots;
    uint32_t blocked_mask;
    uint8_t any_resolved;
    uint8_t wants_alternate;
    uint8_t alternate_resolved;
    uint8_t ready;
    uint8_t keep_trying;
    uint8_t blocked_this_slot;
    float minimum_distance;
    int32_t i;
    int16_t index;
    uint16_t participant_flags;
    uint32_t bit;
    uint8_t found_looking;
    data_iterator iterator;
    void *player;
    float nearest;
    float best_player_distance;
    datum_index player_unit;
    datum_index prop_index;
    prop *p;
    int32_t j;
    datum_index actor_handle;
    datum_index unit_index;
    object *unit_object;
    int16_t object_name;
    int16_t variant;
    uint16_t definition_flags;

    instance = (ai_conversation *)((uint8_t *)ai_conversation_data->data +
                                   (conversation_index & 0xffff) * k_ai_conversation_size);
    definition = (ScenarioAIConversation *)((uint8_t *)(uintptr_t)
                                                global_scenario->ai_conversations.pointer +
                                            (int32_t)instance->definition_index * 0x74);
    participants = (ScenarioAIConversationParticipant *)(uintptr_t)definition->participants.pointer;
    variant_slots = (int16_t *)((uint8_t *)instance + 0x18);

    instance->participant_mask = 0;
    for (i = 0; i < 8; i++) {
        instance->participant_actor[i] = (datum_index)k_datum_index_none;
    }
    for (i = 0; i < 8; i++) {
        variant_slots[i] = -1;
    }

    blocked_mask = 0;
    wants_alternate = 0;
    alternate_resolved = 0;
    any_resolved = 0;
    ready = 1;
    keep_trying = 0;
    minimum_distance = 3.4028235e+38f;

    if (0 < (int32_t)definition->participants.count) {
        index = 0;
        i = 0;
        do {
            // Pass 1: every participant that is not itself an alternate.
            if ((participants[i].flags & 4) == 0) {
                blocked_this_slot = 0;
                ai_conversation_resolve_participant(index, &any_resolved, &wants_alternate,
                                                    &blocked_this_slot, &minimum_distance,
                                                    conversation_index);
                if (blocked_this_slot == 0) {
                    blocked_mask = blocked_mask & ~(1u << ((uint8_t)i & 0x1f));
                } else {
                    blocked_mask = blocked_mask | (1u << ((uint8_t)i & 0x1f));
                }
            }
            index = (int16_t)(index + 1);
            i = (int32_t)index;
        } while (i < (int32_t)definition->participants.count);

        if (wants_alternate != 0 && 0 < (int32_t)definition->participants.count) {
            index = 0;
            i = 0;
            do {
                // Pass 2: the alternates, but only for slots still empty.
                bit = 1u << ((uint8_t)i & 0x1f);
                if ((bit & instance->participant_mask) == 0 && (participants[i].flags & 4) != 0) {
                    blocked_this_slot = 0;
                    if (ai_conversation_resolve_participant(index, &any_resolved, 0,
                                                            &blocked_this_slot, &minimum_distance,
                                                            conversation_index) == 0) {
                        if (blocked_this_slot == 0) {
                            blocked_mask = blocked_mask & ~bit;
                        } else {
                            blocked_mask = blocked_mask | bit;
                        }
                    } else {
                        alternate_resolved = 1;
                    }
                }
                index = (int16_t)(index + 1);
                i = (int32_t)index;
            } while (i < (int32_t)definition->participants.count);
        }
    }

    // A conversation is not ready while any required participant is still unfilled.
    index = 0;
    if (0 < (int32_t)definition->participants.count) {
        i = 0;
        do {
            participant_flags = participants[i].flags;
            if ((participant_flags & 1) == 0 &&
                (instance->participant_mask & (1u << ((uint8_t)i & 0x1f))) == 0 &&
                ((participant_flags & 2) == 0 || alternate_resolved == 0) &&
                ((participant_flags & 4) == 0 || wants_alternate != 0)) {
                ready = 0;
                if ((blocked_mask & (1u << ((uint8_t)index & 0x1f))) == 0) {
                    goto clear_wait;
                }
                goto blocked_but_trying;
            }
            index = (int16_t)(index + 1);
            i = (int32_t)index;
        } while (i < (int32_t)definition->participants.count);
    }

    if ((definition->flags & 0x40) != 0 && 0.0f < definition->trigger_distance &&
        any_resolved != 0 && definition->trigger_distance < minimum_distance) {
blocked_but_trying:
        ready = 0;
        keep_trying = 1;
    }

clear_wait:
    instance->unknown_10 = -1;
    if (ready == 0) {
        goto check_keep_trying;
    }

    if ((definition->flags & 0x10) == 0) {
        goto check_looking;
    }

    // player_must_be_visible: pick the player whose nearest-participant prop distance is
    // smallest and remember its unit, failing the whole conversation when there is none.
    if (any_resolved == 0) {
        ready = 0;
        goto check_keep_trying;
    }
    iterator.data = player_data;
    iterator.next_index = 0;
    iterator.index = (datum_index)k_datum_index_none;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
    best_player_distance = 3.4028235e+38f;
    player = data_iterator_next(&iterator);
    while (player != 0) {
        player_unit = *(datum_index *)((uint8_t *)player + 0x34);
        if (player_unit != (datum_index)k_datum_index_none) {
            nearest = 3.4028235e+38f;
            for (j = 0; j < (int32_t)definition->participants.count; j++) {
                if (instance->participant_actor[j] != (datum_index)k_datum_index_none) {
                    prop_index = actor_find_prop_for_object(player_unit, instance->participant_actor[j]); // 0x4312b0: ECX = participant
                    if (prop_index != (datum_index)k_datum_index_none) {
                        p = (prop *)((uint8_t *)prop_data->data +
                                     (prop_index & 0xffff) * k_prop_size);
                        if (1 < p->kind && p->kind < 4 && p->distance < nearest) {
                            nearest = p->distance;
                        }
                    }
                }
            }
            if (nearest < best_player_distance) {
                best_player_distance = nearest;
                instance->unknown_10 = (int32_t)player_unit;
            }
        }
        player = data_iterator_next(&iterator);
    }
    if (instance->unknown_10 != -1) {
        goto check_looking;
    }
    definition_flags = definition->flags;
    goto not_ready_check_retry;

check_looking:
    if ((definition->flags & 0x80) == 0 || any_resolved == 0) {
        goto apply;
    }
    // player_must_be_looking: any player looking at any resolved participant is enough.
    iterator.data = player_data;
    iterator.next_index = 0;
    iterator.index = (datum_index)k_datum_index_none;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
    found_looking = 0;
    player = data_iterator_next(&iterator);
    while (player != 0) {
        if (found_looking != 0) {
            goto apply;
        }
        if (*(datum_index *)((uint8_t *)player + 0x34) != (datum_index)k_datum_index_none) {
            for (j = 0; j < (int32_t)definition->participants.count; j++) {
                // 0x4313c0: ECX = the player's unit, EDI = the participant actor's +0x120 point
                if (instance->participant_actor[j] != (datum_index)k_datum_index_none &&
                    unit_point_within_look_cone(0.5235988f, *(datum_index *)((uint8_t *)player + 0x34),
                        (real_point3d *)((uint8_t *)actor_data->data +
                            (instance->participant_actor[j] & 0xffff) * k_actor_size + 0x120)) != 0) {
                    found_looking = 1;
                    break;
                }
            }
        }
        player = data_iterator_next(&iterator);
    }
    if (found_looking != 0) {
        goto apply;
    }
    definition_flags = definition->flags;

not_ready_check_retry:
    // Ghidra's fallthrough out of both "player gate failed" paths: when the definition
    // wants to keep retrying, skip the keep_trying test below and report it directly.
    ready = 0;
    if ((definition_flags & 0x40) == 0) {
        goto check_keep_trying;
    }
    goto report_keep_trying;

check_keep_trying:                      // Ghidra LAB_00431202
    if (keep_trying == 0) {
        goto report_not_trying;         // Ghidra LAB_00431573
    }

report_keep_trying:
    if ((definition->flags & 0x40) != 0) {
        *out_keep_trying = 1;
        return ready;
    }

report_not_trying:
    *out_keep_trying = 0;
    return ready;

apply:
    for (i = 0; i < (int32_t)definition->participants.count; i++) {
        if ((instance->participant_mask & (1u << ((uint8_t)i & 0x1f))) == 0) {
            continue;
        }
        actor_handle = instance->participant_actor[i];
        if (actor_handle == (datum_index)k_datum_index_none) {
            continue;
        }
        unit_index = ((actor *)((uint8_t *)actor_data->data +
                                (actor_handle & 0xffff) * k_actor_size))->unit_index;
        unit_object = ((object_header *)object_data->data)[unit_index & 0xffff].data;

        object_name = (int16_t)participants[i].set_new_name;
        if (object_name != -1 && object_name >= 0 &&
            (int32_t)object_name < (int32_t)global_scenario->object_names.count) {
            object_names_to_objects[object_name] = unit_index;
        }
        // 0x4314f2: the conversation mode data (0x14 bytes, mode 12's data_size) comes from 0x402cf0 (EDX = out,
        //   ESI = the conversation); the draft entered conversation mode with NULL, leaving stale mode data.
        if ((definition->flags & 0x20) != 0) {
            ai_conversation_range_lookup mode_data;

            if (ai_conversation_get_run_to_player_range(&mode_data, conversation_index) != 0) {
                actor_set_mode(instance->participant_actor[i], _actor_mode_conversation, &mode_data);
            }
        }
        variant = (int16_t)participants[i].variant_numbers[variant_slots[i]];
        if (*(int16_t *)((uint8_t *)unit_object + 0xbe) != variant) {
            *(int16_t *)((uint8_t *)unit_object + 0xbe) = variant;
            *(uint32_t *)((uint8_t *)unit_object + 0x204) =
                *(uint32_t *)((uint8_t *)unit_object + 0x204) & 0xfffffeffu;
        }
    }
    instance->unknown_06 = 1;
    return ready;
}

#if 0
Original Ghidra decompilation (0x430fc0):

uint FUN_00430fc0(uint param_1,undefined1 *param_2)

{
  byte bVar1;
  ushort uVar2;
  char cVar3;
  short sVar4;
  undefined4 extraout_EDX;
  int iVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  uint uVar9;
  float10 fVar10;
  float10 extraout_ST0;
  byte local_a9;
  char local_a8;
  char local_a7;
  char local_a6;
  char local_a5;
  float local_a4;
  uint local_a0;
  char local_99;
  int local_98;
  uint local_94;
  undefined2 local_90;
  undefined4 local_8c;
  uint local_88;

  iVar6 = (param_1 & 0xffff) * 100 + *(int *)(DAT_008802d4 + 0x34);
  iVar7 = *(short *)(iVar6 + 2) * 0x74 + *(int *)(global_scenario + 0x46c);
  *(undefined4 *)(iVar6 + 0x14) = 0;
  *(undefined4 *)(iVar6 + 0x28) = 0xffffffff;
  *(undefined4 *)(iVar6 + 0x2c) = 0xffffffff;
  *(undefined4 *)(iVar6 + 0x30) = 0xffffffff;
  *(undefined4 *)(iVar6 + 0x34) = 0xffffffff;
  *(undefined4 *)(iVar6 + 0x38) = 0xffffffff;
  *(undefined4 *)(iVar6 + 0x3c) = 0xffffffff;
  *(undefined4 *)(iVar6 + 0x40) = 0xffffffff;
  *(undefined4 *)(iVar6 + 0x44) = 0xffffffff;
  *(undefined4 *)(iVar6 + 0x18) = 0xffffffff;
  *(undefined4 *)(iVar6 + 0x1c) = 0xffffffff;
  *(undefined4 *)(iVar6 + 0x20) = 0xffffffff;
  *(undefined4 *)(iVar6 + 0x24) = 0xffffffff;
  local_a0 = 0;
  local_a8 = '\0';
  local_99 = '\0';
  local_a7 = '\0';
  local_a9 = 1;
  local_a5 = '\0';
  local_a4 = 3.4028235e+38;
  local_98 = iVar7;
  if (0 < *(int *)(iVar7 + 0x50)) {
    iVar8 = 0;
    iVar5 = 0;
    do {
      if ((*(byte *)(iVar8 * 0x54 + 2 + *(int *)(iVar7 + 0x54)) & 4) == 0) {
        local_a6 = '\0';
        FUN_00431680(iVar5,&local_a7,&local_a8,&local_a6,&local_a4);
        if (local_a6 == '\0') {
          local_a0 = local_a0 & ~(1 << ((byte)iVar8 & 0x1f));
        }
        else {
          local_a0 = local_a0 | 1 << ((byte)iVar8 & 0x1f);
        }
      }
      iVar5 = iVar5 + 1;
      iVar8 = (int)(short)iVar5;
    } while (iVar8 < *(int *)(iVar7 + 0x50));
    if ((local_a8 != '\0') && (iVar5 = 0, 0 < *(int *)(iVar7 + 0x50))) {
      iVar8 = 0;
      do {
        uVar9 = 1 << ((byte)iVar8 & 0x1f);
        if (((uVar9 & *(uint *)(iVar6 + 0x14)) == 0) &&
           ((*(byte *)(iVar8 * 0x54 + 2 + *(int *)(iVar7 + 0x54)) & 4) != 0)) {
          local_a6 = '\0';
          cVar3 = FUN_00431680(iVar5,&local_a7,0,&local_a6,&local_a4);
          if (cVar3 == '\0') {
            if (local_a6 == '\0') {
              local_a0 = local_a0 & ~uVar9;
            }
            else {
              local_a0 = local_a0 | uVar9;
            }
          }
          else {
            local_99 = '\x01';
          }
        }
        iVar5 = iVar5 + 1;
        iVar8 = (int)(short)iVar5;
      } while (iVar8 < *(int *)(iVar7 + 0x50));
    }
  }
  sVar4 = 0;
  if (0 < *(int *)(iVar7 + 0x50)) {
    iVar5 = 0;
    do {
      uVar2 = *(ushort *)(iVar5 * 0x54 + *(int *)(iVar7 + 0x54) + 2);
      if (((((uVar2 & 1) == 0) && ((*(uint *)(iVar6 + 0x14) & 1 << ((byte)iVar5 & 0x1f)) == 0)) &&
          (((uVar2 & 2) == 0 || (local_99 == '\0')))) && (((uVar2 & 4) == 0 || (local_a8 != '\0'))))
      {
        local_a9 = 0;
        if ((local_a0 & 1 << ((byte)sVar4 & 0x1f)) == 0) goto LAB_004311dc;
        goto LAB_004311d7;
      }
      sVar4 = sVar4 + 1;
      iVar5 = (int)sVar4;
    } while (iVar5 < *(int *)(iVar7 + 0x50));
  }
  if (((((*(byte *)(iVar7 + 0x20) & 0x40) != 0) && (0.0 < *(float *)(iVar7 + 0x24))) &&
      (local_a7 != '\0')) && (*(float *)(iVar7 + 0x24) < local_a4)) {
LAB_004311d7:
    local_a9 = 0;
    local_a5 = '\x01';
  }
LAB_004311dc:
  *(undefined4 *)(iVar6 + 0x10) = 0xffffffff;
  if (local_a9 == 0) {
LAB_00431202:
    if (local_a5 == '\0') goto LAB_00431573;
  }
  else {
    if ((*(byte *)(iVar7 + 0x20) & 0x10) == 0) {
LAB_00431350:
      if ((-1 < *(char *)(iVar7 + 0x20)) || (local_a7 == '\0')) {
LAB_00431442:
        iVar5 = *(int *)(iVar7 + 0x50);
        local_a0 = 0;
        if (0 < iVar5) {
          iVar8 = 0;
          do {
            if (((*(uint *)(iVar6 + 0x14) & 1 << ((byte)iVar8 & 0x1f)) != 0) &&
               (uVar9 = *(uint *)(iVar6 + 0x28 + iVar8 * 4), uVar9 != 0xffffffff)) {
              iVar5 = iVar8 * 0x54 + *(int *)(iVar7 + 0x54);
              uVar9 = *(uint *)((uVar9 & 0xffff) * 0x724 + 0x18 + *(int *)(DAT_00880360 + 0x34));
              local_a4 = *(float *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar9 & 0xffff) * 0xc);
              sVar4 = *(short *)(iVar5 + 10);
              if ((sVar4 != -1) &&
                 ((-1 < sVar4 && (iVar7 = local_98, (int)sVar4 < *(int *)(global_scenario + 0x204)))
                 )) {
                *(uint *)(DAT_006b8cb8 + sVar4 * 4) = uVar9;
              }
              if (((*(byte *)(iVar7 + 0x20) & 0x20) != 0) &&
                 (cVar3 = FUN_00402cf0(), iVar7 = local_98, cVar3 != '\0')) {
                actor_set_mode(*(undefined4 *)(iVar6 + 0x28 + iVar8 * 4),0xc,extraout_EDX);
                iVar7 = local_98;
              }
              sVar4 = *(short *)(iVar5 + 0x18 + *(short *)(iVar6 + 0x18 + iVar8 * 2) * 2);
              if (*(short *)((int)local_a4 + 0xbe) != sVar4) {
                *(short *)((int)local_a4 + 0xbe) = sVar4;
                *(uint *)((int)local_a4 + 0x204) = *(uint *)((int)local_a4 + 0x204) & 0xfffffeff;
              }
            }
            iVar5 = local_a0 + 1;
            iVar8 = (int)(short)iVar5;
            local_a0 = iVar5;
          } while (iVar8 < *(int *)(iVar7 + 0x50));
        }
        *(undefined1 *)(iVar6 + 6) = 1;
        return CONCAT31((int3)((uint)iVar5 >> 8),local_a9);
      }
      local_94 = DAT_0087a480;
      local_88 = DAT_0087a480 ^ 0x69746572;
      local_a8 = '\0';
      local_90 = 0;
      local_8c = 0xffffffff;
      iVar5 = data_iterator_next();
      if (iVar5 != 0) {
        do {
          if (local_a8 != '\0') goto LAB_00431442;
          if (*(int *)(iVar5 + 0x34) != -1) {
            iVar5 = 0;
            local_a4 = 0.0;
            if (0 < *(int *)(iVar7 + 0x50)) {
              do {
                if ((*(int *)(iVar6 + 0x28 + iVar5 * 4) != -1) &&
                   (cVar3 = FUN_0056c100(0x3f060a92), cVar3 != '\0')) {
                  local_a8 = '\x01';
                  break;
                }
                local_a4 = (float)((int)local_a4 + 1);
                iVar5 = (int)SUB42(local_a4,0);
              } while (iVar5 < *(int *)(iVar7 + 0x50));
            }
          }
          iVar5 = data_iterator_next();
        } while (iVar5 != 0);
        if (local_a8 != '\0') goto LAB_00431442;
      }
      bVar1 = *(byte *)(iVar7 + 0x20);
    }
    else {
      if (local_a7 == '\0') {
        local_a9 = 0;
        goto LAB_00431202;
      }
      local_94 = DAT_0087a480;
      local_88 = DAT_0087a480 ^ 0x69746572;
      local_a4 = 3.4028235e+38;
      local_90 = 0;
      local_8c = 0xffffffff;
      iVar5 = data_iterator_next();
      while (iVar5 != 0) {
        iVar5 = *(int *)(iVar5 + 0x34);
        if (iVar5 != -1) {
          fVar10 = (float10)3.4028235e+38;
          sVar4 = 0;
          if (0 < *(int *)(iVar7 + 0x50)) {
            iVar8 = 0;
            do {
              if ((*(int *)(iVar6 + 0x28 + iVar8 * 4) != -1) &&
                 (uVar9 = FUN_0043ea80(iVar5), fVar10 = extraout_ST0, uVar9 != 0xffffffff)) {
                iVar8 = (uVar9 & 0xffff) * 0x138 + *(int *)(DAT_008802c0 + 0x34);
                if ((1 < *(short *)(iVar8 + 0x24)) &&
                   ((*(short *)(iVar8 + 0x24) < 4 &&
                    ((float10)*(float *)(iVar8 + 0x11c) < extraout_ST0)))) {
                  fVar10 = (float10)*(float *)(iVar8 + 0x11c);
                }
              }
              sVar4 = sVar4 + 1;
              iVar8 = (int)sVar4;
            } while (iVar8 < *(int *)(iVar7 + 0x50));
          }
          if (fVar10 < (float10)local_a4) {
            local_a4 = (float)fVar10;
            *(int *)(iVar6 + 0x10) = iVar5;
          }
        }
        iVar5 = data_iterator_next();
      }
      if (*(int *)(iVar6 + 0x10) != -1) goto LAB_00431350;
      bVar1 = *(byte *)(iVar7 + 0x20);
    }
    local_a9 = 0;
    if ((bVar1 & 0x40) == 0) goto LAB_00431202;
  }
  if ((*(byte *)(iVar7 + 0x20) & 0x40) != 0) {
    *param_2 = 1;
    return (uint)local_a9;
  }
LAB_00431573:
  *param_2 = 0;
  return (uint)local_a9;
}
#endif
