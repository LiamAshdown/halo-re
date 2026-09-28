// ai_communication_play_event_line  (Ghidra: FUN_0042eee0)
// address 0x42eee0, size 888 bytes
// name confidence: 0.4   rewrite confidence: 0.85
// REWRITTEN from objdump 0x42eee0..0x42f257. The draft called the priority check, commit and record helpers
//   without their register operands. Stack: (object, event, force, speaker actor, event record). Walks the event
//   line table (0x656b08, 0x24 rows, ends at -1) for rows of this event (and of the record's kind +0x8 when the
//   row names one), outside the quiet period unless flag 1. The speaker is the given actor's unit, or by the row's
//   mode: 2 someone in the object's encounter / on its team within 9, 3 the record's object, 4 a team member for
//   mode 2 matching. Players never speak here. Unless forced the row's probability is rolled, its predicate asked;
//   then the priority check (0x560d00) and, when it allows, the speech is committed (0x560f20, tail 24 ticks),
//   recorded (0x42f9e0) and the speaker's follow-up order issued (0x4302e0 line 8).
// blam-cc: stack -> object_index, event_id, force, explicit_speaker_actor_index, event_record

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "game.h"
#include "ai.h"
#include <stdint.h>
#include <string.h>

extern data_array *object_data;      // 0x008603b0
extern data_array *actor_data;       // 0x00880360
extern data_array *encounter_data;   // 0x008802c8
extern uint8_t *ai_globals_ptr;      // 0x00880354
extern game_time_globals *game_time; // 0x006f1d6c
extern int32_t ai_communication_quiet_until_tick; // 0x00725204
extern int16_t ai_communication_class_priority[]; // 0x006558c4
extern float ai_communication_class_tail_seconds[]; // 0x006558d4
extern int16_t ai_communication_class_follow_up[]; // 0x006558f4
extern uint8_t ai_communication_event_definitions[];    // 0x00656b08, 0x24-byte rows

typedef uint8_t (*ai_communication_line_predicate)(datum_index object_index, uint32_t *event_record,
                                                   datum_index actor_index);

extern real random_real(void); // 0x4019f0
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, ECX, stack
extern datum_index ai_communication_select_speaker_in_reference(float radius, int16_t allow_unreachable,
    uint32_t fade_limit, uint32_t line_class, uint32_t line_id, int16_t seat_filter, uint8_t flags,
    uint32_t reference, datum_index object_a, datum_index object_b); // 0x42ff80, stack, EAX, EDI, EBX
extern datum_index ai_communication_select_speaker_by_team(int16_t match_mode, datum_index object_a,
    datum_index object_b, float radius, int16_t allow_unreachable, uint32_t fade_limit, uint32_t line_class,
    uint32_t line_id, int16_t seat_filter, uint8_t flags, int16_t team); // 0x4300d0, stack, DI
extern int32_t unit_animation_change_priority_check(uint32_t unit_index, uint8_t follow_fallback,
    int16_t requested_priority, uint8_t allow_repeat, uint32_t *out_unknown_3f0, int16_t *dialogue_index,
    int32_t *chain_value); // 0x560d00, EAX, DL, stack
extern int32_t unit_commit_speech(uint32_t unit_index, const unit_speech *source, int16_t mode); // 0x560f20, EAX, ECX, DX
extern void ai_communication_record_line_played(datum_index object_index, int16_t tier,
    int16_t communication_line_id, int16_t conversation_line_id); // 0x42f9e0, EAX, stack
extern void actor_issue_order_or_vocalize(datum_index prop_index, datum_index actor_index,
    datum_index vehicle_object_index, int16_t line, int16_t variant); // 0x4302e0, EAX, EBX, EDI, stack

#define OBJECT_DATA(h) ((uint8_t *)((object_header *)object_data->data)[(h) & 0xffff].data)

