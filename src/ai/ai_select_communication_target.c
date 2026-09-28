// ai_select_communication_target  (Ghidra: ai_select_communication_target; named for this rewrite)
// address 0x42ec90, size 585 bytes
// name confidence: 0.3   rewrite confidence: 0.85 (REWRITTEN/verified against 0x42ec90: kind 3 reads param_b's object +0x1f4, the speaker select gets DI = param_a's team (+0xb8), and the classifier is given the selected speaker)
// evidence: phase-4 summary ("selects a target object matching a given communication-order
// definition, applying probability and recency weighting, for use by the AI communication
// system"). This is one of the module's more heavily register-aliased functions (Ghidra
// shows an unresolved in_ECX folded into a CONCAT31 with ai_globals.communication_valid, and
// packs two 16-bit halves into ai_communication_select_speaker_by_team's 6th/7th arguments via CONCAT22 in a way that
// does not decompile to clean values); reproduced as close to literally as possible rather
// than reinterpreted, since a confident clean-up was not achievable in the time available.
// The scanned table (base &DAT_00656b0a, stride 0x24 bytes / 0x12 shorts) is the same
// conversation-line table ai_communication_initialize.c counts into conversation_line_count,
// walked here starting one field in.
// register convention: plain __cdecl for the four recognized parameters, plus ECX carrying
// something Ghidra could not separate from ai_globals.communication_valid.
// blam-cc: stack -> param_a, param_b, line_id, sub_id, out_weight
//
// UNSURE, substantially: ai_communication_select_speaker_by_team's exact 10-argument shape reproduced verbatim from the
// decompile including its CONCAT22-packed arguments; ai_communication_class_priority (a per-line-id uint16
// table) is not otherwise established. This file should be treated as a starting point for
// a follow-up disassembly pass, not a finished rewrite.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "game.h"
#include "ai.h"

extern game_time_globals *game_time; // 0x006f1d6c
extern data_array *object_data;       // 0x008603b0
extern ai_communication_event_definition ai_communication_event_definitions[]; // 0x00656b08, stride 0x24
extern ai_globals *ai_globals_ptr;    // 0x00880354
extern uint32_t random_seed_global;    // 0x00719cd0
extern int32_t ai_communication_quiet_until_tick; // 0x00725204, the tick before which most lines are suppressed
extern int16_t ai_communication_class_priority[8]; // 0x006558c4, per-class communication line id
extern int32_t conversation_line_base; // 0x006f0ca4

extern int32_t actor_classify_communication_object_type(datum_index actor_index); // 0x42f9a0, EAX
extern datum_index ai_communication_select_speaker_by_team(int16_t match_mode, datum_index object_a,
    datum_index object_b, float radius, int16_t allow_unreachable, uint32_t fade_limit, uint32_t line_class,
    uint32_t line_id, int16_t seat_filter, uint8_t flags, int16_t team); // 0x4300d0, stack, DI
extern void *object_try_and_get(datum_index object_index, int32_t kind); // 0x4f6ec0

