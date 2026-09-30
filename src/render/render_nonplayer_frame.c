// render_nonplayer_frame  (Ghidra: FUN_0050bdc0; named from types/render.h's module header,
// which documents this address throughout as "render_nonplayer_frame 0x50bdc0", the sibling of
// render_player_frame 0x50ba80)
// address 0x50bdc0, size 211 bytes
// name confidence: 0.6   rewrite confidence: 0.8
// evidence: out/phase4/render_types_notes.md's register-conventions section documents this
//   function's arguments (EAX = nonplayer type, one stack view). Disassembly (objdump -d -M
//   intel, 0x50bdc0..0x50be94) confirms the local 0x258 byte block this function zeroes and
//   fills is a rasterizer_window_parameters (types/rasterizer.h): tracking ESP through the two
//   deferred "push 0x1" call arguments (their cdecl cleanup is batched into one "add esp,0xc"
//   after the third call) shows the rasterizer_camera copy and the second
//   chimera__render_camera_build_frustum call land exactly on the struct's .camera (+0x08) and
//   .frustum (+0x5c) members, and the three explicit field writes right before
//   rasterizer_begin_frame land on .type (+0x00, =1), .window_index (+0x02, =-1) and
//   .clear_target (+0x05, = (nonplayer == 0)). rasterizer_begin_frame copies *source into the
//   global rasterizer_window wholesale, so the closing check against its window_index is a
//   genuine re-read of that shared global, not of this function's own local copy.
// register convention: EAX = nonplayer (nonzero for a non-player, non-loading-screen window; 0
//   selects the loading screen / letterbox / console overlay path), stack = view.
//   // blam-cc: EAX=nonplayer, stack=view

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "render.h"
#include "fn_game.h"
#include "fn_cutscene.h"
#include "fn_render.h"

extern render_camera render_camera_global;     // 0x007c3114, this module (named
                                               // render_camera_global; a variable cannot share
                                               // the render_camera typedef's own name in C)
extern render_frustum render_frustum_global;   // 0x007c3168, this module (see above)
extern rasterizer_window_parameters rasterizer_window; // 0x007c1220, rasterizer module

extern void chimera__render_camera_build_frustum(float *bounds, render_camera *camera,
                                                  render_frustum *frustum_out,
                                                  uint8_t build_projection); // 0x50cc40, this module;
    // blam-cc: EAX=bounds (0 here), ECX=camera, ESI=frustum_out, stack=validate

extern void rasterizer_begin_frame(rasterizer_window_parameters *source); // 0x5175c0, rasterizer
                                                                          // module


extern void ui_error_modal_update(void);                      // 0x494ca0
extern void hud_timer_draw(void); // 0x4add10
extern void chimera__do_show_loading_screen(void);   // 0x497410
extern void console_draw_overlay(void);              // 0x496730


extern rasterizer_frame_statistics rasterizer_frame_statistics_state; // 0x007c30a0, this module
extern void rasterizer_frame_statistics_sample(rasterizer_frame_statistics *statistics,
                                               uint8_t dropped);
    // 0x512530, this module; blam-cc: EBX -> statistics (always 0x007c30a0), stack -> dropped


// Draws the non-3D placeholder pass for a window whose camera/unit is not valid: latches the
// view's two cameras into the global render camera/frustum and a bare rasterizer_window_parameters
// (no fog, no mirror) and begins the frame, then either shows the loading screen / letterbox /
// console overlay (nonplayer == 0) or runs the alternate non-player path, and finally draws the
// framerate statistics overlay if the window that ended up current is the special (-1) one.
void render_nonplayer_frame(uint32_t nonplayer, render_view *view) // blam-cc: EAX=nonplayer, stack=view
{
    rasterizer_window_parameters params = {0};

    render_camera_global = view->source_camera;
    chimera__render_camera_build_frustum(0, &render_camera_global, &render_frustum_global, 1);

    params.camera = view->rasterizer_camera;
    chimera__render_camera_build_frustum(0, &params.camera, &params.frustum, 1);

    params.type = 1;
    params.window_index = -1;
    params.clear_target = (uint8_t)(nonplayer == 0);

    rasterizer_begin_frame(&params);

    if (nonplayer == 0) {
        chimera__letterbox();
        ui_error_modal_update();
        hud_timer_draw();
        chimera__do_show_loading_screen();
        console_draw_overlay();
    } else {
        game_engine_maybe_render_post_game();
    }

    if (rasterizer_window.window_index == -1) {
        rasterizer_frame_statistics_sample(&rasterizer_frame_statistics_state, 0);
        rasterizer_frame_statistics_draw();
    }
}

#if 0
Original Ghidra decompilation (0x50bdc0):

