// network_client_check_connection_quality  (Ghidra: FUN_004e0080, unnamed)
// address 0x4e0080, size 504 bytes
// name confidence: 0.4   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md: "Tracks a client's packet loss/latency using
// the high-resolution timer, and returns false to reject/kick the client once its measured
// loss or latency exceeds the hard-coded thresholds for too many cons[ecutive samples]."
// machine_to_player/datum_get idiom matches the sibling lookups in this batch; player+0x108,
// +0x10c, +0x110, +0x114, +0x118 match types/game.h's player::unknown_108/10c/110/114/118
// exactly (five previously-unnamed fields, now confirmed to be a per-player loss/latency
// sample block).
// register convention: EAX = machine_index (uint32_t), stack = units (uint8_t, a
// packets/bytes-since-last-call count).
// blam-cc: EAX -> machine_index, stack -> units
// UNSURE: DAT_006894a8 (a byte gate) and the DAT_00719700/DAT_00719704 pair (a cached
// QueryPerformanceCounter-style LARGE_INTEGER, multiplied by 1000 and divided by
// performance_frequency to get milliseconds) have no established names elsewhere in this
// module; declared locally with generic names.
// UNSURE: the float thresholds (36.0 loss ratio, 5000ms window, 39.9ms latency, 5 consecutive
// bad samples) are transcribed as literals exactly as decompiled.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern datum_index machine_to_player[16]; // 0x006b1460
extern data_array *player_data; // 0x0087a480, stride 0x200 (game module)
extern uint8_t network_stats_enabled_gate; // 0x006894a8 (UNSURE name)
extern int64_t main_globals_data; // 0x00719700/0x00719704 (UNSURE: cached QPC value)
extern int64_t performance_frequency; // 0x006ac8f8/0x006ac8fc

