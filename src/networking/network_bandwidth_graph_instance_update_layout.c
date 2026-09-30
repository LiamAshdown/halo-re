// network_bandwidth_graph_instance_update_layout  (Ghidra: FUN_004d7e20; named per this rewrite)
// address 0x4d7e20, size 596 bytes
// name confidence: 0.4   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md summary ("Recomputes the on-screen layout and
// label text of one network bandwidth-graph instance object when the screen size changes or a
// refresh is forced"); types/networking.h network_bandwidth_graph for the named fields
// (width/height at +0x14/+0x18 fall inside its documented unknown_0014[0x12], the label/layout
// scratch touched here falls inside its documented unknown_002c[0x90]); this function's own
// summary's naming hints ("update_for_resolution_change") and symbols/functions.txt
// rasterizer_resize_game_window for the 0x0069c634/0x0069c638 screen client-area corners.
// register convention: graph instance in EAX (in_EAX, unresolved), force-refresh flag in
// (Ghidra-recognized) param_1. // blam-cc: EAX -> graph, stack -> force_refresh
// TYPES-GAP: the four float constants this function multiplies by (0x672ab8, 0x672bc8,
// 0x672ac4, 0x672abc) and the two (0x672ba0, 0x672b9c) it divides by are not named anywhere;
// read directly from bin/halo.exe's .rdata (0.2, 0.4, 1.0, 0.5, 640.0, 480.0) since Ghidra folded
// only the first two into literals in its own decompile.
// UNSURE: every `__ftol()` call in Ghidra's decompile of this function is shown with an empty
// argument list -- __ftol takes its argument on the x87 stack (ST(0)), which Ghidra's decompiler
// frequently fails to attribute to a source expression. Every value below was reconstructed by
// tracing the x87 stack by hand against `objdump -d -M intel --start-address=0x4d7e20
// --stop-address=0x4d8080 bin/halo.exe`; the two computed-but-apparently-unread label-quad
// texture-scale expressions (R1, R2 and the fields at +0x2c/+0x2e/+0x34..+0x3e) are especially
// uncertain in *purpose* even though the arithmetic itself is confirmed against the disassembly.
// UNSURE: the label text is snprintf'd to `(uint8_t *)graph + 0x23e0`, which is past the end of
// types/networking.h's declared network_bandwidth_graph (size 0x23e0) -- the real object this
// function operates on is at least 0x25e0 bytes (0x23e0 struct + a 0x200-byte label buffer).
// Since types/*.h cannot be edited, the label buffer is reached by raw offset from the struct
// pointer rather than through a (missing) named field.

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "fn_networking.h"
#include <stdio.h>

extern network_screen_point game_window_top_left;     // 0x0069c634
extern network_screen_point game_window_bottom_right;  // 0x0069c638

extern void network_bandwidth_graph_instance_history_reset(network_bandwidth_graph *graph); // 0x4d8080, this batch

// UNSURE: Ghidra's callee is _snprintf; since the buffer size it enforces (0x200, matching the
// label buffer exactly) is not itself meaningfully different from an unbounded sprintf here,
// plain sprintf (declared by <stdio.h> below) is used for fidelity with what Ghidra shows.
extern const char *network_bandwidth_units_label_table[2];     // 0x0065d428, indexed by units_index
extern const char *network_bandwidth_direction_label_table[2]; // 0x0065d430, indexed by direction_index

