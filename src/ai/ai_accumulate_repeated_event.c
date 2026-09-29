// ai_accumulate_repeated_event  (Ghidra: ai_accumulate_repeated_event; named for this rewrite)
// address 0x42c610, size 813 bytes
// name confidence: 0.4   rewrite confidence: 0.4
// evidence: phase-4 summary ("accumulates and averages the positions of repeated identical
// events (keyed by id) within a short time window, reporting the merged event once ready").
// Reveals the real layout of what types/ai.h currently calls ai_globals.unknown_134[0x280]:
// a 32-entry ring buffer (head/tail at unknown_130/unknown_132, already named in the header)
// of 0x14-byte records {event_id:int16, count:int16, position:real_point3d, last_tick:int32}.
// register convention: plain __cdecl, all four arguments on the stack.
// blam-cc: stack -> event_type, position, event_id, window_ticks
//
// 0x401020 is vector3d_distance_squared (src/math, EAX / ECX): an event matches a record with the
// same id within 1 world unit of its averaged position (orphan pass 4 review).
// UNSURE: ai_broadcast_communication_event's second argument is
// Ghidra's CONCAT22 of the record pointer's own upper half with the event_id's lower half,
// a pointer/short punning artifact; reproduced here as the clean event_id.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "fn_ai.h"

extern ai_globals *ai_globals_ptr; // 0x00880354

extern int32_t game_engine_get_current_tick(void); // 0x470cd0
extern real vector3d_distance_squared(real_point3d *a, real_point3d *b); // 0x401020, src/math; blam-cc: EAX a, ECX b


// blam-cc: stack -> event_type, position, event_id, window_ticks
// Folds a newly observed event_id/position pair into ai_globals's 32-slot recent-event ring:
// expires entries older than 120 ticks (freeing or advancing the ring as it goes), and either
// blends this observation into a still-fresh matching entry's running average, restarts that
// average if the entry had gone stale for 30+ ticks, or starts a brand-new entry when no
// slot currently tracks event_id. Reports the merged event through ai_broadcast_communication_event whenever an
// entry's average was just (re)started rather than merely extended.
void ai_accumulate_repeated_event(int32_t event_type, real_point3d *position, int16_t event_id,
                                   int16_t window_ticks)
{
    int32_t current_tick;
    uint16_t cursor;
    uint16_t free_slot;
    ai_recent_event_record *found;
    ai_recent_event_record *records;
    uint8_t reset_average;
    uint8_t just_reset;

    if (!ai_globals_ptr->actors_valid || window_ticks <= 0) {
        return;
    }
    current_tick = game_engine_get_current_tick();
    records = (ai_recent_event_record *)&ai_globals_ptr->unknown_134;

    found = 0;
    just_reset = 1;
    free_slot = 0xffff;

    for (cursor = ai_globals_ptr->unknown_130; cursor != ai_globals_ptr->unknown_132;
         cursor = (cursor + 1) & 0x1f) {
        uint8_t matches_id = 0;
        // 0x42c6c1..0x42c6d7: EAX = position (stack parameter 2), ECX = &records[cursor].position
        if (records[cursor].event_id == event_id &&
            vector3d_distance_squared(position, &records[cursor].position) < 1.0f) {
            matches_id = 1;
        }
        if (current_tick - 0x78 < records[cursor].last_tick) {
            if (matches_id) {
                found = &records[cursor];
                found->count = found->count + 1;
                reset_average = (found->last_tick < current_tick - 0x1e);
                just_reset = reset_average;
                if (reset_average) {
                    found->position = *position;
                } else {
                    real weight = 1.0f / (real)found->count;
                    real inv_weight = 1.0f - weight;
                    found->position.x = weight * position->x + inv_weight * found->position.x;
                    found->position.y = weight * position->y + inv_weight * found->position.y;
                    found->position.z = weight * position->z + inv_weight * found->position.z;
                }
                break;
            }
        } else {
            records[cursor].event_id = -1;
            if (cursor == ai_globals_ptr->unknown_130) {
                ai_globals_ptr->unknown_130 = (cursor + 1) & 0x1f;
            } else {
                free_slot = cursor;
            }
        }
    }

    if (found == 0) {
        uint16_t slot;
        if (free_slot == 0xffff) {
            slot = ai_globals_ptr->unknown_132;
            ai_globals_ptr->unknown_132 = (ai_globals_ptr->unknown_132 + 1) & 0x1f;
            if (ai_globals_ptr->unknown_132 == ai_globals_ptr->unknown_130) {
                ai_globals_ptr->unknown_130 = (ai_globals_ptr->unknown_130 + 1) & 0x1f;
            }
        } else {
            slot = free_slot;
        }
        found = &records[slot];
        found->position = *position;
        found->last_tick = current_tick;
        found->event_id = event_id;
        found->count = 1;
    }

    if (just_reset) {
        // 0x42c915: EAX = the fourth argument (the gate), ECX = &entry->position, stack (first argument, entry id,
        //   entry count); the draft passed (event_type, id, count)
        ai_broadcast_communication_event(window_ticks, &found->position, event_type, found->event_id, found->count);
    }
}

