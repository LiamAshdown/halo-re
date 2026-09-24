// rasterizer_frame_statistics_sample  (Ghidra: rasterizer_frame_statistics_sample, already named;
// CEA rasterizer_frame_statistics_get_fps(frame_statistics, frame_dropped), hint only; the
// functions.txt name is kept)
// address 0x512530, size 454 bytes
// name confidence: 0.6   rewrite confidence: 0.8
// evidence: types/render.h rasterizer_frame_statistics field offsets (framerate +0x00,
//   sample_count +0x04, average_framerate +0x08, minimum_framerate +0x0c, maximum_framerate
//   +0x10, dropped_percentage +0x14) and the globals frame_statistics_times[60] /
//   frame_statistics_dropped[60] / frame_statistics_count (0x0071cfe8, 0x0071d0d8, 0x0071d114).
//   objdump 0x512530..0x5126f5: EBX is the output block (tested for NULL at 0x51254a, written
//   through [ebx+...]); every caller loads it with 0x007c30a0 (0x50be75, 0x50c56d, 0x50c63a).
//   The dropped flag is the byte at [esp+0x14] (only AL is stored). Intervals are unsigned and
//   clamped to at least 1 (cmp ecx,1; ja) and converted as unsigned (fadd 2^32 when negative).
// review fix (phase-4 gate): the first draft folded EBX into the global and dropped the NULL
//   check; the pointer argument is restored.
// register convention: EBX = rasterizer_frame_statistics*, one stack byte (dropped).
//   // blam-cc: EBX -> statistics, stack -> dropped
// UNSURE: DAT_006893e0/DAT_006893e2 (the "statistics enabled" gate) are not documented anywhere
//   in types/; named generically.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "render.h"

extern uint8_t console_debug_toggle_6893e0; // 0x006893e0 UNSURE: frame statistics enabled flag
extern uint16_t unknown_006893e2;           // 0x006893e2 UNSURE: paired with the above

extern uint32_t frame_statistics_times[60];  // 0x0071cfe8, newest first
extern uint8_t frame_statistics_dropped[60]; // 0x0071d0d8
extern int16_t frame_statistics_count;       // 0x0071d114

extern int32_t time_query_performance_counter_ms(void); // 0x449210, foreign (not yet rewritten); returns a
                                   // milliseconds-resolution system time counter

// Records a new frame sample (its system-time timestamp and whether it was dropped) into the
// 60 entry rolling history, shifting the arrays and folding in the running current/average/min/
// max framerate and dropped-frame percentage into *statistics. Does nothing (but resets the
// count) while frame statistics are disabled or statistics is NULL.
void rasterizer_frame_statistics_sample(rasterizer_frame_statistics *statistics, uint8_t dropped)
    // blam-cc: EBX -> statistics, stack -> dropped
{
    int32_t now;
    int16_t count;
    uint32_t current_interval;
    uint32_t min_interval;
    uint32_t max_interval;
    int16_t dropped_count;
    int16_t i;

    if ((console_debug_toggle_6893e0 == 0 && unknown_006893e2 == 0) || statistics == 0) {
        frame_statistics_count = 0;
        return;
    }

    now = time_query_performance_counter_ms();
    count = frame_statistics_count;

    if (count != 0) {
        current_interval = (uint32_t)now - frame_statistics_times[0];
        min_interval = current_interval;
        max_interval = current_interval;
        dropped_count = 0;

        for (i = (int16_t)(count - 1); i > 0; i--) {
            if (i > 1) {
                uint32_t interval = frame_statistics_times[i - 1] - frame_statistics_times[i];
                if (interval <= min_interval) {
                    min_interval = interval;
                }
                if (max_interval < interval) {
                    max_interval = interval;
                }
            }
            if (frame_statistics_dropped[i] != 0) {
                dropped_count = dropped_count + 1;
            }
            frame_statistics_times[i] = frame_statistics_times[i - 1];
            frame_statistics_dropped[i] = frame_statistics_dropped[i - 1];
        }

        {
            uint32_t latest_interval = (uint32_t)now - frame_statistics_times[0];
            if (latest_interval < 2) {
                latest_interval = 1;
            }
            statistics->sample_count = count;
            statistics->framerate = 1000.0f / (float)latest_interval;
        }
        {
            uint32_t elapsed = (uint32_t)now - frame_statistics_times[count - 1];
            if (elapsed < 2) {
                elapsed = 1;
            }
            statistics->average_framerate =
                ((float)count * 1000.0f) / (float)elapsed;
        }
        if (min_interval < 2) {
            min_interval = 1;
        }
        statistics->maximum_framerate = 1000.0f / (float)min_interval;
        if (max_interval < 2) {
            max_interval = 1;
        }
        statistics->minimum_framerate = 1000.0f / (float)max_interval;
        statistics->dropped_percentage =
            ((float)dropped_count * 100.0f) / (float)count;
    }

    frame_statistics_times[0] = now;
    frame_statistics_dropped[0] = dropped;

    if (count + 1 > 0x3c) {
        frame_statistics_count = 0x3c;
    } else {
        frame_statistics_count = count + 1;
    }
}