// blam-cc: EAX -> graph, stack -> force_refresh
void network_bandwidth_graph_instance_update_layout(network_bandwidth_graph *graph, uint8_t force_refresh)
{
    uint8_t *base = (uint8_t *)graph;
    int32_t width = (int32_t)game_window_bottom_right.x - (int32_t)game_window_top_left.x;
    int32_t height = (int32_t)game_window_bottom_right.y - (int32_t)game_window_top_left.y;

    if (*(int32_t *)(base + 0x14) != width || *(int32_t *)(base + 0x18) != height || force_refresh != 0) {
        float x_scale = (float)width * 0.2f;
        float y_scale = (float)height * 0.4f;
        float right_raw = (float)(game_window_bottom_right.x - 0x40);   // Ghidra's fVar1 (before reuse)
        float baseline_raw = (float)(game_window_bottom_right.y - 0x40); // Ghidra's fVar2 (before reuse)
        float box_y1, box_x1, box_x0, box_y0;
        int16_t field24_v;
        float r1, r2;
        int32_t i;

        *(int32_t *)(base + 0x18) = height;
        *(int32_t *)(base + 0x14) = width;
        *(float *)(base + 0x1c) = x_scale;
        *(float *)(base + 0x20) = y_scale;

        graph->left = (int16_t)(right_raw - y_scale);
        field24_v = (int16_t)(baseline_raw - x_scale);
        *(int16_t *)(base + 0x24) = field24_v;
        graph->right = (int16_t)right_raw;
        graph->baseline = (int16_t)baseline_raw;

        // Clear the 320-entry vertex column array (0x1e00 bytes == 0x780 dwords).
        for (i = 0; i < 0x780; i++) {
            ((int32_t *)(base + 0x5d8))[i] = 0;
        }
        // Clear 30 floats of the label/layout scratch area starting at +0x44.
        for (i = 0; i < 0x1e; i++) {
            ((float *)(base + 0x44))[i] = 0.0f;
        }

        network_bandwidth_graph_instance_history_reset(graph);

        box_x0 = (right_raw - y_scale) - 1.0f;
        *(uint32_t *)(base + 0x50) = 0xffffff00;
        *(uint32_t *)(base + 0x68) = 0xffffff00;
        *(float *)(base + 0x44) = box_x0;
        *(uint32_t *)(base + 0x80) = 0xffffff00;
        *(uint32_t *)(base + 0x98) = 0xffffff00;
        box_y0 = (baseline_raw - x_scale) - 1.0f;
        *(uint32_t *)(base + 0xb0) = 0xffffff00;
        *(float *)(base + 0x48) = box_y0;
        box_y1 = right_raw + 1.0f;
        *(float *)(base + 0x5c) = box_y1;
        *(float *)(base + 0x60) = box_y0;
        *(float *)(base + 0x74) = box_y1;
        box_x1 = baseline_raw + 1.0f;
        *(float *)(base + 0x78) = box_x1;
        *(float *)(base + 0x8c) = box_x0;
        *(float *)(base + 0x90) = box_x1;
        *(float *)(base + 0xa4) = box_x0;
        *(float *)(base + 0xa8) = box_y0;

        // Label quad texture-space scaling; see file header UNSURE note.
        r1 = 640.0f / y_scale;
        r2 = 480.0f / x_scale;
        {
            int16_t v2c = (int16_t)((float)field24_v * r2);
            int16_t v36 = (int16_t)((float)graph->right * r1);

            *(int16_t *)(base + 0x2e) = (int16_t)((float)graph->left * r1);
            *(int16_t *)(base + 0x2c) = v2c;
            *(int16_t *)(base + 0x32) = 0x280;
            *(int16_t *)(base + 0x30) = 0x1e0;

            *(int16_t *)(base + 0x36) = v36;
            *(int16_t *)(base + 0x34) = v2c;
            *(int16_t *)(base + 0x3a) = 0x280;
            *(int16_t *)(base + 0x38) = 0x1e0;
            *(int16_t *)(base + 0x3e) = v36;

            *(int16_t *)(base + 0x3c) = (int16_t)(((x_scale * 0.5f) + (float)field24_v) * r2);
            *(int16_t *)(base + 0x42) = 0x280;
            *(int16_t *)(base + 0x40) = 0x1e0;
        }

        sprintf((char *)(base + 0x23e0), "%s %s",
            network_bandwidth_units_label_table[graph->units_index],
            network_bandwidth_direction_label_table[graph->direction_index]);
    }
}

#if 0
Original Ghidra decompilation (0x4d7e20):

