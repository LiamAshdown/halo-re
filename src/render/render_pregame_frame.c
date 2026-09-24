// render_pregame_frame  (Ghidra: FUN_0050c590; new name, evidence below)
// address 0x50c590, size 207 bytes
// name confidence: 0.45   rewrite confidence: 0.8
// evidence: out/phase4/render_functions.md's phase-2 summary: "Standalone top-level frame path
//   that begins a rasterizer frame and shows the loading screen without any 3D scene content
//   (e.g. no active camera at all)" -- named for that (there is no scenario/camera loaded yet,
//   e.g. the main menu). Disassembly (objdump -d -M intel, 0x50c590..0x50c65f) confirms EAX is a
//   register-passed render_view* (Ghidra's "in_EAX"; the callee-saved copy lives in EBX
//   throughout, exactly as in render_player_frame 0x50ba80 and render_nonplayer_frame 0x50bdc0),
//   and that the local 0x258 byte block is again a rasterizer_window_parameters, built the same
//   way as render_nonplayer_frame's (camera at +0x08, frustum at +0x5c) except that only .type is
//   set (to 1); .window_index and .clear_target are left at the zero-fill value.
// register convention: EAX = view (render_view*), no stack arguments.
//   // blam-cc: EAX=view
// UNSURE: the double passed by address to FUN_00511df0 is never initialized here (the stack slot
//   is read uninitialized in the compiled code); preserved exactly rather than zeroed, since
//   FUN_00511df0 is outside this batch and its use of the pointer is not established.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "render.h"

extern int32_t render_frame_index;           // 0x007c3100, this module
extern render_camera render_camera_global;   // 0x007c3114, this module
extern render_frustum render_frustum_global; // 0x007c3168, this module
extern rasterizer_window_parameters rasterizer_window; // 0x007c1220, rasterizer module

extern void render_cinematic_screen_effect_update(rasterizer_frame_time *time_source);
    // 0x511df0, this module; blam-cc: ECX -> time_source

extern uint8_t rasterizer_reset_device_if_needed(void); // 0x517500, rasterizer module

extern void chimera__render_camera_build_frustum(float *bounds, render_camera *camera,
                                                  render_frustum *frustum_out,
                                                  uint8_t build_projection); // 0x50cc40, this module;
    // blam-cc: EAX=bounds (0 here), ECX=camera, ESI=frustum_out, stack=validate

extern void rasterizer_begin_frame(rasterizer_window_parameters *source); // 0x5175c0, rasterizer
                                                                          // module
extern void widget_draw_fullscreen_region(int16_t controller_index); // 0x4984c0, interface
    // module; blam-cc: AX -> controller_index
extern void chimera__do_show_loading_screen(void); // 0x497410
extern rasterizer_frame_statistics rasterizer_frame_statistics_state; // 0x007c30a0, this module
extern void rasterizer_frame_statistics_sample(rasterizer_frame_statistics *statistics,
                                               uint8_t dropped);
    // 0x512530, this module; blam-cc: EBX -> statistics (always 0x007c30a0), stack -> dropped
extern void rasterizer_frame_statistics_draw(void);               // this module, below this batch
extern void rasterizer_end_frame(void);   // 0x517b90
extern void rasterizer_unbind_stream_and_textures(void);                  // 0x518130

// Draws a frame with no scene at all (no active camera, e.g. before a scenario is loaded):
// latches the view's two cameras into the global render camera/frustum and a bare
// rasterizer_window_parameters (type 1 only; window_index and clear_target stay 0), begins the
// frame, shows the loading screen, and draws the framerate statistics overlay if the window that
// ended up current is the special (-1) one.
void render_pregame_frame(render_view *view) // blam-cc: EAX=view
{
    rasterizer_window_parameters params = {0};
    rasterizer_frame_time frame_time; // UNSURE: left uninitialized, matching the compiled code

    render_frame_index = render_frame_index + 1;
    render_cinematic_screen_effect_update(&frame_time);

    if (!rasterizer_reset_device_if_needed()) {
        return;
    }

    render_camera_global = view->source_camera;
    chimera__render_camera_build_frustum(0, &render_camera_global, &render_frustum_global, 1);

    params.camera = view->rasterizer_camera;
    chimera__render_camera_build_frustum(0, &params.camera, &params.frustum, 1);

    params.type = 1;
    rasterizer_begin_frame(&params);

    widget_draw_fullscreen_region(0);
    chimera__do_show_loading_screen();

    if (rasterizer_window.window_index == -1) {
        rasterizer_frame_statistics_sample(&rasterizer_frame_statistics_state, 0);
        rasterizer_frame_statistics_draw();
    }

    rasterizer_end_frame();
    rasterizer_unbind_stream_and_textures();
}

