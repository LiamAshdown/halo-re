// network_stats_overlay_draw  (Ghidra: network_stats_overlay_draw, already named)
// address 0x4d8620, size 1021 bytes
// name confidence: 0.6   rewrite confidence: 0.35
// evidence: out/phase4/networking_functions.md summary ("Renders the network bandwidth debug
// overlay: draws the scrolling line graph via the Direct3D device and overlays the
// sent/received bits-per-second text"); types/networking.h network_bandwidth_graph.columns
// (+0x5d8), .peak_scale (+0x23d8), .displayed_rate (+0x23dc), .rate_sent (+0xc8),
// .rate_received (+0xcc), the label text at +0x23e0 (see network_bandwidth_graph_instance_
// update_layout, 0x4d7e20, already committed, for why that offset has no header field).
// Ghidra's own decompile of every indirect call through the renderer's vtable
// (`(**(code **)(*DAT_0071d174 + N))()`) is corrupted here -- it shows fabricated locals
// holding literal return addresses instead of real arguments, the same class of failure noted
// in src/structures/structure_picked_polygon_draw.c. This rewrite instead reconstructs every
// vtable call from `objdump -d -M intel --start-address=0x4d8620 --stop-address=0x4d8a20`,
// matching the `void **device = *rasterizer_device_ptr; (*(fn**)((uint8_t*)device+offset))(...)`
// idiom that file already established for the same global. Ordinary direct calls (sprintf,
// chimera__draw_8_bit_text, rasterizer_set_shader_stage_config, hud_text_draw_configure, __ftol) were
// decompiled correctly by Ghidra and are taken from its output as-is (folding __ftol into a
// plain int cast, per the same pattern already used in
// network_bandwidth_graph_instance_update_layout).
// register convention: Ghidra shows only `unaff_ESI`, matching every other function in this
// bandwidth-graph family (the graph singleton arrives in ESI). // blam-cc: ESI -> graph
// vtable offsets 0xe4 and 0x10c are corroborated by other phase-2 batches as SetRenderState-
// style (state, value) and SetSamplerState-style (sampler, type, value) calls
// (out/phase2/results/rasterizer_00.json, render_01.json); 0x14c is corroborated as a
// DrawPrimitive-style call against a raw vertex array (render_01.json), and this batch's own
// evidence (primitive type 3, count 0x13f matching a 320-point line strip, vertex pointer
// exactly graph->columns, stride 0x18 matching sizeof(network_graph_vertex)) confirms it here
// independently. 0x134 is hedged elsewhere as a "SetTextureStageState/blend-style" toggle
// taking a single flag derived from DAT_0069c680 (out/phase2/results/rasterizer_03.json),
// which matches this function's two calls to it exactly.
// UNSURE: vtable offsets 0x15c, 0x170, 0x178, 0x104 and 0x1ac have no corroborating evidence
// anywhere in this repository; they are preserved as raw offset calls with their exact
// arguments rather than guessed names.
// UNSURE: DAT_006e1af0, DAT_006e1af8, DAT_0069e468 and the two packed-point globals at
// 0x7c1254/0x7c1258 are foreign renderer/UI state with no name recoverable from this module;
// declared as opaque externs.
// UNSURE: hud_text_draw_configure is tentatively "hud_meter_set_active_flash_color" per
// symbols/review_queue.txt (confidence 0.3); called here for a side effect unrelated to the
// text color this function sets immediately afterward (DAT_006e4738.. is overwritten
// unconditionally right after the call, per Ghidra's reliable decompile of that part).
// `network_screen_point` (types/networking.h) is the same packed {x,y} int16 pair used for
// game_window_top_left/game_window_bottom_right in network_bandwidth_graph_instance_update_
// layout, but this is a different pair of globals (0x7c1254/0x7c1258) with no confirmed owner.
// reconciled: R36 0x006e4738..0x006e4744 is ColorARGB text_color, alpha first: externs renamed r/g/b/a -> alpha/red/green/blue by address (same bytes)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <stdio.h>

