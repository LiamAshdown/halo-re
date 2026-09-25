// ai_communication_play_event_line  (Ghidra: ai_communication_play_event_line; named for this rewrite)
// address 0x42eee0, size 888 bytes
// name confidence: 0.4   rewrite confidence: 0.35
// evidence: phase-4 summary ("attempts to select and queue playback of a communication line
// for a given event id and unit, gated by probability"). The table it scans, at 0x00656b08,
// is the same one src/ai/ai_communication_record_line_played.c already documents as "the
// conversation-line table, stride 0x24 from &DAT_00656b08"; the stride is confirmed by the
// two rows dumped out of the image (0x00656b08 and 0x00656b2c) and by the
// PTR_FUN_00656b28 / PTR_FUN_00656b4c predicate slots sitting exactly 0x24 apart.
// register convention: plain __cdecl, five stack arguments.
// blam-cc: stack -> object_index, event_id, force, explicit_speaker_actor_index, event_record
//
// UNSURE, substantially:
//  - The 0x24-byte table row's field names below are inferred from use only; the image
//    carries no names for it. The three parallel per-class tables at 0x006558c4 (int16),
//    0x006558d4 (float) and 0x006558f4 (int16) are eight entries each, indexed by the row's
//    class_index.
//  - Ghidra renders both float-to-int conversions as bare `__ftol()` calls with no visible
//    operand. Recovered from the disassembly: `fld [esi+0x18]; fmul ds:0x672ac8` at
//    0x42f126 (row.delay_seconds * 30.0 ticks/second) and
//    `fld [eax*4+0x6558d4]; fmul ds:0x672ac8` at 0x42f1ad
//    (communication_class_delay[class_index] * 30.0).
//  - FUN_00560d00 / unit_commit_speech are the sound/dialogue playback pair in the units module,
//    not rewritten here; their signatures are the five-argument form this repo already uses
//    in src/ai/actor_squad_action_execute.c, plus the EAX/DL register arguments the
//    disassembly shows (`mov eax,edi` / `xor dl,dl` at 0x42f15c).
//  - The 0x28-byte playback record built on the stack is transcribed field-for-field from
//    the stores at 0x42f19b..0x42f20d; only the four fields whose source is obvious are
//    named.
//  - The selection-3 branch's `object_try_and_get(3)` is shown by Ghidra with a single
//    argument; modeled here with this repo's established two-argument signature.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "game.h"
#include "ai.h"
#include <stdint.h>

extern data_array *object_data;      // 0x008603b0
extern data_array *actor_data;       // 0x00880360
extern ai_globals *ai_globals_ptr;   // 0x00880354
extern game_time_globals *game_time; // 0x006f1d6c
extern int32_t ai_communication_warmup_tick; // 0x00725204, UNSURE name
extern int16_t communication_class_line[8];  // 0x006558c4
extern float communication_class_delay[8];   // 0x006558d4
extern int16_t communication_class_order[8]; // 0x006558f4
extern float ticks_per_second;               // 0x00672ac8, 30.0

extern ai_communication_event_definition ai_communication_event_definitions[]; // 0x00656b08

extern real random_real(void); // 0x4019f0
extern void *object_try_and_get(datum_index object_index, int32_t kind); // 0x4f6ec0
extern void ai_communication_record_line_played(datum_index object_index, int16_t tier, int16_t communication_line_id, int16_t conversation_line_id); // 0x42f9e0, blam-cc: EAX -> object_index
extern datum_index ai_communication_select_speaker_in_reference(float radius,
                                                                int16_t allow_unreachable,
                                                                uint32_t fade_limit,
                                                                uint32_t line_class,
                                                                uint32_t line_id,
                                                                int16_t seat_filter, uint8_t flags,
                                                                uint32_t reference,
                                                                datum_index object_a,
                                                                datum_index object_b); // 0x42ff80
extern datum_index ai_communication_select_speaker_by_team(int16_t match_mode, datum_index object_a,
                                                           datum_index object_b, float radius,
                                                           int16_t allow_unreachable,
                                                           uint32_t fade_limit, uint32_t line_class,
                                                           uint32_t line_id, int16_t seat_filter,
                                                           uint8_t flags, int16_t team); // 0x4300d0