#if 0
Original Ghidra decompilation (0x512530):

void rasterizer_frame_statistics_sample(undefined1 param_1)

{
  float fVar1;
  float fVar2;
  int iVar3;
  uint uVar4;
  uint uVar5;
  int *piVar6;
  float *unaff_EBX;
  short sVar7;
  int iVar8;
  short sVar9;
  char *pcVar10;
  uint local_c;
  short local_8;

  if (((DAT_006893e0 == '\0') && (DAT_006893e2 == 0)) || (unaff_EBX == (float *)0x0)) {
    _DAT_0071d114 = _DAT_0071d114 & 0xffff0000;
    return;
  }
  iVar3 = FUN_00449210();
  local_8 = 0;
  sVar9 = DAT_0071d114;
  if (DAT_0071d114 != 0) {
    uVar4 = iVar3 - DAT_0071cfe8;
    iVar8 = _DAT_0071d114 - 1;
    sVar7 = (short)iVar8;
    local_c = uVar4;
    if (0 < sVar7) {
      pcVar10 = &DAT_0071d0d8 + sVar7;
      piVar6 = &DAT_0071cfe8 + sVar7;
      local_8 = 0;
      do {
        if (1 < (short)iVar8) {
          uVar5 = piVar6[-1] - *piVar6;
          if (uVar5 <= uVar4) {
            uVar4 = uVar5;
          }
          if (local_c < uVar5) {
            local_c = uVar5;
          }
        }
        if (*pcVar10 != '\0') {
          local_8 = local_8 + 1;
        }
        *piVar6 = piVar6[-1];
        iVar8 = iVar8 + -1;
        piVar6 = piVar6 + -1;
        *pcVar10 = pcVar10[-1];
        pcVar10 = pcVar10 + -1;
      } while (0 < (short)iVar8);
      sVar9 = DAT_0071d114;
    }
    uVar5 = iVar3 - DAT_0071cfe8;
    if (uVar5 < 2) {
      uVar5 = 1;
    }
    fVar1 = (float)(int)uVar5;
    if ((int)uVar5 < 0) {
      fVar1 = fVar1 + 4.2949673e+09;
    }
    *(short *)(unaff_EBX + 1) = sVar9;
    *unaff_EBX = 1000.0 / fVar1;
    uVar5 = iVar3 - *(int *)(&DAT_0071cfe4 + sVar9 * 4);
    if (uVar5 < 2) {
      uVar5 = 1;
    }
    fVar2 = (float)(int)sVar9;
    fVar1 = (float)(int)uVar5;
    if ((int)uVar5 < 0) {
      fVar1 = fVar1 + 4.2949673e+09;
    }
    unaff_EBX[2] = (fVar2 * 1000.0) / fVar1;
    if (uVar4 < 2) {
      uVar4 = 1;
    }
    fVar1 = (float)(int)uVar4;
    if ((int)uVar4 < 0) {
      fVar1 = fVar1 + 4.2949673e+09;
    }
    unaff_EBX[4] = 1000.0 / fVar1;
    if (local_c < 2) {
      local_c = 1;
    }
    fVar1 = (float)(int)local_c;
    if ((int)local_c < 0) {
      fVar1 = fVar1 + 4.2949673e+09;
    }
    unaff_EBX[3] = 1000.0 / fVar1;
    unaff_EBX[5] = ((float)(int)local_8 * 100.0) / fVar2;
  }
  DAT_0071cfe8 = iVar3;
  DAT_0071d0d8 = param_1;
  if (0x3c < sVar9 + 1) {
    _DAT_0071d114 = CONCAT22(DAT_0071d114_2,0x3c);
    return;
  }
  _DAT_0071d114 = CONCAT22(DAT_0071d114_2,(short)(sVar9 + 1));
  return;
}
#endif
