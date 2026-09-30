// render_frame  (Ghidra: render_views_draw_all; renamed to match types/render.h's module header
// and out/phase4/render_types_notes.md, which document this exact address throughout as
// "render_frame 0x50bea0")
// address 0x50bea0, size 269 bytes
// VERIFIED against disassembly 0x50bea0..0x50bfad (2026-09-30). the disassembly does test the screenshot page pointer
//   (0x50bf3c) before dereferencing it
// name confidence: 0.6   rewrite confidence: 0.8
// evidence: out/phase4/render_types_notes.md's register-conventions section: "EBX = screenshot
//   index (Point2DInt*, never loaded inside the function), plus five stack arguments (views,
//   count, screenshot page index, time since tick, time since frame). The combined tile is
//   page*0x00696568 + index." Ghidra's own decompile of this function is materially incomplete
//   (it drops the whole screenshot-tile computation and mis-signatures the two render calls), so
//   this rewrite follows disassembly (objdump -d -M intel, 0x50bea0..0x50bfaf) throughout; see
//   the note inline at the tile computation and the #if 0 block below for the full trace.
// review fix (phase-4 gate): 0x511df0 receives a 16 byte rasterizer_frame_time in ECX, not a
//   bare double; its two trailing dwords are zeroed at 0x50becd..0x50bef2.
// register convention: EBX = screenshot_tile (Point2DInt*, may be 0), stack = (views, count,
//   screenshot_page [Point2DInt*, may be 0], time_since_tick, time_since_frame).
//   // blam-cc: EBX=screenshot_tile, stack=(views, count, screenshot_page, time_since_tick, time_since_frame)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "render.h"
#include "game.h"

extern int32_t render_frame_index;      // 0x007c3100, this module
extern float render_time_since_tick;    // 0x007c310c, this module
extern float render_time_since_frame;   // 0x007c3110, this module
extern int16_t render_window_index;     // 0x007c310a, this module
extern int16_t screenshot_scale;        // 0x00696568, UNSURE (see src/rasterizer/lens_flare_add_instance.c)
extern game_time_globals *game_time;    // 0x006f1d6c

extern void render_cinematic_screen_effect_update(rasterizer_frame_time *time_source);
    // 0x511df0, this module; blam-cc: ECX -> time_source

extern uint8_t rasterizer_reset_device_if_needed(void); // 0x517500, rasterizer module
extern void ui_draw_trouble_brewing_indicator(void);    // 0x49c870
extern void rasterizer_end_frame(void);          // 0x517b90
extern void rasterizer_unbind_stream_and_textures(void);                         // 0x518130

extern void render_player_frame(Point2DInt *screenshot_tile, render_view *view); // 0x50ba80,
    // this module; blam-cc: EAX=screenshot_tile, stack=view
extern void render_nonplayer_frame(uint32_t nonplayer, render_view *view); // 0x50bdc0, this
    // module; blam-cc: EAX=nonplayer, stack=view

// Top-level per-frame render dispatcher: bumps the frame index and latches the tick/frame time
// globals, dispatches the cinematic screen effect for the elapsed time, resets the rasterizer
// device if needed, then draws every split-screen viewport (its 3D scene through
// render_player_frame, or its placeholder through render_nonplayer_frame), combining the
// screenshot page and tile indices into the one actually handed to render_player_frame. Finally
// draws the "trouble brewing" indicator and the rasterizer's own per-frame globals/debug pass.
void render_frame(Point2DInt *screenshot_tile, render_view *views, int16_t count,
                   Point2DInt *screenshot_page, float time_since_tick, float time_since_frame)
    // blam-cc: EBX=screenshot_tile, stack=(views, count, screenshot_page, time_since_tick, time_since_frame)
{
    rasterizer_frame_time frame_time;
    int16_t i;

    render_frame_index = render_frame_index + 1;
    render_time_since_tick = time_since_tick;
    render_time_since_frame = time_since_frame;

    frame_time.unknown_08 = 0;
    frame_time.unknown_0c = 0;
    frame_time.time = (double)game_time->game_time * (1.0 / 30.0) + (double)time_since_tick;
    render_cinematic_screen_effect_update(&frame_time);

    if (!rasterizer_reset_device_if_needed()) {
        return;
    }

    for (i = 0; i < count; i++) {
        render_view *view = &views[i];

        render_window_index = i;

        if (view->nonplayer != 0) {
            render_nonplayer_frame(0, view);
        } else if (view->local_player_index == -1) {
            render_nonplayer_frame(1, view);
        } else {
            Point2DInt combined_tile;
            Point2DInt *tile_argument = 0;

            // UNSURE: reached only when screenshot_tile != 0; screenshot_page is dereferenced
            // unconditionally in the compiled code once screenshot_tile passes its own null
            // check, with no separate null check of its own (the source likely never calls this
            // with a tile but no page). Preserved exactly rather than guarded.
            if (screenshot_tile != 0 && screenshot_page != 0) {
                combined_tile.x = (int16_t)(screenshot_page->x * screenshot_scale + screenshot_tile->x);
                combined_tile.y = (int16_t)(screenshot_page->y * screenshot_scale + screenshot_tile->y);
            }
            if (screenshot_tile != 0) {
                tile_argument = &combined_tile;
            }
            render_player_frame(tile_argument, view);
        }
    }

    ui_draw_trouble_brewing_indicator();
    rasterizer_end_frame();
    rasterizer_unbind_stream_and_textures();
}

