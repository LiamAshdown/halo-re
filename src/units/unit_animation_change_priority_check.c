// unit_animation_change_priority_check  (Ghidra: unit_animation_change_priority_check)
// address 0x560d00, size 539 bytes
// name confidence: 0.4   rewrite confidence: 0.85
// REWRITTEN from objdump 0x560d00..0x560f1a: the unit speech arbiter. EAX = unit, DL = follow_fallback; stack:
//   requested priority, allow_repeat, out_unknown_3f0, dialogue index (in/out), chain value (in/out).
//   Without a chain value yet, the unit dialogue tag (+0x384) entry for the index (+0x1c + index * 16) is read,
//   following the fallback table (0x65e7a8) while follow_fallback is set and the entry is missing. A unit
//   flagged +0x106 bit 2 only speaks priority 0xa; a missing line returns 0. Otherwise, against the current
//   (+0x388) and pending (+0x3b8) speech priorities (a finished line of priority 2/7/10 with its duration
//   +0x3fa elapsed counts as nothing current when outranked): 2 = speak now (nothing current, or a priority >= 7
//   that the table priority 0x65e94c does not beat), 3 = the table priority beats both, 1 = replace (with
//   allow_repeat, the minimum repeat interval 0x65e964 in seconds -- FLT_MAX always, 0 never -- elapsed since the
//   last line, and the request above the current/pending, or the current being 2/7 or the request 6), else 0.
//   The index and chain are written back and out_unknown_3f0 receives +0x3f0.
// blam-cc: EAX -> unit_index, DL -> follow_fallback, stack -> requested_priority, allow_repeat, out_unknown_3f0,
//   dialogue_index, chain_value

#include "tags.h"
#include "memory.h"
#include "hs.h"
#include "math.h"
#include "game.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern int16_t unit_speech_fallback_index[];   // 0x0065e7a8
extern int16_t unit_speech_priority_table[];   // 0x0065e94c
extern float unit_speech_repeat_seconds[];     // 0x0065e964

int32_t unit_animation_change_priority_check(uint32_t unit_index, uint8_t follow_fallback,
    int16_t requested_priority, uint8_t allow_repeat, uint32_t *out_unknown_3f0, int16_t *dialogue_index,
    int32_t *chain_value)
{
    uint8_t *obj = (uint8_t *)((object_header *)object_data->data)[unit_index & 0xffff].data;
    int32_t chain = *chain_value;
    int16_t index = *dialogue_index;
    int16_t result = 0;

    if (chain == -1 && *(datum_index *)(obj + 0x384) != k_datum_index_none && index != -1) {
        uint8_t *dialogue = (uint8_t *)tag_instances[*(datum_index *)(obj + 0x384) & 0xffff].data;

        for (;;) {
            chain = *(int32_t *)(dialogue + index * 16 + 0x1c);
            if (!follow_fallback || chain != -1) {
                break;
            }
            index = unit_speech_fallback_index[index];
            if (index == -1) {
                break;
            }
        }
    }
    if (((obj[0x106] & 4) == 0 || requested_priority == 0xa) && chain != -1) {
        int16_t current = *(int16_t *)(obj + 0x388);

        if (current == 0) {
            result = 2;
        } else {
            int16_t pending = *(int16_t *)(obj + 0x3b8);
            int16_t highest = (current > pending) ? current : pending;
            int16_t table;
            uint8_t allowed = 0;

            if ((requested_priority == 2 || requested_priority == 7 || requested_priority == 10) &&
                obj[0x3f4] != 0 && *(int16_t *)(obj + 0x3fa) == 0 && requested_priority > highest) {
                highest = pending;
                current = 0;
            }
            table = unit_speech_priority_table[requested_priority];
            if (table >= highest) {
                result = 3;
            } else if (requested_priority >= 7 && table >= current) {
                result = 2;
            } else if (allow_repeat) {
                float interval = unit_speech_repeat_seconds[requested_priority];

                if (interval != 0.0f) {
                    if (interval == 3.4028235e+38f) {
                        allowed = 1;
                    } else {
                        allowed = (uint8_t)(*(int16_t *)(obj + 0x3fe) + *(int16_t *)(obj + 0x3fa) <
                            (int16_t)(int32_t)(interval * 30.0f));
                    }
                    if (allowed) {
                        if (requested_priority > highest) {
                            result = 1;
                        } else if (requested_priority > *(int16_t *)(obj + 0x3b8)) {
                            if (current == 2 || current == 7 || requested_priority == 6 || allowed) {
                                result = 1;
                            }
                        }
                    }
                }
            }
        }
    }
    *dialogue_index = index;
    *chain_value = chain;
    if (out_unknown_3f0 != 0) {
        *out_unknown_3f0 = *(uint32_t *)(obj + 0x3f0);
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
