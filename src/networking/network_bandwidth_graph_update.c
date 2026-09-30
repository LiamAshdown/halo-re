// network_bandwidth_graph_update  (Ghidra: FUN_004d7ad0; named per this rewrite)
// address 0x4d7ad0, size 701 bytes
// name confidence: 0.4   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md summary ("Recomputes the on-screen layout and
// label of the global network bandwidth debug graph whenever the screen size changes, then
// redraws it"); every field this function touches is the SAME layout this batch's
// network_bandwidth_graph_instance_update_layout.c (0x4d7e20) computes for a per-instance
// graph, just inlined against the global singleton's absolute addresses instead of taking a
// pointer -- 0x00719cf4 is exactly network_bandwidth_graph_globals + 0x14, and the final
// snprintf destination 0x0071c0c0 is exactly network_bandwidth_graph_globals + 0x23e0, the same
// "past the declared struct" label buffer documented there. Cross-checking the two functions'
// field offsets against each other is itself part of the evidence both are transcribed
// correctly. See that file's header for the objdump-verified __ftol() argument reconstruction
// this one reuses.
// register convention: __cdecl, no parameters.
// TYPES-GAP: see network_bandwidth_graph_instance_update_layout.c for the label buffer past
// the struct's declared end. The screen client-area corners are types/networking.h's
// network_screen_point.

// VERIFIED against disassembly 0x4d7ad0..0x4d7d8d (2026-09-30): FIXED: right_raw/baseline_raw had br.x/br.y swapped (orig: +0x24 = (br.x-0x40)-W*0.2, +0x26 = (br.y-0x40)-H*0.4, +0x28 = br.x-0x40, +0x2a = br.y-0x40); 640/480 divisors are the raw height/width, not the scaled ones; snprintf(0x200) not sprintf; stray +0x12 store removed; network_stats_overlay_draw takes the graph in ESI
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <stdio.h>

extern network_screen_point game_window_top_left;     // 0x0069c634
extern network_screen_point game_window_bottom_right; // 0x0069c638

extern uint8_t network_bandwidth_overlay_enabled;              // 0x00710305
extern network_bandwidth_graph network_bandwidth_graph_globals; // 0x00719ce0

extern void network_bandwidth_graph_instance_history_reset(network_bandwidth_graph *graph); // 0x4d8080, this batch
extern void network_stats_overlay_draw(network_bandwidth_graph *graph); // 0x4d8620, this batch; blam-cc: ESI -> graph

extern const char *network_bandwidth_units_label_table[2];     // 0x0065d428
extern const char *network_bandwidth_direction_label_table[2]; // 0x0065d430

void network_bandwidth_graph_update(void)
{
    network_bandwidth_graph *graph = &network_bandwidth_graph_globals;
    uint8_t *base = (uint8_t *)graph;

    if (network_bandwidth_overlay_enabled != 0) {
        int32_t width = (int32_t)game_window_bottom_right.x - (int32_t)game_window_top_left.x;
        int32_t height = (int32_t)game_window_bottom_right.y - (int32_t)game_window_top_left.y;

        if (*(int32_t *)(base + 0x14) != width || *(int32_t *)(base + 0x18) != height) {
            float x_scale = (float)width * 0.2f;
            float y_scale = (float)height * 0.4f;
            float right_raw = (float)(game_window_bottom_right.y - 0x40);   // FIXED: this is br.y - 0x40 (edx at 0x4d7ad0..), stored at +0x2a and combined with y_scale;
            float right_minus_yscale = right_raw - y_scale;
            float baseline_raw = (float)(game_window_bottom_right.x - 0x40); // FIXED: this is br.x - 0x40, stored at +0x28 and combined with x_scale;
            float baseline_minus_xscale = baseline_raw - x_scale;
            float box_x1, box_y1, box_x0, box_y0;
            int16_t field24_v;
            float r1, r2;
            int32_t i;

            *(float *)(base + 0x1c) = x_scale;
            *(float *)(base + 0x20) = y_scale;
            *(int32_t *)(base + 0x14) = width;
            *(int32_t *)(base + 0x18) = height;

            graph->left = (int16_t)right_minus_yscale;
            field24_v = (int16_t)baseline_minus_xscale;
            *(int16_t *)(base + 0x24) = field24_v;
            graph->right = (int16_t)right_raw;
            graph->baseline = (int16_t)baseline_raw;

            for (i = 0; i < 0x780; i++) {
                ((int32_t *)(base + 0x5d8))[i] = 0;
            }
            for (i = 0; i < 0x1e; i++) {
                ((float *)(base + 0x44))[i] = 0.0f;
            }

            network_bandwidth_graph_instance_history_reset(graph);

            box_x0 = right_minus_yscale - 1.0f;
            *(uint32_t *)(base + 0x50) = 0xffffff00;
            *(uint32_t *)(base + 0x68) = 0xffffff00;
            *(uint32_t *)(base + 0x80) = 0xffffff00;
            *(uint32_t *)(base + 0x98) = 0xffffff00;
            *(float *)(base + 0x44) = box_x0;
            box_y0 = baseline_minus_xscale - 1.0f;
            *(uint32_t *)(base + 0xb0) = 0xffffff00;
            *(float *)(base + 0x48) = box_y0;
            box_y1 = right_raw + 1.0f;
            box_x1 = baseline_raw + 1.0f;
            *(float *)(base + 0x5c) = box_y1;
            *(float *)(base + 0x60) = box_y0;
            *(float *)(base + 0x74) = box_y1;
            *(float *)(base + 0x78) = box_x1;
            *(float *)(base + 0x8c) = box_x0;
            *(float *)(base + 0x90) = box_x1;
            *(float *)(base + 0xa4) = box_x0;
            *(float *)(base + 0xa8) = box_y0;

            r1 = 640.0f / (float)height; // FIXED: divides by the raw height (fst [esp+0x1c] before the *0.4), not y_scale
            r2 = 480.0f / (float)width;  // FIXED: raw width, not x_scale
            {
                int16_t v2c = (int16_t)((float)field24_v * r2);
                int16_t v3x = (int16_t)((float)graph->left * r1); // Ghidra's uVar5 reused across
                                                                    // three fields, see header

                *(int16_t *)(base + 0x2e) = v3x;
                *(int16_t *)(base + 0x2c) = v2c;
                *(int16_t *)(base + 0x32) = 0x280;
                *(int16_t *)(base + 0x30) = 0x1e0;
                *(int16_t *)(base + 0x34) = v2c;

                *(int16_t *)(base + 0x36) = (int16_t)((float)graph->right * r1);
                *(int16_t *)(base + 0x3a) = 0x280;
                *(int16_t *)(base + 0x38) = 0x1e0;
                *(int16_t *)(base + 0x3e) = *(int16_t *)(base + 0x36);

                *(int16_t *)(base + 0x3c) = (int16_t)(((x_scale * 0.5f) + (float)field24_v) * r2);
                *(int16_t *)(base + 0x42) = 0x280;
                *(int16_t *)(base + 0x40) = 0x1e0;
            }

            _snprintf((char *)(base + 0x23e0), 0x200, "%s %s",
                network_bandwidth_units_label_table[graph->units_index],
                network_bandwidth_direction_label_table[graph->direction_index]);
        }
        network_stats_overlay_draw(graph);
    }
}