#if 0
Original Ghidra decompilation (0x42c610):

void FUN_0042c610(undefined4 param_1,float *param_2,short param_3,short param_4)

{
  bool bVar1;
  float fVar2;
  float fVar3;
  bool bVar4;
  int iVar5;
  float10 fVar6;
  ushort local_1c;
  short *local_14;
  ushort local_10;

  if ((*(char *)(DAT_00880354 + 1) != '\0') && (iVar5 = game_engine_get_current_tick(), 0 < param_4)
     ) {
    local_14 = (short *)0x0;
    bVar1 = true;
    local_10 = 0xffff;
    for (local_1c = *(ushort *)(DAT_00880354 + 0x130); local_1c != *(ushort *)(DAT_00880354 + 0x132)
        ; local_1c = local_1c + 1 & 0x1f) {
      bVar4 = false;
      if ((param_3 == *(short *)(DAT_00880354 + 0x134 + (short)local_1c * 0x14)) &&
         (fVar6 = (float10)FUN_00401020(), fVar6 < (float10)1.0)) {
        bVar4 = true;
      }
      if (iVar5 + -0x78 < *(int *)(DAT_00880354 + 0x144 + (short)local_1c * 0x14)) {
        if (bVar4) {
          local_14 = (short *)(DAT_00880354 + 0x134 + (short)local_1c * 0x14);
          local_14[1] = local_14[1] + 1;
          bVar1 = *(int *)(local_14 + 8) < iVar5 + -0x1e;
          if (*(int *)(local_14 + 8) < iVar5 + -0x1e) {
            *(float *)(local_14 + 2) = *param_2;
            *(float *)(local_14 + 4) = param_2[1];
            *(float *)(local_14 + 6) = param_2[2];
          }
          else {
            fVar2 = 1.0 / (float)(int)local_14[1];
            fVar3 = 1.0 - fVar2;
            *(float *)(local_14 + 2) = fVar2 * *param_2 + fVar3 * *(float *)(local_14 + 2);
            *(float *)(local_14 + 4) = fVar2 * param_2[1] + fVar3 * *(float *)(local_14 + 4);
            *(float *)(local_14 + 6) = fVar2 * param_2[2] + fVar3 * *(float *)(local_14 + 6);
          }
          break;
        }
      }
      else {
        *(undefined2 *)(DAT_00880354 + 0x134 + (short)local_1c * 0x14) = 0xffff;
        if (local_1c == *(ushort *)(DAT_00880354 + 0x130)) {
          *(ushort *)(DAT_00880354 + 0x130) = local_1c + 1 & 0x1f;
        }
        else {
          local_10 = local_1c;
        }
      }
    }
    if (local_14 == (short *)0x0) {
      if (local_10 == 0xffff) {
        local_1c = *(ushort *)(DAT_00880354 + 0x132);
        *(ushort *)(DAT_00880354 + 0x132) = *(short *)(DAT_00880354 + 0x132) + 1U & 0x1f;
        if (*(short *)(DAT_00880354 + 0x132) == *(short *)(DAT_00880354 + 0x130)) {
          *(ushort *)(DAT_00880354 + 0x130) = *(short *)(DAT_00880354 + 0x130) + 1U & 0x1f;
        }
      }
      else {
        local_1c = local_10;
      }
      local_14 = (short *)(DAT_00880354 + 0x134 + (short)local_1c * 0x14);
      *(float *)(local_14 + 2) = *param_2;
      *(float *)(local_14 + 4) = param_2[1];
      *(float *)(local_14 + 6) = param_2[2];
      *(int *)(local_14 + 8) = iVar5;
      *local_14 = param_3;
      local_14[1] = 1;
    }
    if (bVar1) {
      FUN_00429fc0(param_1,CONCAT22((short)((uint)local_14 >> 0x10),*local_14),local_14[1]);
    }
  }
  return;
}
#endif
