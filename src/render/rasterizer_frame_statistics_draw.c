// rasterizer_frame_statistics_draw  (Ghidra: rasterizer_frame_statistics_draw, already named;
// CEA rasterizer_frame_statistics_draw(void) via both format strings)
// address 0x512e80, size 1064 bytes
// name confidence: 0.9   rewrite confidence: 0.7
// evidence: objdump -d -M intel 0x512e80..0x5132a7, traced with the stack depth tracked at every
//   push (Ghidra's decompile gets the text colours and the tab stops wrong because the compiler
//   reuses pending argument slots). Frame: ebp frame aligned to 8, __chkstk 0x3060.
//   - ctrl+f11 / ctrl+f12: input_get_key_state 0x490b50 (ECX = key; 0x6f any control, 0x0b
//     f11, 0x0c f12, types/input.h) with the edge latches 0x0071d120 / 0x0071d124, flipping
//     frame_graph_render_infos (0x0071d12c = 1 - it) and frame_graph_render_graph (0x0071d128).
//   - milliseconds = QueryPerformanceCounter * 1000 / performance_frequency (__allmul
//     0x62de80, __alldiv 0x639230); the low dword minus frame_statistics_last_time (0x0071d130)
//     gives the frame time, and 1000 / it (unsigned div, left 0 when the delta is 0) is the
//     sample handed as an unsigned to float to fg_add_sample 0x512d90 (ECX = 0).
//   - fg_init 0x512700 first, fg_render 0x5129a0 (BL / AL from the two toggles, low bytes)
//     after, then network_bandwidth_graph_update 0x4d7ad0.
//   - when the statistics toggle 0x006893e0 is on: a Rectangle2D copied from 0x0069c63c
//     (top += 0x20, bottom += 0x20), six int16 tab stops {100, 200, 300, 400, 500, 600} plus
//     its left edge (0x0069c63e) with the first one then overwritten by the left edge itself,
//     hud_text_draw_configure 0x4944c0 (1, -1, 0, 0, 5, 0), the header line in white, the value
//     line in (1, 0.66, 1, 0.66) ARGB, and white restored after, each draw with text mode 6 in
//     0x006e4748 and the tab stops in 0x006e474a..0x006e4755 (mode 0 restored at the end).
//   - the value line's third number is (present count 0x0069c648 - frame_statistics_unknown_d8)
//     * 1000 / ((uint32)milliseconds - frame_statistics_unknown_d0): the average frame rate
//     since those baselines (both baselines are never written, so since startup). The
//     millisecond value is zero extended before the 64 bit subtraction (xor edx,edx at
//     0x5130b8), reproduced here.
//   - chimera__draw_8_bit_text 0x5148b0: EAX = 0, ECX = &bounds, stack (&cursor, -4, text).
//     The second line starts one pixel above the y the first draw left in cursor.y
//     (0x513197: bounds.top = cursor[1] - 1).
// register convention: none (void, cdecl).
//   // blam-cc: void
// UNSURE: cursor.x is never written before the first draw (the slot holds the fps value the
//   compiler parked there earlier), so it is left uninitialized here; the names of 0x0069c63c
//   (a screen bounds rectangle), 0x0069c648 (the present counter of
//   rasterizer_capture_and_present) and 0x006e474a (text tab stops) are provisional.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "render.h"

extern uint8_t console_debug_toggle_6893e0;       // 0x006893e0 frame statistics enabled
extern int32_t frame_statistics_key_a_latch;      // 0x0071d120, this module
extern int32_t frame_statistics_key_b_latch;      // 0x0071d124, this module
extern int32_t frame_graph_render_graph;          // 0x0071d128, this module
extern int32_t frame_graph_render_infos;          // 0x0071d12c, this module
extern int32_t frame_statistics_last_time;        // 0x0071d130, this module
extern int64_t frame_statistics_unknown_d0;       // 0x0071cfd0, this module, never written
extern int64_t frame_statistics_unknown_d8;       // 0x0071cfd8, this module, never written
extern rasterizer_frame_statistics rasterizer_frame_statistics_state; // 0x007c30a0, this module
extern int64_t performance_frequency;             // 0x006ac8f8/0x006ac8fc
extern Rectangle2D unknown_0069c63c;              // 0x0069c63c UNSURE: screen bounds
extern int64_t unknown_0069c648;                  // 0x0069c648 present counter (64 bit)

