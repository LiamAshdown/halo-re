// ai_conversation_current_line_is_ready  (Ghidra: ai_conversation_current_line_is_ready; named for this rewrite)
// address 0x431e70, size 646 bytes
// name confidence: 0.4   rewrite confidence: 0.3
// evidence: out/phase4/ai_types_notes.md's misattribution table: "whether the activated
// squad member is ready to be placed" is actually "whether the participant is ready to
// speak". Only caller is ai_conversation_update (0x430a70, already rewritten), which passes
// the instance handle through in EAX exactly like ai_conversation_activate_next_participant
// (0x431d10, this batch; confirmed by objdump, `mov esi,eax` is the first instruction here
// too). types/ai.h ai_conversation fields all match one-to-one. types/units.h unit_speech
// (0x30 bytes, "0x560f20 block-moves 0xc dwords of this into unit 0x388") is the local
// stack block this function builds byte-for-byte before calling unit_commit_speech -- confirmed
// by objdump (bin/halo.exe 0x431f60..0x432045): priority=6, scream_type=-1, sound_tag =
// ai_conversation.unknown_5c, unknown_14=-1, ai_line_index=-1, unknown_18=-1 all line up
// with the named fields, though this call also writes non-zero values into three dwords of
// the struct's documented-always-zero tail (see TYPES-GAP below). objdump also confirms
// unit_commit_speech is called with ECX -> &request and EAX still holding the unit index from the
// mov two instructions earlier (matching the EAX -> object_index role already declared for
// this callee in ai_communication_play_event_line.c); EDX carries only a stale value here
// and is not treated as a real argument.
// register convention: EAX -> instance_handle (confirmed by objdump).
//   // blam-cc: EAX -> instance_handle
//
// TYPES-GAP: the request this function builds writes real values (1, 1, and the unit index)
// into unit_speech's unknown_1c tail, which types/units.h documents as always zero for the
// other constructor (0x561030). Reproduced exactly via a local struct definition that
// extends unit_speech's tail with three named fields, flagged UNSURE rather than folded
// into units.h.
// UNSURE: actor.mode_data sub-fields at +0x4/+0x5/+0xc (absolute actor+0xa0/+0xa1/+0xa8)
// have no established meaning; kept as raw offsets into mode_data. object+0x388 (unit_data
// current_speech.sound_tag, read back here as an int16 category compared against 6) and the
// DAT_00725204 tick threshold are likewise not otherwise attributed.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "game.h"
#include "ai.h"
#include <string.h>

// TYPES-GAP: mirrors types/units.h unit_speech (0x30 bytes) but names the tail three fields
// this call site actually writes (documented as always-zero padding for the other builder).
extern data_array *ai_conversation_data; // 0x008802d4
extern data_array *actor_data;           // 0x00880360
extern data_array *object_data;          // 0x008603b0
extern Scenario *global_scenario;        // 0x00746f8c
extern game_time_globals *game_time;     // 0x006f1d6c
extern int32_t DAT_00725204;             // 0x00725204, UNSURE: a tick threshold before conversations can proceed

// blam-cc: ECX -> sound_ref, EAX -> unused (-1), stack -> volume
extern int32_t sound_impulse_start(uint32_t sound_ref, uint32_t unused, float volume); // 0x543e10, UNSURE args
extern int32_t sound_impulse_time(void); // 0x543fc0, UNSURE args/return
// blam-cc: stack -> reachability_kind, unit_query, unused, out_a, out_b
extern int16_t unit_animation_change_priority_check(int32_t reachability_kind, int32_t unit_query, int32_t unused,
                             uint32_t *out_a, uint32_t *out_b); // 0x560d00, this session (also called
                             // from actor_squad_action_execute.c with the same signature)
// blam-cc: EAX -> object_index (unit index), ECX -> request
extern void unit_commit_speech(ai_conversation_speech_request *request); // 0x560f20, UNSURE args (see file header)

