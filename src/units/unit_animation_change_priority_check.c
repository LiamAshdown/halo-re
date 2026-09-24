// unit_animation_change_priority_check  (Ghidra: unit_animation_change_priority_check)
// address 0x560d00, size 539 bytes
// name confidence: 0.4 (phase2 candidate)   rewrite confidence: 0.25
// evidence: types/units.h unit_data.dialogue_tag_index (0x384), .current_speech/.pending_speech
//   (unit_speech at 0x388/0x3b8, .priority the first int16), .speech_started (0x3f4),
//   .speech_duration_ticks (0x3fa), .speech_tail_ticks (0x3fe), .unknown_3f0 (0x3f0).
// register convention: unit index in EAX; requested_priority in DX-sized stack param_1, an
//   allow-repeat flag in param_2, and three output pointers.
//   // blam-cc: in_EAX -> unit_index, stack params in order -> requested_priority,
//   //   allow_repeat, out_unknown_3f0, dialogue_index (in/out), chain_value (in/out)
// UNSURE: the dialogue tag record read at dialogue_tag_data + 0x1c + dialogue_index*0x10 has no
//   named struct in types/tags.h (UnitDialogueVariant is 0x18 bytes, not 0x10) -- kept as a raw
//   int32 "chain" field. The global tables at 0x0065e7a8/0x0065e94c/0x0065e964 (fallback chain,
//   priority table, minimum repeat interval) are declared as bare arrays since their element
//   counts are not established. __ftol's real argument arrives on the x87 stack (see
//   src/objects/antenna_apply_marker_delta.c for the established convention in this codebase);
//   it is modelled here as converting the repeat-interval seconds value into ticks via the
//   game clock's seconds-per-tick global, which is the only quantity in scope that makes the
//   surrounding tick comparison meaningful, but this is a guess.

#include "tags.h"
#include "memory.h"
#include "hs.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern int16_t dialogue_fallback_chain[];      // 0x0065e7a8, UNSURE element count
extern int16_t dialogue_priority_table[];      // 0x0065e94c, UNSURE element count
extern float dialogue_min_repeat_interval[];   // 0x0065e964, UNSURE element count
extern hs_game_time_globals *game_time; // 0x006f1d6c, the game time globals (types/hs.h)
extern int32_t __ftol(); // 0x6391b4, MSVC 7.1 CRT float-to-int truncation; the double is on the x87 stack

int32_t unit_animation_change_priority_check(uint32_t unit_index, int16_t requested_priority,
                                              uint8_t allow_repeat, uint32_t *out_unknown_3f0,
                                              int16_t *dialogue_index, int32_t *chain_value) // blam-cc: see file header
{
    object *obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);

    int32_t chain = *chain_value;
    int16_t index = *dialogue_index;
    int32_t result = 0;

    if (chain == -1 && unit->dialogue_tag_index != (datum_index)-1 && index != -1) {
        uint8_t *dialogue_data = (uint8_t *)tag_instances[unit->dialogue_tag_index & 0xffff].data;
        do {
            chain = *(int32_t *)(dialogue_data + 0x1c + index * 0x10); // UNSURE: raw record field
            if (!allow_repeat || chain != -1) {
                break;
            }
            index = dialogue_fallback_chain[index];
        } while (index != -1);
    }

    if (((obj->vitality_flags & _object_health_frozen_bit) == 0 || requested_priority == 10) && chain != -1) {
        int16_t playing_priority = unit->current_speech.priority;

        if (playing_priority == 0) {
            result = 2;
        } else {
            int16_t queued_priority = unit->pending_speech.priority;
            int16_t max_priority = (playing_priority <= queued_priority) ? queued_priority : playing_priority;

            if ((requested_priority == 2 || requested_priority == 7 || requested_priority == 10) &&
                unit->speech_started != 0 && unit->speech_duration_ticks == 0 &&
                max_priority < requested_priority) {
                playing_priority = 0;
                max_priority = queued_priority;
            }

            if (dialogue_priority_table[requested_priority] < max_priority) {
                if (requested_priority < 7 || dialogue_priority_table[requested_priority] < playing_priority) {
                    if (allow_repeat && dialogue_min_repeat_interval[requested_priority] != 0.0f) {
                        uint8_t ok = 1;
                        if (dialogue_min_repeat_interval[requested_priority] != 3.4028235e+38f) {
                            int32_t min_repeat_ticks =
                                __ftol((double)(dialogue_min_repeat_interval[requested_priority] /
                                                 game_time->seconds_per_tick)); // UNSURE
                            ok = (int32_t)unit->speech_tail_ticks + (int32_t)unit->speech_duration_ticks <
                                 min_repeat_ticks;
                            if (!ok) {
                                goto done;
                            }
                        }
                        if (requested_priority <= max_priority) {
                            if (requested_priority <= unit->pending_speech.priority) {
                                goto done;
                            }
                            if (playing_priority == 2 || playing_priority == 7) {
                                ok = 1;
                            }
                            if (requested_priority != 6 && !ok) {
                                goto done;
                            }
                        }
                        result = 1;
                    }
                } else {
                    result = 2;
                }
            } else {
                result = 3;
            }
        }
    }