#if 0
Original Ghidra decompilation (0x50bea0) -- INCOMPLETE, see the evidence note above; kept only
for reference to the parts it did get right (the counters, the loop stride, the two render calls
by name):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void render_views_draw_all
               (short *param_1,short param_2,undefined4 param_3,undefined4 param_4,
               undefined4 param_5)

{
  bool bVar1;
  short sVar2;

  DAT_007c3100 = DAT_007c3100 + 1;
  _DAT_007c310c = param_4;
  DAT_007c3110 = param_5;
  FUN_00511df0();
  bVar1 = rasterizer_reset_device_if_needed();
  if (bVar1) {
    sVar2 = 0;
    if (0 < param_2) {
      do {
        _DAT_007c310a = sVar2;
        if (((char)param_1[1] == '\0') && (*param_1 != -1)) {
          FUN_0050ba80(param_1);
        }
        else {
          FUN_0050bdc0(param_1);
        }
        sVar2 = sVar2 + 1;
        param_1 = param_1 + 0x56;
      } while (sVar2 < param_2);
    }
    ui_draw_trouble_brewing_indicator();
    chimera__rasterizer_globals();
    FUN_00518130();
  }
  return;
}

Disassembly (objdump -d -M intel, 0x50bea0..0x50bfaf), which the rewrite above actually follows:

0050bea0:
  sub    esp,0x14
  mov    edx,DWORD PTR ds:0x7c3100      ; render_frame_index
  mov    eax,DWORD PTR [esp+0x24]       ; arg4 = time_since_tick
  mov    ecx,DWORD PTR [esp+0x28]       ; arg5 = time_since_frame
  inc    edx
  mov    DWORD PTR ds:0x7c3100,edx
  mov    ds:0x7c310c,eax
  mov    eax,ds:0x6f1d6c                ; game_time
  mov    ds:0x7c3110,ecx
  fild   DWORD PTR [eax+0xc]            ; game_time->game_time
  xor    edx,edx
  mov    DWORD PTR [esp+0x4],edx
  fmul   QWORD PTR ds:0x672d88          ; * (double)0.0333333333333333 (== 1/30, read from .rdata)
  push   ebp
  fadd   DWORD PTR [esp+0x28]           ; + time_since_tick (arg4, still at the same real address)
  mov    ebp,DWORD PTR [esp+0x24]       ; ebp = arg3 = screenshot_page (Point2DInt*)
  mov    DWORD PTR [esp+0xc],edx
  push   esi
  mov    DWORD PTR [esp+0x14],edx
  fstp   QWORD PTR [esp+0xc]            ; frame_time (double) stored on the stack
  push   edi
  mov    DWORD PTR [esp+0x1c],edx
  lea    ecx,[esp+0x10]                 ; &frame_time
  call   0x511df0
  call   0x517500                       ; rasterizer_reset_device_if_needed
  test   al,al
  je     0x50bfa6
  xor    edi,edi                        ; i = 0
  cmp    WORD PTR [esp+0x28],di         ; arg2 = count
  jle    0x50bf97
  mov    esi,DWORD PTR [esp+0x24]       ; esi = arg1 = views
0050bf20:
  mov    WORD PTR ds:0x7c310a,di        ; render_window_index = i
  mov    al,BYTE PTR [esi+0x2]          ; view->nonplayer
  test   al,al
  je     0x50bf32
  xor    eax,eax                        ; render_nonplayer_frame(0, view)
  jmp    0x50bf80
0050bf32:
  cmp    WORD PTR [esi],0xffff          ; view->local_player_index == -1
  je     0x50bf7b
  test   ebx,ebx                        ; screenshot_tile == 0
  je     0x50bf67
  test   ebp,ebp                        ; screenshot_page == 0
  je     0x50bf67
  mov    ax,ds:0x696568
  mov    cx,WORD PTR [ebp+0x0]          ; screenshot_page->x
  mov    dx,WORD PTR [ebp+0x2]          ; screenshot_page->y
  imul   cx,ax
  add    cx,WORD PTR [ebx]              ; + screenshot_tile->x
  imul   dx,ax
  add    dx,WORD PTR [ebx+0x2]          ; + screenshot_tile->y
  mov    WORD PTR [esp+0x30],cx         ; combined_tile.x
  mov    WORD PTR [esp+0x32],dx         ; combined_tile.y
0050bf67:
  mov    eax,ebx
  neg    eax
  sbb    eax,eax                        ; eax = (ebx != 0) ? -1 : 0
  lea    ecx,[esp+0x30]                 ; &combined_tile
  and    eax,ecx                        ; eax = screenshot_tile ? &combined_tile : 0
  push   esi
  call   0x50ba80                       ; render_player_frame(eax, view)
  jmp    0x50bf86
0050bf7b:
  mov    eax,0x1                        ; render_nonplayer_frame(1, view)
0050bf80:
  push   esi
  call   0x50bdc0
0050bf86:
  add    esp,0x4
  inc    edi
  add    esi,0xac                       ; views + 1 (render_view stride 0xac)
  cmp    di,WORD PTR [esp+0x28]
  jl     0x50bf20
0050bf97:
  call   0x49c870                       ; ui_draw_trouble_brewing_indicator
  call   0x517b90                       ; chimera__rasterizer_globals
  call   0x518130
#endif