void FUN_0050bdc0(undefined4 *param_1)

{
  int in_EAX;
  int iVar1;
  undefined4 *puVar2;
  short *psVar3;
  undefined4 *puVar4;
  short local_260 [2];
  undefined1 local_25b;
  undefined4 local_258 [149];

  psVar3 = local_260;
  for (iVar1 = 0x96; iVar1 != 0; iVar1 = iVar1 + -1) {
    psVar3[0] = 0;
    psVar3[1] = 0;
    psVar3 = psVar3 + 2;
  }
  puVar4 = &DAT_007c3114;
  puVar2 = param_1;
  for (iVar1 = 0x15; puVar2 = puVar2 + 1, iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar4 = *puVar2;
    puVar4 = puVar4 + 1;
  }
  chimera__render_camera_build_frustum(1);
  puVar2 = param_1 + 0x16;
  puVar4 = local_258;
  for (iVar1 = 0x15; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar4 = *puVar2;
    puVar2 = puVar2 + 1;
    puVar4 = puVar4 + 1;
  }
  chimera__render_camera_build_frustum(1);
  local_25b = in_EAX == 0;
  local_260[0] = 1;
  local_260[1] = 0xffff;
  rasterizer_begin_frame(local_260);
  if (in_EAX == 0) {
    chimera__letterbox();
    FUN_00494ca0();
    chimera__fix_counters_timer_begin();
    chimera__do_show_loading_screen();
    console_draw_overlay();
  }
  else {
    FUN_00461a80();
  }
  if (DAT_007c1220._2_2_ == -1) {
    rasterizer_frame_statistics_sample(0);
    rasterizer_frame_statistics_draw();
  }
  return;
}

Relevant disassembly (objdump -d -M intel, 0x50bdc0..0x50be94):

0050bdc0:
  push   ebp
  mov    ebp,esp
  and    esp,0xfffffff8
  sub    esp,0x25c
  push   ebx
  push   esi
  mov    esi,DWORD PTR [ebp+0x8]     ; esi = param_1 (view*)
  push   edi
  mov    ebx,eax                     ; ebx = entry EAX (nonplayer)
  xor    eax,eax
  mov    ecx,0x96
  lea    edi,[esp+0x10]              ; &params (base of the zeroed 0x258 byte block)
  rep stos DWORD PTR es:[edi],eax
  add    esi,0x4                     ; esi = &view->source_camera
  mov    ecx,0x15
  mov    edi,0x7c3114                ; render_camera_global
  rep movs DWORD PTR es:[edi],DWORD PTR ds:[esi]
  push   0x1
  mov    esi,0x7c3168                ; render_frustum_global
  mov    ecx,0x7c3114
  call   0x50cc40                    ; chimera__render_camera_build_frustum(EAX=0, ECX=camera, ESI=frustum_out, stack=1)
  mov    esi,DWORD PTR [ebp+0x8]
  add    esi,0x58                    ; esi = &view->rasterizer_camera
  mov    ecx,0x15
  lea    edi,[esp+0x1c]              ; &params.camera (base+8, since ESP is still down by 4
                                     ;   from the un-popped "push 0x1" above)
  rep movs DWORD PTR es:[edi],DWORD PTR ds:[esi]
  push   0x1
  lea    esi,[esp+0x74]              ; &params.frustum (base+0x5c)
  xor    eax,eax
  lea    ecx,[esp+0x20]              ; &params.camera again
  call   0x50cc40
  test   ebx,ebx
  sete   al                          ; al = (nonplayer == 0)
  lea    ecx,[esp+0x18]              ; &params (base)
  or     esi,0xffffffff              ; esi = -1
  push   ecx
  mov    WORD PTR [esp+0x1c],0x1     ; params.type = 1
  mov    BYTE PTR [esp+0x21],al      ; params.clear_target = (nonplayer == 0)
  mov    WORD PTR [esp+0x1e],si      ; params.window_index = -1
  call   0x5175c0                    ; rasterizer_begin_frame(&params)
  mov    eax,ebx
  add    esp,0xc                     ; cleans up both deferred "push 0x1"s plus "push ecx"
  sub    eax,0x0
  je     0x50be56                    ; nonplayer == 0
  call   0x461a80
  jmp    0x50be6f
0050be56:
  call   0x4499c0
  call   0x494ca0
  call   0x4add10
  call   0x497410
  call   0x496730
0050be6f:
  cmp    WORD PTR ds:0x7c1222,si     ; rasterizer_window.window_index == -1 (si is still -1)
  jne    0x50be8c
  push   0x0
  mov    ebx,0x7c30a0
  call   0x512530
  call   0x512e80
#endif