// blam-cc: stack -> param_a, param_b, line_id, sub_id, out_weight
// Scans the conversation-line table for an entry matching (line_id, sub_id) whose cooldown
// and random-chance gates pass, resolves a candidate object per its target-kind field (a
// scored search, a vehicle occupant, or another scored search with a different kind), then
// checks that candidate's per-index recency record in the conversation-line timestamp table
// before accepting it. Reports a recency-based weight (0..1) through out_weight when given.
int32_t ai_select_communication_target(uint32_t param_a, uint32_t param_b, int16_t line_id,
                                        int16_t sub_id, float *out_weight)
{
    uint16_t *entry;
    uint16_t *terminator;
    int32_t result;
    float weight;
    int32_t index;
    int16_t target_kind;
    int16_t candidate_a;
    int16_t candidate_b;
    uint32_t search_kind;
    void *vehicle_obj;
    int16_t comm_kind;
    int32_t *timestamp_pair;
    int32_t now;

    result = -1;
    weight = 1.0f;

    if (ai_globals_ptr->communication_valid && line_id != -1) {
        index = 0;
        // &ai_communication_event_definitions[0].required_kind: the original walks the
        // 0x24-stride table one field in, so entry[-1] is event_id and entry[0] is
        // required_kind.
        entry = (uint16_t *)&ai_communication_event_definitions[0].required_kind;
        do {
            if (*(int16_t *)(entry - 1) == line_id &&
                (*(int16_t *)entry == -1 || *(int16_t *)entry == sub_id)) {
                comm_kind = *(int16_t *)(entry + 4);
                if ((ai_communication_quiet_until_tick <= game_time->game_time ||
                     (*(uint8_t *)(entry + 5) & 1) != 0) &&
                    0.0f < *(float *)(entry + 9)) {
                    random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
                    if ((float)((uint32_t)random_seed_global >> 0x10) * 1.5259022e-05f <
                        *(float *)(entry + 9)) {
                        target_kind = *(int16_t *)(entry + 1);
                        if (target_kind == 2 || target_kind == 4) {
                            // entry + 2 is row + 0x06 (line_id) and entry + 3 is row + 0x08
                            // (seat_filter); the scorer takes them in that order.
                            candidate_a = *(int16_t *)(entry + 2);
                            candidate_b = *(int16_t *)(entry + 3);
                            search_kind = (target_kind == 2) ? 1u : 2u;
                            // FIXED (0x42edfd): DI = the team word at +0xb8 of param_a's object; the
                            //   old call left the 11th (register) argument unset
                            result = ai_communication_select_speaker_by_team((int16_t)search_kind, param_a,
                                                   0xffffffff, 9.0f, -1,
                                                   (uint32_t)(uint16_t)comm_kind,
                                                   (uint32_t)(uint16_t)ai_communication_class_priority[comm_kind],
                                                   (uint32_t)(uint16_t)candidate_a, candidate_b, 0,
                                                   *(int16_t *)((uint8_t *)((object_header *)object_data->data)[param_a & 0xffff].data + 0xb8));
                        } else if (target_kind == 3) {
                            // FIXED (0x42edae): the object is param_b ([esp+0x24]), not param_a
                            vehicle_obj = object_try_and_get(param_b, 3);
                            result = -1;
                            if (vehicle_obj != 0) {
                                result = *(int32_t *)((uint8_t *)vehicle_obj + 0x1f4);
                            }
                        }

                        if (result != -1) {
                            // FIXED (0x42ee17): classifies the selected speaker (EAX = result), not
                            //   the line kind
                            int16_t comm_index = (int16_t)actor_classify_communication_object_type((datum_index)result);
                            if (comm_index != -1) {
                                now = game_time->game_time;
                                timestamp_pair = (int32_t *)(conversation_line_base +
                                    (comm_index + index * 2) * 8);
                                if (timestamp_pair[0] != -1) {
                                    weight = (float)(now - timestamp_pair[0]) * 0.0011111111f;
                                    if (0.0f <= weight) {
                                        if (1.0f < weight) {
                                            weight = 1.0f;
                                        }
                                    } else {
                                        weight = 0.0f;
                                    }
                                }
                                if (timestamp_pair[1] != -1 && timestamp_pair[1] != now &&
                                    -1 < timestamp_pair[1] - now) {
                                    result = -1;
                                    goto next_entry;
                                }
                            }
                        }
                    }
                }
                if (result != -1) {
                    break;
                }
            }
next_entry:
            index = index + 1;
            terminator = entry + 0x11;
            entry = entry + 0x12;
        } while (*terminator != (uint16_t)-1);
    }

    if (out_weight != 0) {
        *out_weight = weight;
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x42ec90):

int FUN_0042ec90(undefined4 param_1,undefined4 param_2,short param_3,short param_4,float *param_5)

{
  short *psVar1;
  int *piVar2;
  short sVar3;
  short sVar4;
  int iVar5;
  short sVar6;
  undefined4 in_ECX;
  int iVar7;
  short *psVar8;
  int iVar9;
  undefined4 uVar10;
  float local_c;
  int local_8;

  iVar7 = CONCAT31((int3)((uint)in_ECX >> 8),*(char *)(DAT_00880354 + 0x10));
  iVar9 = -1;
  local_c = 1.0;
  if ((*(char *)(DAT_00880354 + 0x10) != '\0') && (param_3 != -1)) {
    local_8 = 0;
    psVar8 = &DAT_00656b0a;
    do {
      if ((psVar8[-1] == param_3) && ((*psVar8 == -1 || (*psVar8 == param_4)))) {
        sVar6 = psVar8[4];
        if (((DAT_00725204 <= *(int *)(DAT_006f1d6c + 0xc)) || ((*(byte *)(psVar8 + 5) & 1) != 0))
           && ((0.0 < *(float *)(psVar8 + 9) &&
               (random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f,
               (float)(random_seed_global >> 0x10) * 1.5259022e-05 < *(float *)(psVar8 + 9))))) {
          sVar3 = psVar8[1];
          if (sVar3 == 2) {
            sVar3 = psVar8[3];
            sVar4 = psVar8[2];
            uVar10 = 1;
LAB_0042edfd:
            iVar9 = FUN_004300d0(uVar10,param_1,0xffffffff,0x41100000,0xffffffff,
                                 CONCAT22((short)((uint)iVar7 >> 0x10),sVar6),
                                 CONCAT22(sVar6 >> 0xf,*(undefined2 *)(&DAT_006558c4 + sVar6 * 2)),
                                 sVar4,sVar3,0);
          }
          else if (sVar3 == 3) {
            iVar7 = object_try_and_get(3);
            if (iVar7 != 0) {
              iVar9 = *(int *)(iVar7 + 500);
            }
          }
          else if (sVar3 == 4) {
            sVar3 = psVar8[3];
            sVar4 = psVar8[2];
            uVar10 = 2;
            goto LAB_0042edfd;
          }
          if (iVar9 == -1) goto LAB_0042eeaa;
          sVar6 = FUN_0042f9a0();
          if (sVar6 != -1) {
            iVar7 = *(int *)(DAT_006f1d6c + 0xc);
            piVar2 = (int *)(DAT_006f0ca4 + ((int)sVar6 + (short)local_8 * 2) * 8);
            iVar5 = *piVar2;
            if (iVar5 != -1) {
              local_c = (float)(iVar7 - iVar5) * 0.0011111111;
              if (0.0 <= local_c) {
                if (1.0 < local_c) {
                  local_c = 1.0;
                }
              }
              else {
                local_c = 0.0;
              }
            }
            iVar5 = piVar2[1];
            if ((iVar5 != -1) && (iVar5 != iVar7 && -1 < iVar5 - iVar7)) {
              iVar9 = -1;
              goto LAB_0042eeaa;
            }
          }
        }
        if (iVar9 != -1) break;
      }
LAB_0042eeaa:
      iVar7 = local_8 + 1;
      psVar1 = psVar8 + 0x11;
      psVar8 = psVar8 + 0x12;
      local_8 = iVar7;
    } while (*psVar1 != -1);
  }
  if (param_5 != (float *)0x0) {
    *param_5 = local_c;
  }
  return iVar9;
}
#endif