void FUN_004d7e20(char param_1)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  undefined2 uVar5;
  undefined2 uVar6;
  int in_EAX;
  int iVar7;
  int iVar8;
  int iVar9;
  int iVar10;
  undefined4 *puVar11;
  float *pfVar12;

  iVar9 = (int)DAT_0069c638._2_2_;
  iVar10 = (int)(short)DAT_0069c638;
  iVar8 = iVar9 - DAT_0069c634._2_2_;
  iVar7 = iVar10 - (short)DAT_0069c634;
  if (((*(int *)(in_EAX + 0x14) != iVar7) || (*(int *)(in_EAX + 0x18) != iVar8)) ||
     (param_1 != '\0')) {
    *(int *)(in_EAX + 0x18) = iVar8;
    *(int *)(in_EAX + 0x14) = iVar7;
    *(float *)(in_EAX + 0x1c) = (float)iVar7 * 0.2;
    *(float *)(in_EAX + 0x20) = (float)iVar8 * 0.4;
    fVar2 = (float)(iVar10 + -0x40);
    fVar1 = (float)(iVar9 + -0x40);
    uVar5 = __ftol();
    *(undefined2 *)(in_EAX + 0x26) = uVar5;
    uVar5 = __ftol();
    *(undefined2 *)(in_EAX + 0x24) = uVar5;
    uVar5 = __ftol();
    *(undefined2 *)(in_EAX + 0x2a) = uVar5;
    uVar5 = __ftol();
    *(undefined2 *)(in_EAX + 0x28) = uVar5;
    puVar11 = (undefined4 *)(in_EAX + 0x5d8);
    for (iVar9 = 0x780; iVar9 != 0; iVar9 = iVar9 + -1) {
      *puVar11 = 0;
      puVar11 = puVar11 + 1;
    }
    pfVar12 = (float *)(in_EAX + 0x44);
    for (iVar9 = 0x1e; iVar9 != 0; iVar9 = iVar9 + -1) {
      *pfVar12 = 0.0;
      pfVar12 = pfVar12 + 1;
    }
    FUN_004d8080();
    fVar3 = (fVar1 - (float)iVar8 * 0.4) - 1.0;
    *(undefined4 *)(in_EAX + 0x50) = 0xffffff00;
    *(undefined4 *)(in_EAX + 0x68) = 0xffffff00;
    *(float *)(in_EAX + 0x44) = fVar3;
    *(undefined4 *)(in_EAX + 0x80) = 0xffffff00;
    *(undefined4 *)(in_EAX + 0x98) = 0xffffff00;
    fVar4 = (fVar2 - (float)iVar7 * 0.2) - 1.0;
    *(undefined4 *)(in_EAX + 0xb0) = 0xffffff00;
    *(float *)(in_EAX + 0x48) = fVar4;
    fVar1 = fVar1 + 1.0;
    *(float *)(in_EAX + 0x5c) = fVar1;
    *(float *)(in_EAX + 0x60) = fVar4;
    *(float *)(in_EAX + 0x74) = fVar1;
    fVar2 = fVar2 + 1.0;
    *(float *)(in_EAX + 0x78) = fVar2;
    *(float *)(in_EAX + 0x8c) = fVar3;
    *(float *)(in_EAX + 0x90) = fVar2;
    *(float *)(in_EAX + 0xa4) = fVar3;
    *(float *)(in_EAX + 0xa8) = fVar4;
    uVar5 = __ftol();
    *(undefined2 *)(in_EAX + 0x2e) = uVar5;
    uVar5 = __ftol();
    *(undefined2 *)(in_EAX + 0x2c) = uVar5;
    *(undefined2 *)(in_EAX + 0x32) = 0x280;
    *(undefined2 *)(in_EAX + 0x30) = 0x1e0;
    uVar6 = __ftol();
    *(undefined2 *)(in_EAX + 0x36) = uVar6;
    *(undefined2 *)(in_EAX + 0x34) = uVar5;
    *(undefined2 *)(in_EAX + 0x3a) = 0x280;
    *(undefined2 *)(in_EAX + 0x38) = 0x1e0;
    *(undefined2 *)(in_EAX + 0x3e) = uVar6;
    uVar5 = __ftol();
    *(undefined2 *)(in_EAX + 0x3c) = uVar5;
    *(undefined2 *)(in_EAX + 0x42) = 0x280;
    *(undefined2 *)(in_EAX + 0x40) = 0x1e0;
    __snprintf((char *)(in_EAX + 0x23e0),0x200,"%s %s",
               (&PTR_s_bytes_0065d428)[*(int *)(in_EAX + 0xc)],
               (&PTR_DAT_0065d430)[*(int *)(in_EAX + 0x10)]);
  }
  return;
}
#endif