done:
    *dialogue_index = index;
    *chain_value = chain;
    if (out_unknown_3f0 != 0) {
        *out_unknown_3f0 = unit->unknown_3f0;
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x560d00):

undefined4 FUN_00560d00(short param_1,char param_2,undefined4 *param_3,short *param_4,int *param_5)

{
  short sVar1;
  short sVar2;
  int iVar3;
  bool bVar4;
  short sVar5;
  uint in_EAX;
  int iVar6;
  char in_DL;
  short sVar7;
  short sVar8;
  int iVar9;
  short sVar10;
  undefined4 local_10;

  iVar3 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  iVar9 = *param_5;
  sVar10 = *param_4;
  local_10 = 0;
  if (((iVar9 == -1) && (*(uint *)(iVar3 + 900) != 0xffffffff)) && (sVar10 != -1)) {
    do {
      iVar9 = *(int *)(sVar10 * 0x10 + 0x1c +
                      *(int *)((*(uint *)(iVar3 + 900) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14));
      if ((in_DL == '\0') || (iVar9 != -1)) break;
      sVar10 = *(short *)(&DAT_0065e7a8 + sVar10 * 2);
    } while (sVar10 != -1);
  }
  if ((((*(byte *)(iVar3 + 0x106) & 4) == 0) || (param_1 == 10)) && (iVar9 != -1)) {
    sVar8 = *(short *)(iVar3 + 0x388);
    if (sVar8 == 0) {
      local_10 = 2;
    }
    else {
      sVar1 = *(short *)(iVar3 + 0x3b8);
      sVar7 = sVar8;
      if (sVar8 <= sVar1) {
        sVar7 = sVar1;
      }
      iVar6 = (int)param_1;
      if (((((iVar6 == 2) || (iVar6 == 7)) || (iVar6 == 10)) &&
          ((*(char *)(iVar3 + 0x3f4) != '\0' && (*(short *)(iVar3 + 0x3fa) == 0)))) &&
         (sVar7 < param_1)) {
        sVar8 = 0;
        sVar7 = sVar1;
      }
      if (*(short *)(&DAT_0065e94c + iVar6 * 2) < sVar7) {
        if ((param_1 < 7) || (*(short *)(&DAT_0065e94c + iVar6 * 2) < sVar8)) {
          if ((param_2 != '\0') && (*(float *)(&DAT_0065e964 + iVar6 * 4) != 0.0)) {
            if (*(float *)(&DAT_0065e964 + iVar6 * 4) == 3.4028235e+38) {
              bVar4 = true;
            }
            else {
              sVar1 = *(short *)(iVar3 + 0x3fe);
              sVar2 = *(short *)(iVar3 + 0x3fa);
              sVar5 = __ftol();
              bVar4 = (int)sVar1 + (int)sVar2 < (int)sVar5;
              if (!bVar4) goto LAB_00560eef;
            }
            if (param_1 <= sVar7) {
              if (param_1 <= *(short *)(iVar3 + 0x3b8)) goto LAB_00560eef;
              if ((sVar8 == 2) || (sVar8 == 7)) {
                bVar4 = true;
              }
              if ((param_1 != 6) && (!bVar4)) goto LAB_00560eef;
            }
            local_10 = 1;
          }
        }
        else {
          local_10 = 2;
        }
      }
      else {
        local_10 = 3;
      }
    }
  }
LAB_00560eef:
  *param_4 = sVar10;
  *param_5 = iVar9;
  if (param_3 != (undefined4 *)0x0) {
    *param_3 = *(undefined4 *)(iVar3 + 0x3f0);
  }
  return local_10;
}
#endif