void ai_communication_play_event_line(datum_index object_index, int16_t event_id, uint8_t force,
                                      datum_index explicit_speaker_actor_index, uint32_t *event_record)
{
    uint8_t *row = ai_communication_event_definitions;
    int32_t row_index = 0;              // [esp+0x10]

    if (!ai_globals_ptr[0x10] || event_id == -1) {
        return;
    }
    for (; *(int16_t *)row != -1; row += 0x24, row_index++) {
        uint8_t *object;
        datum_index object_actor;
        int16_t class_index;
        int16_t priority;               // bp
        datum_index speaker_unit;       // edi
        uint8_t *speaker;               // [esp+0x20]
        int16_t dialogue_index;         // [esp+0x14]
        int32_t chain = -1;             // [esp+0x18]
        int16_t delay;                  // [esp+0x1c]
        uint32_t unused_out = 0;        // [esp+0x24]
        int32_t status;

        if (*(int16_t *)row != event_id) {
            continue;
        }
        object = OBJECT_DATA(object_index);
        object_actor = *(datum_index *)(object + 0x1f4);
        class_index = *(int16_t *)(row + 0xa);
        priority = ai_communication_class_priority[class_index];
        if (*(int16_t *)(row + 0x2) != -1 && *(int16_t *)(row + 0x2) != *(int16_t *)((uint8_t *)event_record + 0x8)) {
            continue;
        }
        if (game_time->game_time < ai_communication_quiet_until_tick && !(row[0xc] & 1)) {
            continue;
        }
        if (explicit_speaker_actor_index != k_datum_index_none) {
            speaker_unit = *(datum_index *)((uint8_t *)actor_data->data + (explicit_speaker_actor_index & 0xffff) * 0x724 + 0x18);
        } else {
            int16_t mode = *(int16_t *)(row + 0x4);
            datum_index found;

            if (mode == 3) {
                speaker_unit = event_record[0];
                if (object_try_and_get(speaker_unit, 3) == 0) {
                    continue;
                }
            } else if (mode == 2 || mode == 4) {
                uint8_t *actor = object_actor != k_datum_index_none
                    ? (uint8_t *)actor_data->data + (object_actor & 0xffff) * 0x724 : 0;

                if (mode == 2 && actor != 0 && *(datum_index *)(actor + 0x34) != k_datum_index_none) {
                    found = ai_communication_select_speaker_in_reference(9.0f, -1, (uint16_t)class_index,
                        (uint16_t)priority, *(uint16_t *)(row + 0x6), *(int16_t *)(row + 0x8), 0,
                        *(datum_index *)(actor + 0x34) & 0xffff, object_index, k_datum_index_none);
                } else {
                    found = ai_communication_select_speaker_by_team(mode == 2 ? 1 : 2, object_index, k_datum_index_none,
                        9.0f, -1, (uint16_t)class_index, (uint16_t)priority, *(uint16_t *)(row + 0x6),
                        *(int16_t *)(row + 0x8), 0, *(int16_t *)(object + 0xb8));
                }
                if (found == k_datum_index_none) {
                    continue;
                }
                speaker_unit = *(datum_index *)((uint8_t *)actor_data->data + (found & 0xffff) * 0x724 + 0x18);
            } else {
                continue;
            }
        }
        if (speaker_unit == k_datum_index_none) {
            continue;
        }
        speaker = OBJECT_DATA(speaker_unit);
        if (*(datum_index *)(speaker + 0x218) != k_datum_index_none) {
            continue;
        }
        if (!force) {
            float probability = *(float *)(row + 0x10);

            if (!(probability > 0.0f) || !(random_real() < probability)) {
                continue;
            }
        }
        if (*(ai_communication_line_predicate *)(row + 0x20) != 0 &&
            !(*(ai_communication_line_predicate *)(row + 0x20))(object_index, event_record,
                                                                 *(datum_index *)(speaker + 0x1f4))) {
            continue;
        }
        dialogue_index = (int16_t)*(uint16_t *)(row + 0x6);
        delay = (int16_t)(int32_t)(*(float *)(row + 0x18) * 30.0f);
        status = unit_animation_change_priority_check(speaker_unit, 0, priority, 1, &unused_out, &dialogue_index, &chain);
        if ((int16_t)status <= 0) {
            continue;
        }

        // 0x42f18d: queue the line
        {
            uint8_t speech[0x30];       // [esp+0x28]

            memset(speech, 0, sizeof(speech));
            *(int16_t *)(speech + 0x00) = priority;
            *(int16_t *)(speech + 0x02) = dialogue_index;
            *(int32_t *)(speech + 0x04) = chain;
            *(int16_t *)(speech + 0x08) = delay;
            *(int16_t *)(speech + 0x0a) = (int16_t)(int32_t)(ai_communication_class_tail_seconds[class_index] * 30.0f);
            *(int16_t *)(speech + 0x0c) = 0x18;
            *(datum_index *)(speech + 0x10) = object_index;
            *(int16_t *)(speech + 0x14) = -1;
            *(int16_t *)(speech + 0x16) = -1;
            *(int16_t *)(speech + 0x18) = -1;
            speech[0x1a] = 1;
            unit_commit_speech(speaker_unit, (unit_speech *)speech, (int16_t)status);
            ai_communication_record_line_played(speaker_unit, priority, -1, (int16_t)row_index);
            actor_issue_order_or_vocalize(k_datum_index_none, *(datum_index *)(speaker + 0x1f4), object_index, 8,
                                          (int16_t)(uint16_t)ai_communication_class_follow_up[class_index]);
        }
        return;
    }
}

#if 0
Original Ghidra decompilation (0x42eee0):

void FUN_0042eee0(uint param_1,short param_2,char param_3,uint param_4,uint *param_5)