#if 0
Original Ghidra decompilation (0x50c590):

void FUN_0050c590(void)

{
  bool bVar1;
  undefined4 *in_EAX;
  int iVar2;
  undefined4 *puVar3;
  short *psVar4;
  undefined4 *puVar5;
  short asStack_25c [4];
  undefined4 auStack_254 [149];

  DAT_007c3100 = DAT_007c3100 + 1;
  FUN_00511df0();
  bVar1 = rasterizer_reset_device_if_needed();
  if (bVar1) {
    psVar4 = asStack_25c;
    for (iVar2 = 0x96; iVar2 != 0; iVar2 = iVar2 + -1) {
      psVar4[0] = 0;
      psVar4[1] = 0;
      psVar4 = psVar4 + 2;
    }
    puVar5 = &DAT_007c3114;
    puVar3 = in_EAX;
    for (iVar2 = 0x15; puVar3 = puVar3 + 1, iVar2 != 0; iVar2 = iVar2 + -1) {
      *puVar5 = *puVar3;
      puVar5 = puVar5 + 1;
    }
    chimera__render_camera_build_frustum(1);
    puVar3 = in_EAX + 0x16;
    puVar5 = auStack_254;
    for (iVar2 = 0x15; iVar2 != 0; iVar2 = iVar2 + -1) {
      *puVar5 = *puVar3;
      puVar3 = puVar3 + 1;
      puVar5 = puVar5 + 1;
    }
    chimera__render_camera_build_frustum(1);
    asStack_25c[0] = 1;
    rasterizer_begin_frame(asStack_25c);
    FUN_004984c0();
    chimera__do_show_loading_screen();
    if (DAT_007c1220._2_2_ == -1) {
      rasterizer_frame_statistics_sample(0);
      rasterizer_frame_statistics_draw();
    }
    chimera__rasterizer_globals();
    FUN_00518130();
  }
  return;
}

Disassembly (objdump -d -M intel, 0x50c590..0x50c65f) confirming EAX/EBX and the struct layout:

0050c590:
  mov    edx,DWORD PTR ds:0x7c3100
  sub    esp,0x270
  push   ebx
  push   esi
  inc    edx
  push   edi
  lea    ecx,[esp+0x10]              ; &frame_time (uninitialized)
  mov    ebx,eax                     ; ebx = entry EAX (view*)
  mov    DWORD PTR ds:0x7c3100,edx
  call   0x511df0
  call   0x517500
  test   al,al
  je     0x50c655
  xor    eax,eax
  mov    ecx,0x96
  lea    edi,[esp+0x20]              ; &params (base of the zeroed 0x258 byte block)
  rep stos DWORD PTR es:[edi],eax
  lea    esi,[ebx+0x4]               ; &view->source_camera
  mov    ecx,0x15
  mov    edi,0x7c3114                ; render_camera_global
  rep movs DWORD PTR es:[edi],DWORD PTR ds:[esi]
  push   0x1
  mov    esi,0x7c3168                ; render_frustum_global
  mov    ecx,0x7c3114
  call   0x50cc40
  lea    esi,[ebx+0x58]              ; &view->rasterizer_camera
  mov    ecx,0x15
  lea    edi,[esp+0x2c]              ; &params.camera (base+8)
  rep movs DWORD PTR es:[edi],DWORD PTR ds:[esi]
  push   0x1
  lea    esi,[esp+0x84]              ; &params.frustum (base+0x5c)
  xor    eax,eax
  lea    ecx,[esp+0x30]              ; &params.camera again
  call   0x50cc40
  lea    eax,[esp+0x28]              ; &params (base)
  push   eax
  mov    WORD PTR [esp+0x2c],0x1     ; params.type = 1
  call   0x5175c0                    ; rasterizer_begin_frame(&params)
  add    esp,0xc
  xor    eax,eax
  call   0x4984c0
  call   0x497410
  cmp    WORD PTR ds:0x7c1222,0xffff
  jne    0x50c64b
  push   0x0
  mov    ebx,0x7c30a0
  call   0x512530
  call   0x512e80
0050c64b:
  call   0x517b90
  call   0x518130
#endif