#if 0
Original Ghidra decompilation (0x4d7ad0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_004d7ad0(void)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  undefined2 uVar5;
  int iVar6;
  int iVar7;
  undefined4 *puVar8;

  if (DAT_00710305 != '\0') {
    iVar7 = (int)DAT_0069c638._2_2_ - (int)DAT_0069c634._2_2_;
    iVar6 = (int)(short)DAT_0069c638 - (int)(short)DAT_0069c634;
    if ((DAT_00719cf4 != iVar6) || (DAT_00719cf8 != iVar7)) {
      _DAT_00719cfc = (float)iVar6 * 0.2;
      _DAT_00719d00 = (float)iVar7 * 0.4;
      fVar3 = (float)((short)DAT_0069c638 + -0x40);
      fVar4 = fVar3 - _DAT_00719cfc;
      fVar2 = (float)(DAT_0069c638._2_2_ + -0x40);
      fVar1 = fVar2 - _DAT_00719d00;
      DAT_00719cf4 = iVar6;
      DAT_00719cf8 = iVar7;
      DAT_00719d06 = __ftol();
      DAT_00719d04 = __ftol();
      DAT_00719d0a = __ftol();
      _DAT_00719d08 = __ftol();
      puVar8 = &DAT_0071a2b8;
      for (iVar6 = 0x780; iVar6 != 0; iVar6 = iVar6 + -1) {
        *puVar8 = 0;
        puVar8 = puVar8 + 1;
      }
      puVar8 = &DAT_00719d24;
      for (iVar6 = 0x1e; iVar6 != 0; iVar6 = iVar6 + -1) {
        *puVar8 = 0;
        puVar8 = puVar8 + 1;
      }
      FUN_004d8080();
      DAT_00719d24 = fVar1 - 1.0;
      _DAT_00719d30 = 0xffffff00;
      _DAT_00719d48 = 0xffffff00;
      _DAT_00719d60 = 0xffffff00;
      _DAT_00719d78 = 0xffffff00;
      DAT_00719d28 = fVar4 - 1.0;
      _DAT_00719d90 = 0xffffff00;
      _DAT_00719d3c = fVar2 + 1.0;
      _DAT_00719d58 = fVar3 + 1.0;
      _DAT_00719d40 = DAT_00719d28;
      _DAT_00719d54 = _DAT_00719d3c;
      _DAT_00719d6c = DAT_00719d24;
      _DAT_00719d70 = _DAT_00719d58;
      _DAT_00719d84 = DAT_00719d24;
      _DAT_00719d88 = DAT_00719d28;
      _DAT_00719d0e = __ftol();
      uVar5 = __ftol();
      _DAT_00719d12 = 0x280;
      _DAT_00719d10 = 0x1e0;
      _DAT_00719d0c = uVar5;
      _DAT_00719d16 = __ftol();
      _DAT_00719d1a = 0x280;
      _DAT_00719d18 = 0x1e0;
      _DAT_00719d14 = uVar5;
      _DAT_00719d1e = _DAT_00719d16;
      _DAT_00719d1c = __ftol();
      _DAT_00719d22 = 0x280;
      _DAT_00719d20 = 0x1e0;
      __snprintf(&DAT_0071c0c0,0x200,"%s %s",(&PTR_s_bytes_0065d428)[DAT_00719cec],
                 (&PTR_DAT_0065d430)[DAT_00719cf0]);
    }
    network_stats_overlay_draw();
  }
  return;
}
#endif