{
  undefined2 uVar1;
  short sVar2;
  short sVar3;
  char cVar4;
  short sVar5;
  int iVar6;
  uint uVar7;
  short *psVar8;
  float fVar9;
  undefined4 uVar10;
  int local_48;
  uint local_44;
  undefined4 local_40;
  undefined4 local_3c;
  int local_38;
  undefined1 local_34 [4];
  undefined2 local_30;
  undefined2 local_2e;
  undefined4 local_2c;
  undefined2 local_28;
  undefined2 local_26;
  undefined2 local_24;
  uint local_20;
  undefined2 local_1c;
  undefined2 local_1a;
  undefined2 local_18;
  undefined1 local_16;
  undefined2 local_14;
  undefined2 local_12;
  undefined2 local_c;
  undefined4 local_8;
  undefined4 local_4;

  if ((*(char *)(DAT_00880354 + 0x10) == '\0') || (param_2 == -1)) {
    return;
  }
  psVar8 = &DAT_00656b08;
  local_48 = 0;
  do {
    if (*psVar8 == param_2) {
      uVar7 = *(uint *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc) + 500
                       );
      if (uVar7 == 0xffffffff) {
        local_44 = 0;
      }
      else {
        local_44 = (uVar7 & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
      }
      sVar5 = psVar8[5];
      uVar1 = *(undefined2 *)(&DAT_006558c4 + sVar5 * 2);
      if (((psVar8[1] == -1) || (psVar8[1] == (short)param_5[2])) &&
         ((DAT_00725204 <= *(int *)(DAT_006f1d6c + 0xc) || ((*(byte *)(psVar8 + 6) & 1) != 0)))) {
        if (param_4 == 0xffffffff) {
          sVar2 = psVar8[2];
          if (sVar2 == 2) {
            if ((local_44 == 0) || (*(int *)(local_44 + 0x34) == -1)) {
              sVar2 = psVar8[4];
              sVar3 = psVar8[3];
              uVar10 = 1;
LAB_0042f079:
              uVar7 = FUN_004300d0(uVar10,param_1,0xffffffff,0x41100000,0xffffffff,sVar5,uVar1,sVar3
                                   ,sVar2,0);
            }
            else {
              uVar7 = FUN_0042ff80(0x41100000,0xffffffff,sVar5,uVar1,psVar8[3],psVar8[4],0);
            }
            if (uVar7 != 0xffffffff) {
              uVar7 = *(uint *)((uVar7 & 0xffff) * 0x724 + 0x18 + *(int *)(DAT_00880360 + 0x34));
              goto LAB_0042f0a9;
            }
          }
          else if (sVar2 == 3) {
            uVar7 = *param_5;
            iVar6 = object_try_and_get(3);
            if (iVar6 != 0) goto LAB_0042f0a9;
          }
          else if (sVar2 == 4) {
            sVar2 = psVar8[4];
            sVar3 = psVar8[3];
            uVar10 = 2;
            goto LAB_0042f079;
          }
        }
        else {
          uVar7 = *(uint *)((param_4 & 0xffff) * 0x724 + 0x18 + *(int *)(DAT_00880360 + 0x34));
LAB_0042f0a9:
          if ((((uVar7 != 0xffffffff) &&
               (iVar6 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar7 & 0xffff) * 0xc),
               local_38 = iVar6, *(int *)(iVar6 + 0x218) == -1)) &&
              ((param_3 != '\0' ||
               ((0.0 < *(float *)(psVar8 + 8) &&
                (fVar9 = random_real(), fVar9 < *(float *)(psVar8 + 8))))))) &&
             ((*(code **)(psVar8 + 0x10) == (code *)0x0 ||
              (cVar4 = (**(code **)(psVar8 + 0x10))(param_1,param_5,*(undefined4 *)(iVar6 + 500)),
              cVar4 != '\0')))) {
            local_44 = (uint)(ushort)psVar8[3];
            local_40 = 0xffffffff;
            local_3c = __ftol();
            sVar5 = FUN_00560d00(uVar1,1,local_34,&local_44,&local_40);
            if (0 < sVar5) {
              local_2e = (undefined2)local_44;
              local_2c = local_40;
              local_28 = (undefined2)local_3c;
              local_30 = uVar1;
              local_26 = __ftol();
              local_1c = 0xffff;
              local_1a = 0xffff;
              local_18 = 0xffff;
              local_8 = 0;
              local_20 = param_1;
              local_14 = 0;
              local_12 = 0;
              local_c = 0;
              local_4 = 0;
              local_24 = 0x18;
              local_16 = 1;
              FUN_00560f20();
              ai_communication_record_line_played(uVar1,0xffffffff,local_48);
              FUN_004302e0(8,*(undefined2 *)(&DAT_006558f4 + psVar8[5] * 2));
              return;
            }
          }
        }
      }
    }
    psVar8 = psVar8 + 0x12;
    local_48 = local_48 + 1;
    if (*psVar8 == -1) {
      return;
    }
  } while( true );
}
#endif