extern void ***rasterizer_device_ptr; // 0x0071d174, foreign render module (read, not owned);
    // same global and access idiom as src/structures/structure_picked_polygon_draw.c

extern uint32_t renderer_unknown_6e1af0; // 0x006e1af0, foreign render module, UNSURE
extern uint32_t renderer_unknown_6e1af8; // 0x006e1af8, foreign render module, UNSURE
extern uint32_t renderer_unknown_69e468; // 0x0069e468, foreign render module, UNSURE
extern uint8_t renderer_feature_flag_69c680; // 0x0069c680, foreign render module; used
    // elsewhere (out/phase2/results/rasterizer_03.json) as a skinning/blend-style flag

extern network_screen_point network_stats_overlay_text_rect_min; // 0x007c1254, UNSURE owner
extern network_screen_point network_stats_overlay_text_rect_max; // 0x007c1258, UNSURE owner

extern float renderer_text_color_alpha; // 0x006e4738
extern float renderer_text_color_red; // 0x006e473c
extern float renderer_text_color_green; // 0x006e4740
extern float renderer_text_color_blue; // 0x006e4744
extern uint16_t renderer_text_color_flags; // 0x006e4748
extern const char *decimal_format_string; // 0x0065fb30, the literal "%d"

extern void rasterizer_set_shader_stage_config(int32_t stage); // 0x519200, blam-cc: EAX -> stage, outside this batch
extern int32_t hud_text_draw_configure(int32_t a, int32_t b, int32_t c, int32_t d, int32_t e, int32_t f); // 0x4944c0,
    // tentatively "hud_meter_set_active_flash_color" per symbols/review_queue.txt (0.3), outside this batch
extern void chimera__draw_8_bit_text(int32_t x, int32_t y, const char *text); // 0x5148b0, outside this batch

// -- thin typed wrappers over the renderer's vtable, all sharing the one dereference idiom
// already established in src/structures/structure_picked_polygon_draw.c --

static void device_call1(void **device, uint32_t vtable_offset, int32_t arg1)
{
    (*(void (__stdcall **)(void *, int32_t))((uint8_t *)device + vtable_offset))(device, arg1);
}

static void device_call2(void **device, uint32_t vtable_offset, int32_t arg1, int32_t arg2)
{
    (*(void (__stdcall **)(void *, int32_t, int32_t))((uint8_t *)device + vtable_offset))(device, arg1, arg2);
}

static void device_set_render_state(void **device, int32_t state, int32_t value) // vtable+0xe4
{
    device_call2(device, 0xe4, state, value);
}

static void device_set_sampler_state(void **device, int32_t sampler, int32_t type, int32_t value) // vtable+0x10c
{
    (*(void (__stdcall **)(void *, int32_t, int32_t, int32_t))((uint8_t *)device + 0x10c))(device, sampler, type, value);
}

static void device_draw_primitive_up(void **device, int32_t primitive_type, int32_t primitive_count,
    const void *vertex_data, int32_t stride) // vtable+0x14c
{
    (*(void (__stdcall **)(void *, int32_t, int32_t, const void *, int32_t))((uint8_t *)device + 0x14c))(
        device, primitive_type, primitive_count, vertex_data, stride);
}

static void device_call_mode_ptr_count(void **device, uint32_t vtable_offset, int32_t mode,
    const void *ptr, int32_t count) // vtable+0x178's shape: (this, count, ptr, mode)
{
    (*(void (__stdcall **)(void *, int32_t, const void *, int32_t))((uint8_t *)device + vtable_offset))(
        device, count, ptr, mode);
}