extern float hud_text_draw_color_a;               // 0x006e4738, interface module
extern float hud_text_draw_color_r;               // 0x006e473c
extern float hud_text_draw_color_g;               // 0x006e4740
extern float hud_text_draw_color_b;               // 0x006e4744
extern int16_t hud_text_draw_background_mode;     // 0x006e4748
extern int16_t hud_text_draw_tab_stops[6];        // 0x006e474a UNSURE name

extern uint8_t input_get_key_state(int16_t key_index); // 0x490b50, input; blam-cc: ECX
extern void rasterizer_frame_statistics_graph_init(void); // 0x512700, this module (fg_init)
extern void fg_add_sample(int32_t index, float sample);   // 0x512d90, this module;
    // blam-cc: ECX -> index, stack -> sample
extern void fg_render(uint8_t render_graph, uint8_t render_infos); // 0x5129a0, this module;
    // blam-cc: BL=render_graph, AL=render_infos
extern void network_bandwidth_graph_update(void); // 0x4d7ad0, networking module
extern void hud_text_draw_configure(int16_t font_table_index, uint16_t color_or_flags,
    int16_t column, uint32_t unknown_4730, int16_t color_table_index, int16_t color_index);
    // 0x4944c0, interface module (cdecl)
extern void chimera__draw_8_bit_text(Rectangle2D *clip_rect_override, int32_t *dest_rect_override,
    Point2DInt *cursor, int32_t flags, const char *text);
    // 0x5148b0, rasterizer module; blam-cc: EAX -> clip_rect_override, ECX -> dest_rect_override,
    // stack -> (cursor, flags, text). src/rasterizer types the two stack slots as opaque
    // uint32_t position_or_color1/2; 0x512e80 passes a Point2DInt out cursor and -4 there.
extern int32_t sprintf(char *buffer, const char *format, ...); // 0x623693 _sprintf

static void set_text_state(const float color[4], const int16_t tab_stops[6])
{
    int16_t i;

    hud_text_draw_color_a = color[0];
    hud_text_draw_color_r = color[1];
    hud_text_draw_color_g = color[2];
    hud_text_draw_color_b = color[3];
    hud_text_draw_background_mode = 6;
    for (i = 0; i < 6; i++) {
        hud_text_draw_tab_stops[i] = tab_stops[i];
    }
}

// Per frame: handles the ctrl+f11 / ctrl+f12 toggles, samples the frame time into the frame
// graph and draws it, and, when frame statistics are enabled, prints the two line table of
// framerate / average / overall average / min / max / dropped percentage.
void rasterizer_frame_statistics_draw(void)
{
    large_integer counter;
    uint32_t milliseconds;
    uint32_t delta;
    uint32_t sample;
    Rectangle2D bounds;
    int16_t tab_stops[6];
    Point2DInt cursor;
    float header_color[4];
    float value_color[4];
    float restore_color[4];
    int64_t presents;
    int64_t elapsed;
    int16_t left;
    int16_t i;
    char text[0x3000];

    if (input_get_key_state(0x6f) && input_get_key_state(0x0b)) {
        if (frame_statistics_key_a_latch == 0) {
            frame_graph_render_infos = 1 - frame_graph_render_infos;
            frame_statistics_key_a_latch = 1;
        }
    } else {
        frame_statistics_key_a_latch = 0;
    }
    if (input_get_key_state(0x6f) && input_get_key_state(0x0c)) {
        if (frame_statistics_key_b_latch == 0) {
            frame_graph_render_graph = 1 - frame_graph_render_graph;
            frame_statistics_key_b_latch = 1;
        }
    } else {
        frame_statistics_key_b_latch = 0;
    }

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    milliseconds = (uint32_t)((counter.quad_part * 1000) / performance_frequency);
    delta = milliseconds - (uint32_t)frame_statistics_last_time;
    frame_statistics_last_time = (int32_t)milliseconds;
    sample = delta;
    if (delta != 0) {
        sample = 1000 / delta;
    }
    rasterizer_frame_statistics_graph_init();
    fg_add_sample(0, (float)sample);
    fg_render((uint8_t)frame_graph_render_graph, (uint8_t)frame_graph_render_infos);
    network_bandwidth_graph_update();

    if (!console_debug_toggle_6893e0) {
        return;
    }

    presents = unknown_0069c648 - frame_statistics_unknown_d8;
    left = unknown_0069c63c.left;
    tab_stops[0] = 100;
    tab_stops[1] = 200;
    tab_stops[2] = 300;
    tab_stops[3] = 400;
    tab_stops[4] = 500;
    tab_stops[5] = 600;
    cursor.y = 0;
    header_color[0] = 1.0f;
    header_color[1] = 1.0f;
    header_color[2] = 1.0f;
    header_color[3] = 1.0f;
    value_color[0] = 1.0f;
    value_color[1] = 0.66f;
    value_color[2] = 1.0f;
    value_color[3] = 0.66f;
    restore_color[0] = 1.0f;
    restore_color[1] = 1.0f;
    restore_color[2] = 1.0f;
    restore_color[3] = 1.0f;

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    milliseconds = (uint32_t)((counter.quad_part * 1000) / performance_frequency);
    elapsed = (int64_t)(uint64_t)milliseconds - frame_statistics_unknown_d0;

    for (i = 0; i < 6; i++) {
        tab_stops[i] = (int16_t)(tab_stops[i] + left);
    }
    bounds = unknown_0069c63c;
    bounds.top = (int16_t)(bounds.top + 0x20);
    bounds.bottom = (int16_t)(bounds.bottom + 0x20);

    hud_text_draw_configure(1, 0xffff, 0, 0, 5, 0);
    sprintf(text, "|n|tframerate|taverage (of %d)|tmin|tmax|tdropped",
            (int32_t)rasterizer_frame_statistics_state.sample_count);
    tab_stops[0] = left;
    set_text_state(header_color, tab_stops);
    chimera__draw_8_bit_text(0, (int32_t *)&bounds, &cursor, -4, text);

    bounds.top = (int16_t)(cursor.y - 1);
    sprintf(text, "|t%.0f|t%.0f/%.0f|t%.0f|t%.0f|t%5.1f%%|n",
            (double)rasterizer_frame_statistics_state.framerate,
            (double)rasterizer_frame_statistics_state.average_framerate,
            (double)(((float)presents * 1000.0f) / (float)elapsed),
            (double)rasterizer_frame_statistics_state.minimum_framerate,
            (double)rasterizer_frame_statistics_state.maximum_framerate,
            (double)rasterizer_frame_statistics_state.dropped_percentage);
    tab_stops[0] = left;
    set_text_state(value_color, tab_stops);
    chimera__draw_8_bit_text(0, (int32_t *)&bounds, &cursor, -4, text);

    hud_text_draw_color_a = restore_color[0];
    hud_text_draw_background_mode = 0;
    hud_text_draw_color_r = restore_color[1];
    hud_text_draw_color_g = restore_color[2];
    hud_text_draw_color_b = restore_color[3];
}