// Resolves machine_index to a live player via machine_to_player/player_data, then -- while the
// stats gate is enabled -- maintains a rolling packet-loss ratio and per-sample latency in
// that player's unknown_108..unknown_118 block, returning false once loss exceeds 36:1 over a
// 5000ms window or latency exceeds 39.9ms for more than 5 consecutive samples.
uint32_t network_client_check_connection_quality(uint32_t machine_index, uint8_t units)
{
    datum_index resolved;
    int16_t player_index;
    player *plr;
    int16_t salt;

    resolved = machine_to_player[machine_index & 0xffff];
    if (resolved == (datum_index)0xffffffff) {
        return 0;
    }
    player_index = (int16_t)resolved;
    if (player_index < 0 || player_index >= player_data->maximum_count) {
        return 0;
    }
    plr = (player *)((uint8_t *)player_data->data + player_data->size * player_index);
    if (plr->identifier == 0) {
        return 0;
    }
    salt = (int16_t)(resolved >> 16);
    if (salt != 0 && plr->identifier != salt) {
        return 0;
    }

    if (network_stats_enabled_gate == 1) {
        int32_t now_ms;
        uint32_t added;
        uint32_t sample_count;
        float loss_ratio;
        float latency;

        now_ms = (int32_t)((main_globals_data * 1000) / performance_frequency);
        added = units;

        if (plr->connection_quality_started == 0) {
            plr->loss_window_start_ms = now_ms;
            plr->loss_window_units = added;
            plr->latency_last_sample_ms = now_ms;
            plr->latency_bad_sample_count = 0;
            plr->connection_quality_started = 1;
        } else {
            plr->loss_window_units = plr->loss_window_units + added;
            sample_count = (uint32_t)(now_ms - plr->loss_window_start_ms);
            // The two casts to (uint32_t) below replicate the original's manual
            // "add 4.2949673e+09 when negative" bit-pattern correction: both quantities are
            // computed as signed subtractions/sums but are meant to be read as unsigned.
            if (sample_count == 0) {
                loss_ratio = 0.0f;
            } else {
                loss_ratio = (float)(uint32_t)plr->loss_window_units / ((float)sample_count * 0.001f);
            }
            if (sample_count > 10000) {
                plr->loss_window_start_ms = now_ms;
                plr->loss_window_units = added;
                sample_count = 0;
            }

            {
                uint32_t latency_window;

                latency_window = (uint32_t)(now_ms - plr->latency_last_sample_ms);
                if (latency_window == 0) {
                    latency = 0.0f;
                } else {
                    latency = (float)added / ((float)latency_window * 0.001f);
                }
                plr->latency_last_sample_ms = now_ms;

                if (loss_ratio > 36.0f && sample_count > 5000) {
                    return 0;
                }
                if (latency != 0.0f) {
                    if (latency <= 39.9f) {
                        plr->latency_bad_sample_count = 0;
                        return 1;
                    }
                    plr->latency_bad_sample_count = plr->latency_bad_sample_count + 1;
                    if (plr->latency_bad_sample_count > 5) {
                        return 0;
                    }
                }
            }
        }
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4e0080):

undefined4 FUN_004e0080(byte param_1)

{
  float fVar1;
  float fVar2;
  short sVar3;
  uint in_EAX;
  int iVar4;
  int iVar5;
  uint uVar6;
  short sVar7;
  uint uVar8;
  int iVar9;
  undefined8 uVar10;
  float local_8;

  iVar4 = (&DAT_006b1460)[in_EAX & 0xffff];
  if (((iVar4 != -1) && (sVar3 = (short)iVar4, -1 < sVar3)) &&
     (sVar3 < *(short *)(DAT_0087a480 + 0x20))) {
    iVar9 = (int)*(short *)(DAT_0087a480 + 0x22) * (int)sVar3;
    sVar3 = *(short *)(iVar9 + *(int *)(DAT_0087a480 + 0x34));
    iVar9 = iVar9 + *(int *)(DAT_0087a480 + 0x34);
    if ((sVar3 != 0) && ((sVar7 = (short)((uint)iVar4 >> 0x10), sVar7 == 0 || (sVar3 == sVar7)))) {
      if (DAT_006894a8 == '\x01') {
        uVar10 = __allmul(DAT_00719700,DAT_00719704,1000,0);
        iVar4 = __alldiv(uVar10,DAT_006ac8f8,DAT_006ac8fc);
        uVar8 = (uint)param_1;
        if (*(char *)(iVar9 + 0x108) == '\0') {
          *(int *)(iVar9 + 0x10c) = iVar4;
          *(uint *)(iVar9 + 0x110) = uVar8;
          *(int *)(iVar9 + 0x114) = iVar4;
          *(undefined4 *)(iVar9 + 0x118) = 0;
          *(undefined1 *)(iVar9 + 0x108) = 1;
        }
        else {
          iVar5 = *(int *)(iVar9 + 0x110) + uVar8;
          *(int *)(iVar9 + 0x110) = iVar5;
          uVar6 = iVar4 - *(int *)(iVar9 + 0x10c);
          if (uVar6 == 0) {
            fVar1 = 0.0;
          }
          else {
            fVar1 = (float)iVar5;
            if (iVar5 < 0) {
              fVar1 = fVar1 + 4.2949673e+09;
            }
            fVar2 = (float)(int)uVar6;
            if ((int)uVar6 < 0) {
              fVar2 = fVar2 + 4.2949673e+09;
            }
            fVar1 = fVar1 / (fVar2 * 0.001);
          }
          if (10000 < uVar6) {
            *(int *)(iVar9 + 0x10c) = iVar4;
            *(uint *)(iVar9 + 0x110) = uVar8;
            uVar6 = 0;
          }
          iVar5 = iVar4 - *(int *)(iVar9 + 0x114);
          if (iVar5 == 0) {
            local_8 = 0.0;
          }
          else {
            fVar2 = (float)iVar5;
            if (iVar5 < 0) {
              fVar2 = fVar2 + 4.2949673e+09;
            }
            local_8 = (float)uVar8 / (fVar2 * 0.001);
          }
          *(int *)(iVar9 + 0x114) = iVar4;
          if ((36.0 < fVar1) && (5000 < uVar6)) {
            return 0;
          }
          if (local_8 != 0.0) {
            if (local_8 <= 39.9) {
              *(undefined4 *)(iVar9 + 0x118) = 0;
              return 1;
            }
            iVar4 = *(int *)(iVar9 + 0x118) + 1;
            *(int *)(iVar9 + 0x118) = iVar4;
            if (5 < iVar4) {
              return 0;
            }
          }
        }
      }
      return 1;
    }
  }
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