// blam-cc: ESI -> graph
void network_stats_overlay_draw(network_bandwidth_graph *graph)
{
    void **device = *rasterizer_device_ptr;
    float delta_y, delta_x;
    float label_quad[13] = { 0 }; // see file header: indices 0,1,2,5,7,9 are never written,
                                  // matching the original's uninitialized stack slots there

    device_call1(device, 0x15c, (int32_t)renderer_unknown_6e1af0);

    {
        uint32_t flag = ((renderer_feature_flag_69c680 != 0) ? 0x10u : 0u) & 0x10u;
        flag = (flag | renderer_unknown_6e1af8) & 0x10u;
        device_call1(device, 0x134, (int32_t)flag);
    }

    device_call1(device, 0x170, (int32_t)renderer_unknown_69e468);

    // Build the small "label" quad's four scale/offset floats from the text-rect deltas; the
    // constants (1.0, -1.0, -2.0) are read directly from bin/halo.exe's .rdata at
    // 0x672ac4/0x672ba8/0x672bb0, the same way network_bandwidth_graph_instance_update_layout
    // (0x4d7e20) reads its own float constants.
    delta_y = (float)(int16_t)(network_stats_overlay_text_rect_max.y - network_stats_overlay_text_rect_min.y);
    delta_x = (float)(int16_t)(network_stats_overlay_text_rect_max.x - network_stats_overlay_text_rect_min.x);
    label_quad[3] = 2.0f * (1.0f / delta_y);
    label_quad[6] = -1.0f - (1.0f / delta_y);
    label_quad[8] = -2.0f * (1.0f / delta_x);
    label_quad[10] = (1.0f / delta_x) + 1.0f;
    device_call_mode_ptr_count(device, 0x178, 5, label_quad, 0xd);

    device_call1(device, 0x1ac, 0);
    rasterizer_set_shader_stage_config(0);

    device_set_render_state(device, 0x16, 1);
    device_set_render_state(device, 0xa8, 0xf);
    device_set_render_state(device, 0x1b, 0);
    device_set_render_state(device, 0xf, 0);
    device_set_render_state(device, 0x7, 0);
    device_set_render_state(device, 0xe, 0);
    device_set_render_state(device, 0x1c, 0);

    device_set_sampler_state(device, 0, 1, 3);
    device_set_sampler_state(device, 0, 3, 0);
    device_set_sampler_state(device, 0, 4, 3);
    device_set_sampler_state(device, 0, 6, 0);
    device_set_sampler_state(device, 1, 1, 1);
    device_set_sampler_state(device, 1, 4, 1);

    device_call2(device, 0x104, 0, 0);

    device_set_render_state(device, 0x9c, 1);
    device_set_render_state(device, 0x9d, 0);

    // The scrolling line graph itself: 320 vertices, drawn as one 319-segment line strip.
    device_draw_primitive_up(device, 3, 0x13f, graph->columns, sizeof(network_graph_vertex));
    // The border quad: 4 vertices at graph+0x44 (inside the header's documented
    // unknown_002c scratch range; see network_bandwidth_graph_instance_update_layout).
    device_draw_primitive_up(device, 3, 4, (uint8_t *)graph + 0x44, sizeof(network_graph_vertex));

    hud_text_draw_configure(1, -1, 0, 0, 5, 0); // see file header UNSURE note
    renderer_text_color_alpha = 1.0f;
    renderer_text_color_red = 1.0f;
    renderer_text_color_green = 1.0f;
    renderer_text_color_blue = 1.0f;
    renderer_text_color_flags = 0;

    {
        char text[128]; // matches Ghidra's acStack_330 usage, only ever holding short printed numbers/labels

        sprintf(text, "%s|n%.2f bps sent|n%.2f bps recv", (char *)graph + 0x23e0,
            (double)graph->rate_sent, (double)graph->rate_received);
        chimera__draw_8_bit_text(0, 0, text);

        sprintf(text, decimal_format_string, graph->peak_scale);
        chimera__draw_8_bit_text(0, 0, text);

        sprintf(text, decimal_format_string, (int32_t)graph->displayed_rate); // __ftol, see file header
        chimera__draw_8_bit_text(0, 0, text);
    }

    device_call1(device, 0x134, (int32_t)renderer_feature_flag_69c680);
}