extern void actor_issue_order_or_vocalize(datum_index prop_index, datum_index actor_index,
                                           datum_index vehicle_object_index, int16_t line,
                                           int16_t variant); // 0x4302e0
extern int16_t unit_animation_change_priority_check(int32_t line_class, int32_t kind, void *out_record, void *inout_a,
                            void *inout_b);  // SIGNATURE-CONFLICT: this call site disagrees with the form the rest of
  // src/ai uses for this address; kept local. See src/ai/README.md.
// src/ai/actor_squad_action_execute.c declares the third argument as an int32_t. // 0x560d00, not yet rewritten; blam-cc also EAX -> object_index, DL -> flag
extern void unit_commit_speech(void); // 0x560f20, not yet rewritten; blam-cc: EAX -> object_index, EDX -> handle, ECX -> record

// blam-cc: stack -> object_index, event_id, force, explicit_speaker_actor_index, event_record
// Walks the communication event table for rows matching event_id, picks a speaker for the
// first one whose gates pass (an explicit speaker when the caller supplies one, otherwise a
// squad-mate, a fixed object or a hostile actor depending on the row's selection mode),
// rolls the row's probability unless `force` is set, and queues the line for playback,
// stamping the per-line cooldown and issuing the matching follow-up order.
void ai_communication_play_event_line(datum_index object_index, int16_t event_id, uint8_t force,
                                      datum_index explicit_speaker_actor_index,
                                      uint32_t *event_record)
{
    ai_communication_event_definition *row;
    int32_t row_index;
    datum_index actor_index;
    actor *speaker_actor;
    int16_t class_index;
    int16_t line_class;
    datum_index speaker;
    datum_index speaker_unit;
    object *speaker_object;
    int16_t match_mode;
    int16_t status;
    uint8_t playback_record[0x28];
    uint8_t lookup_record[4];
    uint32_t lookup_line;
    uint32_t lookup_handle;
    int32_t lookup_delay;

    if (ai_globals_ptr->communication_valid == 0 || event_id == -1) {
        return;
    }

    row = &ai_communication_event_definitions[0];
    row_index = 0;
    do {
        if (row->event_id == event_id) {
            actor_index = ((unit_data *)((object_header *)object_data->data)
                               [object_index & 0xffff].data)->actor_index;
            if (actor_index == (datum_index)k_datum_index_none) {
                speaker_actor = 0;
            } else {
                speaker_actor = (actor *)((uint8_t *)actor_data->data +
                                          (actor_index & 0xffff) * k_actor_size);
            }
            class_index = row->class_index;
            line_class = communication_class_line[class_index];

            if ((row->required_kind == -1 ||
                 row->required_kind == (int16_t)event_record[2]) &&
                (ai_communication_warmup_tick <= game_time->game_time ||
                 (row->flags & 1) != 0)) {
                speaker_unit = (datum_index)k_datum_index_none;
                if (explicit_speaker_actor_index == (datum_index)k_datum_index_none) {
                    if (row->selection == 2) {
                        if (speaker_actor == 0 ||
                            speaker_actor->encounter_index == (datum_index)k_datum_index_none) {
                            match_mode = 1;
                            goto select_by_team;
                        }
                        speaker = ai_communication_select_speaker_in_reference(
                            9.0f, -1, (uint32_t)class_index, (uint32_t)line_class,
                            (uint32_t)row->line_id, row->seat_filter, 0,
                            /* reference */ (uint32_t)speaker_actor->encounter_index,
                            /* object_a */ object_index,
                            /* object_b */ (datum_index)k_datum_index_none);
                        goto have_speaker;
                    } else if (row->selection == 3) {
                        // UNSURE: the fixed-object branch. event_record[0] is an object index.
                        speaker_unit = (datum_index)event_record[0];
                        if (object_try_and_get(speaker_unit, 3) != 0) {
                            goto play;
                        }
                        goto advance;
                    } else if (row->selection == 4) {
                        match_mode = 2;
select_by_team:
                        speaker = ai_communication_select_speaker_by_team(
                            match_mode, object_index, (datum_index)k_datum_index_none, 9.0f, -1,
                            (uint32_t)class_index, (uint32_t)line_class, (uint32_t)row->line_id,
                            row->seat_filter, 0,
                            /* team */ (speaker_actor != 0) ? speaker_actor->team : (int16_t)-1);
have_speaker:
                        if (speaker != (datum_index)k_datum_index_none) {
                            speaker_unit = ((actor *)((uint8_t *)actor_data->data +
                                                      (speaker & 0xffff) * k_actor_size))->unit_index;
                            goto play;
                        }
                    }
                } else {
                    speaker_unit = ((actor *)((uint8_t *)actor_data->data +
                                              (explicit_speaker_actor_index & 0xffff) *
                                                  k_actor_size))->unit_index;
play:
                    if (speaker_unit == (datum_index)k_datum_index_none) {
                        goto advance;
                    }
                    speaker_object = ((object_header *)object_data->data)
                                         [speaker_unit & 0xffff].data;
                    // object+0x218 is the currently-playing dialogue handle; a speaker
                    // already saying something is skipped.
                    if (*(int32_t *)((uint8_t *)speaker_object + 0x218) != -1) {
                        goto advance;
                    }
                    if (force == 0 &&
                        !(0.0f < row->probability && random_real() < row->probability)) {
                        goto advance;
                    }
                    if (row->predicate != 0 &&
                        row->predicate(object_index, event_record,
                                       ((unit_data *)speaker_object)->actor_index) == 0) {
                        goto advance;
                    }

                    lookup_line = (uint32_t)(uint16_t)row->line_id;
                    lookup_handle = (uint32_t)k_datum_index_none;
                    lookup_delay = (int32_t)(row->delay_seconds * ticks_per_second);
                    status = unit_animation_change_priority_check((int32_t)line_class, 1, lookup_record, &lookup_line,
                                          &lookup_handle);
                    if (0 < status) {
                        // The 0x28-byte playback request. Offsets are from the stores at
                        // 0x42f19b onwards; unnamed slots are transcribed verbatim.
                        *(int16_t *)(playback_record + 0x00) = line_class;                  // esp+0x28
                        *(int16_t *)(playback_record + 0x02) = (int16_t)lookup_line;        // esp+0x2a
                        *(uint32_t *)(playback_record + 0x04) = lookup_handle;              // esp+0x2c
                        *(int16_t *)(playback_record + 0x08) = (int16_t)lookup_delay;       // esp+0x30
                        *(int16_t *)(playback_record + 0x0a) =
                            (int16_t)(communication_class_delay[class_index] * ticks_per_second);
                        *(int16_t *)(playback_record + 0x0c) = 0x18;                        // esp+0x34
                        *(uint32_t *)(playback_record + 0x10) = (uint32_t)(uintptr_t)speaker_object;
                        *(int16_t *)(playback_record + 0x14) = -1;                          // esp+0x3c
                        *(int16_t *)(playback_record + 0x16) = -1;
                        *(int16_t *)(playback_record + 0x18) = -1;
                        *(int16_t *)(playback_record + 0x1c) = 0;                           // esp+0x44
                        *(int16_t *)(playback_record + 0x1e) = 0;
                        *(uint8_t *)(playback_record + 0x1a) = 1;                           // esp+0x42
                        *(int16_t *)(playback_record + 0x24) = 0;                           // esp+0x4c
                        *(uint32_t *)(playback_record + 0x28 - 4) = 0;
                        unit_commit_speech();
                        ai_communication_record_line_played(line_class, -1, row_index,
                                                            speaker_unit);
                        actor_issue_order_or_vocalize((datum_index)k_datum_index_none,
                                                      ((unit_data *)speaker_object)->actor_index,
                                                      (datum_index)k_datum_index_none, 8,
                                                      communication_class_order[row->class_index]);
                        return;
                    }
                }
            }
        }
advance:
        row = row + 1;
        row_index = row_index + 1;
    } while (row->event_id != -1);
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