// blam-cc: EAX -> instance_handle
// Gates whether the current line of a live ai_conversation instance is ready to play: first
// (once per instance, cached in unknown_61) whether every participant that needs to be
// physically present has arrived (mode 12, "in conversation", and settled per its mode
// data), and if so, whether the speaker's unit needs to travel to speak (issuing the
// movement/speech request via unit_commit_speech when it does); then (cached in unknown_62)
// whether the speaking unit's current-speech category allows a new line; then counts down
// the line's delay in unknown_4c; and finally, if the line waits for an external
// confirmation (flag 8), holds at not-ready until that confirmation flag is set.
uint8_t ai_conversation_current_line_is_ready(datum_index instance_handle)
{
    ai_conversation *instance = &((ai_conversation *)ai_conversation_data->data)[instance_handle & 0xffff];
    ScenarioAIConversation *definition =
        &((ScenarioAIConversation *)global_scenario->ai_conversations.pointer)[instance->definition_index];

    if (instance->unknown_63 != 0) {
        return instance->unknown_63;
    }

    if (instance->unknown_61 == 0) {
        uint8_t someone_still_arriving = 0;

        if (instance->unknown_5c != (uint32_t)k_datum_index_none) {
            uint16_t line_flags = instance->unknown_4e;
            uint16_t wait_speaker_nearby = line_flags & 0x10;

            if ((line_flags & 0x10) != 0 || (line_flags & 0x20) != 0) {
                int32_t participant_count = definition->participants.count;
                int32_t i;
                for (i = 0; i < participant_count; i++) {
                    datum_index participant_actor_handle = instance->participant_actor[i];
                    if (participant_actor_handle != (datum_index)k_datum_index_none) {
                        actor *a = &((actor *)actor_data->data)[participant_actor_handle & 0xffff];
                        if (((line_flags & 0x20) != 0 ||
                             (wait_speaker_nearby != 0 && participant_actor_handle == instance->unknown_50)) &&
                            a->mode == _actor_mode_conversation) {
                            uint8_t *mode_data = a->mode_data;
                            if (*(int32_t *)(mode_data + 0xc) != (int32_t)k_datum_index_none &&
                                mode_data[5] == 0 && mode_data[4] == 0) {
                                someone_still_arriving = 1;
                            }
                        }
                    }
                }
            }
        }

        if (game_time->game_time < DAT_00725204 || someone_still_arriving != 0) {
            goto check_second_stage;
        }

        if (instance->unknown_54 == (uint32_t)k_datum_index_none || instance->unknown_60 != 0) {
            sound_impulse_start(instance->unknown_5c, (uint32_t)k_datum_index_none, 1.0f);
        } else {
            uint32_t out_a = (uint32_t)k_datum_index_none;
            uint32_t out_b = instance->unknown_5c;
            int16_t reach_result = unit_animation_change_priority_check(6, 1, 0, &out_a, &out_b);

            if (reach_result == 1) {
                goto check_second_stage;
            }
            if (reach_result > 0) {
                ai_conversation_speech_request request;
                memset(&request, 0, sizeof(request));
                request.sound_tag = instance->unknown_5c;
                request.unknown_10 = instance->unknown_58;
                request.unsure_unit_index = instance->unknown_54;
                request.priority = 6;
                request.scream_type = (int16_t)k_datum_index_none;
                request.unknown_14 = (int16_t)k_datum_index_none;
                request.ai_line_index = (int16_t)k_datum_index_none;
                request.unknown_18 = (int16_t)k_datum_index_none;
                request.unsure_flag_a = 1;
                request.unsure_flag_b = 1;
                unit_commit_speech(&request);
            }
        }

        instance->unknown_61 = 1;
        instance->unknown_05 = 1;
    }

check_second_stage:
    if (instance->unknown_61 == 0) {
        return instance->unknown_63;
    }

    if (instance->unknown_62 == 0) {
        uint8_t ready2;
        if (instance->unknown_54 == (uint32_t)k_datum_index_none) {
            ready2 = (instance->unknown_5c == (uint32_t)k_datum_index_none || sound_impulse_time() == 0) ? 1 : 0;
        } else {
            object_header *header = &((object_header *)object_data->data)[instance->unknown_54 & 0xffff];
            unit_data *unit = (unit_data *)((uint8_t *)header->data + k_unit_data_offset);
            ready2 = (*(int16_t *)((uint8_t *)unit + 0x388) != 6) ? 1 : 0; // UNSURE: unit_data.current_speech.sound_tag category
        }
        instance->unknown_62 = ready2;
        if (ready2 == 0) {
            return instance->unknown_63;
        }
    }

    {
        int16_t wait_ticks = instance->unknown_4c;
        if (wait_ticks > 0) {
            instance->unknown_4c = wait_ticks - 1;
            return instance->unknown_63;
        }
        instance->unknown_63 = 1;
        if ((instance->unknown_4e & 8) != 0) {
            if (instance->unknown_07[1] == 0) {
                instance->unknown_07[1] = 1;
                instance->unknown_07[2] = 0;
            }
            if (instance->unknown_07[2] != 0) {
                instance->unknown_07[1] = 0;
                return instance->unknown_63;
            }
            instance->unknown_63 = 0;
        }
    }

    return instance->unknown_63;
}