#if 0
Original Ghidra decompilation (0x4d8620):

void network_stats_overlay_draw(void)

{
  undefined4 uVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  undefined4 uVar4;
  int unaff_ESI;
  char acStack_330 [4];
  int *piStack_32c;
  undefined4 uStack_328;
  undefined4 uStack_324;
  undefined4 uStack_320;
  int *piStack_31c;
  undefined4 uStack_318;
  undefined4 uStack_314;
  undefined4 uStack_310;
  int *piStack_30c;
  undefined4 uStack_308;
  undefined4 uStack_304;
  undefined4 uStack_300;
  int *piStack_2fc;
  undefined4 uStack_2f8;
  undefined4 uStack_2f4;
  undefined4 uStack_2f0;
  int *piStack_2ec;
  undefined4 uStack_2e8;
  undefined4 uStack_2e4;
  int *piStack_2e0;
  undefined4 uStack_2dc;
  undefined4 uStack_2d8;
  int *piStack_2d4;
  undefined4 uStack_2d0;
  undefined4 uStack_2cc;
  int *piStack_2c8;
  undefined4 uStack_2c4;
  undefined4 uStack_2c0;
  int *piStack_2bc;
  undefined4 uStack_2b8;
  undefined4 uStack_2b4;
  int *piStack_2b0;
  undefined4 uStack_2ac;
  undefined4 uStack_2a8;
  int *piStack_2a4;
  undefined4 uStack_2a0;
  undefined4 uStack_29c;
  int *piStack_298;
  undefined4 uStack_294;
  int *piStack_290;
  undefined4 uStack_28c;
  undefined1 *puStack_288;
  undefined4 uStack_284;
  int *piStack_280;
  int iStack_27c;
  int *piStack_278;
  uint uStack_274;
  int *piStack_270;
  undefined4 uStack_26c;

  uStack_26c = DAT_006e1af0;
  piStack_270 = DAT_0071d174;
  uStack_274 = 0x4d863c;
  (**(code **)(*DAT_0071d174 + 0x15c))();
  uStack_274 = -(uint)(DAT_0069c680 != '\0') & 0x10 | DAT_006e1af8 & 0x10;
  piStack_278 = DAT_0071d174;
  iStack_27c = 0x4d8663;
  (**(code **)(*DAT_0071d174 + 0x134))();
  iStack_27c = DAT_0069e468;
  piStack_280 = DAT_0071d174;
  uStack_284 = 0x4d8678;
  (**(code **)(*DAT_0071d174 + 0x170))();
  iStack_27c = (int)(short)((short)DAT_007c1258 - (short)DAT_007c1254);
  uStack_284 = 5;
  puStack_288 = &stack0xfffffd98;
  uStack_28c = 0xd;
  piStack_290 = DAT_0071d174;
  uStack_294 = 0x4d8778;
  (**(code **)(*DAT_0071d174 + 0x178))();
  uStack_294 = 0;
  piStack_298 = DAT_0071d174;
  uStack_29c = 0x4d8788;
  (**(code **)(*DAT_0071d174 + 0x1ac))();
  uStack_29c = 0x4d878f;
  rasterizer_set_shader_stage_config();
  uStack_29c = 1;
  uStack_2a0 = 0x16;
  piStack_2a4 = DAT_0071d174;
  uStack_2a8 = 0x4d87a1;
  (**(code **)(*DAT_0071d174 + 0xe4))();
  uStack_2a8 = 0xf;
  uStack_2ac = 0xa8;
  piStack_2b0 = DAT_0071d174;
  uStack_2b4 = 0x4d87b6;
  (**(code **)(*DAT_0071d174 + 0xe4))();
  uStack_2b4 = 0;
  uStack_2b8 = 0x1b;
  piStack_2bc = DAT_0071d174;
  uStack_2c0 = 0x4d87c8;
  (**(code **)(*DAT_0071d174 + 0xe4))();
  uStack_2c0 = 0;
  uStack_2c4 = 0xf;
  piStack_2c8 = DAT_0071d174;
  uStack_2cc = 0x4d87da;
  (**(code **)(*DAT_0071d174 + 0xe4))();
  uStack_2cc = 0;
  uStack_2d0 = 7;
  piStack_2d4 = DAT_0071d174;
  uStack_2d8 = 0x4d87ec;
  (**(code **)(*DAT_0071d174 + 0xe4))();
  uStack_2d8 = 0;
  uStack_2dc = 0xe;
  piStack_2e0 = DAT_0071d174;
  uStack_2e4 = 0x4d87fe;
  (**(code **)(*DAT_0071d174 + 0xe4))();
  uStack_2e4 = 0;
  uStack_2e8 = 0x1c;
  piStack_2ec = DAT_0071d174;
  uStack_2f0 = 0x4d8810;
  (**(code **)(*DAT_0071d174 + 0xe4))();
  uStack_2f0 = 3;
  uStack_2f4 = 1;
  uStack_2f8 = 0;
  piStack_2fc = DAT_0071d174;
  uStack_300 = 0x4d8824;
  (**(code **)(*DAT_0071d174 + 0x10c))();
  uStack_300 = 0;
  uStack_304 = 3;
  uStack_308 = 0;
  piStack_30c = DAT_0071d174;
  uStack_310 = 0x4d8838;
  (**(code **)(*DAT_0071d174 + 0x10c))();
  uStack_310 = 3;
  uStack_314 = 4;
  uStack_318 = 0;
  piStack_31c = DAT_0071d174;
  uStack_320 = 0x4d884c;
  (**(code **)(*DAT_0071d174 + 0x10c))();
  uStack_320 = 0;
  uStack_324 = 6;
  uStack_328 = 0;
  piStack_32c = DAT_0071d174;
  acStack_330[0] = '`';
  acStack_330[1] = -0x78;
  acStack_330[2] = 'M';
  acStack_330[3] = '\0';
  (**(code **)(*DAT_0071d174 + 0x10c))();
  acStack_330[0] = '\x01';
  acStack_330[1] = '\0';
  acStack_330[2] = '\0';
  acStack_330[3] = '\0';
  (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,1,1);
  (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,1,4,1);
  (**(code **)(*DAT_0071d174 + 0x104))(DAT_0071d174,0,0);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x9c,1);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x9d,0);
  (**(code **)(*DAT_0071d174 + 0x14c))(DAT_0071d174,3,0x13f,unaff_ESI + 0x5d8,0x18);
  (**(code **)(*DAT_0071d174 + 0x14c))(DAT_0071d174,3,4,unaff_ESI + 0x44,0x18);
  uVar1 = 0x3f800000;
  uVar2 = 0x3f800000;
  uVar3 = 0x3f800000;
  uVar4 = 0x3f800000;
  FUN_004944c0(1,0xffffffff,0,0,5,0);
  DAT_006e4744 = uVar4;
  DAT_006e4740 = uVar3;
  DAT_006e473c = uVar2;
  DAT_006e4738 = uVar1;
  DAT_006e4748 = 0;
  _sprintf(acStack_330,"%s|n%.2f bps sent|n%.2f bps recv",unaff_ESI + 0x23e0,
           (double)*(float *)(unaff_ESI + 200),(double)*(float *)(unaff_ESI + 0xcc));
  chimera__draw_8_bit_text(0,0,acStack_330);
  _sprintf(acStack_330,"%d",*(undefined4 *)(unaff_ESI + 0x23d8));
  chimera__draw_8_bit_text(0,0,acStack_330);
  __ftol();
  _sprintf(acStack_330,"%d");
  chimera__draw_8_bit_text(0,0,acStack_330);
  (**(code **)(*DAT_0071d174 + 0x134))(DAT_0071d174,DAT_0069c680);
  return;
}
#endif