#if 0
Original Ghidra decompilation (0x512e80):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */
/* WARNING: Removing unreachable block (ram,0x00512f8a) */
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl rasterizer_frame_statistics_draw(void)

{
  short sVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  char cVar4;
  int iVar5;
  uint uVar6;
  short *psVar7;
  undefined8 uVar8;
  LARGE_INTEGER local_3068;
  undefined4 local_305c;
  undefined4 local_3058;
  undefined4 local_3054;
  undefined2 local_3050;
  short sStack_304e;
  undefined4 local_3048;
  undefined4 local_3044;
  undefined4 local_3040;
  undefined4 local_303c;
  undefined4 local_3038;
  undefined4 local_3034;
  undefined4 local_3030;
  undefined4 local_302c;
  undefined4 local_3028;
  undefined4 local_3024;
  undefined4 local_3020;
  undefined4 local_301c;
  int local_3018;
  int iStack_3014;
  int local_3010;
  int iStack_300c;
  char local_3008 [12284];
  undefined4 uStack_c;
  
  uStack_c = 0x512e90;
  cVar4 = FUN_00490b50();
  if ((cVar4 == '\0') || (cVar4 = FUN_00490b50(), cVar4 == '\0')) {
    DAT_0071d120 = 0;
  }
  else if (DAT_0071d120 == 0) {
    _DAT_0071d12c = 1 - _DAT_0071d12c;
    DAT_0071d120 = 1;
  }
  cVar4 = FUN_00490b50();
  if ((cVar4 == '\0') || (cVar4 = FUN_00490b50(), cVar4 == '\0')) {
    DAT_0071d124 = 0;
  }
  else if (DAT_0071d124 == 0) {
    _DAT_0071d128 = 1 - _DAT_0071d128;
    DAT_0071d124 = 1;
  }
  QueryPerformanceCounter(&local_3068);
  uVar8 = __allmul(local_3068.s.LowPart,local_3068.s.HighPart,1000,0);
  iVar5 = __alldiv(uVar8,DAT_006ac8f8,DAT_006ac8fc);
  local_3050 = 0;
  if (iVar5 - DAT_0071d130 != 0) {
    local_3050 = (undefined2)(1000 / (ulonglong)(uint)(iVar5 - DAT_0071d130));
  }
  DAT_0071d130 = iVar5;
  rasterizer_frame_statistics_graph_init();
  sStack_304e = 0;
  FUN_00512d90();
  rasterizer_frame_statistics_draw_graph();
  FUN_004d7ad0();
  if (DAT_006893e0 != '\0') {
    sVar1 = DAT_0069c63e;
    local_3010 = DAT_0069c648 - DAT_0071cfd8;
    iStack_300c = (DAT_0069c64c - DAT_0071cfdc) - (uint)(DAT_0069c648 < DAT_0071cfd8);
    local_305c = 0xc80064;
    local_3058 = 0x190012c;
    local_3054 = 0x25801f4;
    sStack_304e = 0;
    local_3038 = 0x3f800000;
    local_3034 = 0x3f28f5c3;
    local_3030 = 0x3f800000;
    local_302c = 0x3f28f5c3;
    local_3048 = 0x3f800000;
    local_3044 = 0x3f800000;
    local_3040 = 0x3f800000;
    local_303c = 0x3f800000;
    local_3028 = 0x3f800000;
    local_3024 = 0x3f800000;
    local_3020 = 0x3f800000;
    local_301c = 0x3f800000;
    QueryPerformanceCounter(&local_3068);
    uVar8 = __allmul(local_3068.s.LowPart,local_3068.s.HighPart,1000,0);
    uVar6 = __alldiv(uVar8,DAT_006ac8f8,DAT_006ac8fc);
    local_3018 = uVar6 - DAT_0071cfd0;
    iStack_3014 = -(uint)(uVar6 < DAT_0071cfd0) - DAT_0071cfd4;
    psVar7 = (short *)&local_305c;
    iVar5 = 6;
    do {
      *psVar7 = *psVar7 + sVar1;
      psVar7 = psVar7 + 1;
      iVar5 = iVar5 + -1;
    } while (iVar5 != 0);
    local_3068.s.LowPart._0_2_ = (undefined2)_DAT_0069c63c;
    local_3068.s.LowPart._0_2_ = local_3068.s.LowPart._0_2_ + 0x20;
    local_3068.s.LowPart._2_2_ = (undefined2)((uint)_DAT_0069c63c >> 0x10);
    local_3068.s.HighPart._0_2_ = (undefined2)_DAT_0069c640;
    local_3068.s.HighPart._2_2_ = (undefined2)((uint)_DAT_0069c640 >> 0x10);
    local_3068.s.HighPart._0_2_ = local_3068.s.HighPart._0_2_ + 0x20;
    FUN_004944c0(1,0xffffffff,0,0,5,0);
    _sprintf(local_3008,"|n|tframerate|taverage (of %d)|tmin|tmax|tdropped");
    uVar3 = local_3054;
    uVar2 = local_3058;
    local_305c._0_2_ = sVar1;
    _DAT_006e474a = sVar1;
    DAT_006e474a_2 = local_305c._2_2_;
    DAT_006e473c = local_3044;
    DAT_006e4740 = local_3040;
    DAT_006e4738 = local_3048;
    DAT_006e4748 = 6;
    _DAT_006e474e = local_3058;
    _DAT_006e4752 = local_3054;
    DAT_006e4744 = local_303c;
    chimera__draw_8_bit_text(&local_3050,0xfffffffc,local_3008);
    local_3068.s.LowPart._0_2_ = sStack_304e + -1;
    _sprintf(local_3008,"|t%.0f|t%.0f/%.0f|t%.0f|t%.0f|t%5.1f%%|n",(double)_DAT_007c30a0,
             (double)_DAT_007c30a8,
             (double)(((float)CONCAT44(iStack_300c,local_3010) * 1000.0) /
                     (float)CONCAT44(iStack_3014,local_3018)),(double)_DAT_007c30ac,
             (double)_DAT_007c30b0,(double)_DAT_007c30b4);
    local_305c = CONCAT22(local_305c._2_2_,sVar1);
    _DAT_006e474a = local_305c;
    DAT_006e4738 = local_3038;
    DAT_006e473c = local_3034;
    DAT_006e4748 = 6;
    _DAT_006e474e = uVar2;
    _DAT_006e4752 = uVar3;
    DAT_006e4740 = local_3030;
    DAT_006e4744 = local_302c;
    chimera__draw_8_bit_text(&local_3050,0xfffffffc,local_3008);
    DAT_006e4738 = local_3028;
    DAT_006e4748 = 0;
    DAT_006e473c = local_3024;
    DAT_006e4740 = local_3020;
    DAT_006e4744 = local_301c;
  }
  return;
}
#endif