#if 0
Original Ghidra decompilation (0x431e70):

uint FUN_00431e70(void)

{
  ushort uVar1;
  undefined1 uVar2;
  uint in_EAX;
  uint uVar3;
  int iVar4;
  short sVar5;
  int iVar6;
  int iVar7;
  undefined4 *puVar8;
  char local_39;
  uint local_38;
  int local_34;
  undefined4 local_30;
  undefined4 local_2c;
  undefined4 local_20;
  undefined2 local_1c;
  undefined2 local_1a;
  undefined2 local_18;
  undefined2 local_14;
  undefined2 local_12;
  undefined4 local_10;
  undefined2 local_c;

  iVar6 = (in_EAX & 0xffff) * 100;
  iVar4 = *(int *)(DAT_008802d4 + 0x34);
  iVar7 = iVar6 + iVar4;
  uVar3 = *(short *)(iVar6 + 2 + iVar4) * 0x74 + *(int *)(global_scenario + 0x46c);
  if (*(char *)(iVar6 + 99 + iVar4) != '\0') goto LAB_004320ed;
  if (*(char *)(iVar7 + 0x61) == '\0') {
    local_39 = '\0';
    if (*(int *)(iVar7 + 0x5c) != -1) {
      uVar1 = *(ushort *)(iVar7 + 0x4e);
      local_38 = uVar1 & 0x10;
      if (((uVar1 & 0x10) != 0) || ((uVar1 & 0x20) != 0)) {
        local_34 = *(int *)(uVar3 + 0x50);
        sVar5 = 0;
        if (0 < local_34) {
          iVar4 = 0;
          do {
            uVar3 = *(uint *)(iVar7 + 0x28 + iVar4 * 4);
            if (((uVar3 != 0xffffffff) &&
                (((iVar4 = (uVar3 & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34),
                  (uVar1 & 0x20) != 0 ||
                  (((short)local_38 != 0 && (uVar3 == *(uint *)(iVar7 + 0x50))))) &&
                 (*(short *)(iVar4 + 0x6c) == 0xc)))) &&
               (((*(int *)(iVar4 + 0xa8) != -1 && (*(char *)(iVar4 + 0xa1) == '\0')) &&
                (*(char *)(iVar4 + 0xa0) == '\0')))) {
              local_39 = '\x01';
            }
            sVar5 = sVar5 + 1;
            iVar4 = (int)sVar5;
          } while (iVar4 < local_34);
        }
      }
      uVar3 = *(uint *)(DAT_006f1d6c + 0xc);
      if (((int)uVar3 < DAT_00725204) ||
         (uVar3 = CONCAT31((int3)(uVar3 >> 8),local_39), local_39 != '\0')) goto LAB_00432046;
      if ((*(int *)(iVar7 + 0x54) == -1) || (*(char *)(iVar7 + 0x60) != '\0')) {
        uVar3 = FUN_00543e10(0x3f800000);
      }
      else {
        local_34 = *(int *)(iVar7 + 0x5c);
        local_38 = 0xffffffff;
        uVar3 = FUN_00560d00(6,1,0,&local_38,&local_34);
        if ((short)uVar3 == 1) goto LAB_00432046;
        if (0 < (short)uVar3) {
          puVar8 = &local_30;
          for (iVar4 = 0xc; iVar4 != 0; iVar4 = iVar4 + -1) {
            *puVar8 = 0;
            puVar8 = puVar8 + 1;
          }
          local_2c = *(undefined4 *)(iVar7 + 0x5c);
          local_20 = *(undefined4 *)(iVar7 + 0x58);
          local_10 = *(undefined4 *)(iVar7 + 0x54);
          local_30._0_2_ = 6;
          local_30._2_2_ = 0xffff;
          local_1c = 0xffff;
          local_18 = 0xffff;
          local_1a = 0xffff;
          local_14 = 1;
          local_12 = 1;
          local_c = 0;
          uVar3 = FUN_00560f20();
        }
      }
    }
    *(undefined1 *)(iVar7 + 0x61) = 1;
    *(undefined1 *)(iVar7 + 5) = 1;
  }
LAB_00432046:
  uVar3 = CONCAT31((int3)(uVar3 >> 8),*(char *)(iVar7 + 0x61));
  if (*(char *)(iVar7 + 0x61) != '\0') {
    if (*(char *)(iVar7 + 0x62) == '\0') {
      if (*(uint *)(iVar7 + 0x54) == 0xffffffff) {
        if ((*(int *)(iVar7 + 0x5c) == -1) || (iVar4 = FUN_00543fc0(), iVar4 == 0)) {
          uVar3 = 1;
        }
        else {
          uVar3 = 0;
        }
      }
      else {
        uVar3 = CONCAT31((int3)((uint)DAT_008603b0 >> 8),
                         *(short *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                            (*(uint *)(iVar7 + 0x54) & 0xffff) * 0xc) + 0x388) != 6)
        ;
      }
      *(char *)(iVar7 + 0x62) = (char)uVar3;
      if ((char)uVar3 == '\0') goto LAB_004320ed;
    }
    uVar1 = *(ushort *)(iVar7 + 0x4c);
    uVar3 = (uint)uVar1;
    if (0 < (short)uVar1) {
      *(short *)(iVar7 + 0x4c) = (short)(uVar3 - 1);
      return CONCAT31((int3)(uVar3 - 1 >> 8),*(undefined1 *)(iVar7 + 99));
    }
    *(undefined1 *)(iVar7 + 99) = 1;
    if ((*(byte *)(iVar7 + 0x4e) & 8) != 0) {
      uVar2 = (undefined1)(uVar1 >> 8);
      if (*(char *)(iVar7 + 8) == '\0') {
        *(undefined1 *)(iVar7 + 8) = 1;
        *(undefined1 *)(iVar7 + 9) = 0;
      }
      uVar3 = (uint)CONCAT11(uVar2,*(char *)(iVar7 + 9));
      if (*(char *)(iVar7 + 9) != '\0') {
        *(undefined1 *)(iVar7 + 8) = 0;
        return (uint)CONCAT11(uVar2,*(undefined1 *)(iVar7 + 99));
      }
      *(undefined1 *)(iVar7 + 99) = 0;
    }
  }
LAB_004320ed:
  return CONCAT31((int3)(uVar3 >> 8),*(undefined1 *)(iVar7 + 99));
}
#endif
