// movie_play_bink  (Ghidra: movie_play_bink, already named)
// address 0x43ed20, size 740 bytes
// name confidence: 0.6   rewrite confidence: 0.8
// evidence: rewritten from the raw disassembly 0x43ed20..0x43f003 (phase 4 review); Ghidra's
//   decompile does not settle (it treats EBP, which holds the HBINK returned by _BinkOpen@8, as
//   an inherited register). Callers: the main loop 0x4c775a / 0x4c7767 / 0x4c7774 (the three
//   intro movies) and 0x4921e1 (push 0x669910), each `push name; call; add esp,4`, so the only
//   argument is the .bik path on the stack (cdecl). The device is rasterizer_device 0x0071d174
//   (types/rasterizer.h, IDirect3DDevice9): +0x0c TestCooperativeLevel, +0x88 StretchRect,
//   +0x90 CreateOffscreenPlainSurface, +0x98 GetRenderTarget; the surface is IDirect3DSurface9:
//   +0x08 Release, +0x34 LockRect, +0x38 UnlockRect. 0x88760869 is D3DERR_DEVICENOTRESET.
//   BinkCopyToBuffer flags 3 = BINKSURFACE32, 0x80000000 = BINKCOPYALL. bink_movie_prefix
//   (types/main.h) supplies frame_count +0x08, frame_index +0x0c and paused +0xfc.
// register convention: cdecl, one stack argument (the movie path). No register arguments.
//   rasterizer_device_reset 0x515d90 takes its d3d_present_parameters * on the stack (`push edx`
//   at 0x43eeae, `add esp,4` after), as at its other callers 0x4956d1 / 0x4bb86e / 0x517563.
// UNSURE: 0x007196d4 (owner unknown; nonzero skips or aborts playback) is named after its use
//   here only.
// Note: when GetRenderTarget fails the original releases the render target slot (still NULL
//   unless D3D wrote it) and leaks the offscreen surface (0x43ed87 -> 0x43eff5). Kept verbatim.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "interface.h"
#include "shell.h"
#include "main.h"
#include <stdint.h> // uintptr_t

extern int32_t movie_playback_abort;       // 0x007196d4, UNSURE owner (see header)
extern void *rasterizer_device;            // 0x0071d174, IDirect3DDevice9
extern d3d_present_parameters rasterizer_present_parameters; // 0x007c04a0
extern uint8_t rasterizer_device_lost;     // 0x007c10b0
extern uint32_t __stdcall BinkOpenDirectSound(uint32_t param); // import 0x6a0050 (its address is handed to BinkSetSoundSystem)

extern uint8_t rasterizer_device_reset(d3d_present_parameters *present_parameters); // 0x515d90, foreign (rasterizer); stack arg
extern void rasterizer_capture_and_present(const int16_t *tile, BitmapData *bitmap); // 0x518180, foreign (rasterizer)
    // blam-cc: EAX -> tile, stack -> bitmap

extern int32_t __stdcall BinkSetSoundSystem(void *open, uint32_t param);                 // import 0x6a006c
extern bink_movie_prefix *__stdcall BinkOpen(const char *name, uint32_t flags);           // import 0x6a0064
extern void __stdcall BinkClose(bink_movie_prefix *bink);                                 // import 0x6a0068
extern int32_t __stdcall BinkPause(bink_movie_prefix *bink, int32_t pause);               // import 0x6a0058
extern int32_t __stdcall BinkWait(bink_movie_prefix *bink);                               // import 0x6a0060
extern int32_t __stdcall BinkDoFrame(bink_movie_prefix *bink);                            // import 0x6a0070
extern void __stdcall BinkNextFrame(bink_movie_prefix *bink);                             // import 0x6a005c
extern int32_t __stdcall BinkCopyToBuffer(bink_movie_prefix *bink, void *dest, int32_t dest_pitch,
    uint32_t dest_height, uint32_t dest_x, uint32_t dest_y, uint32_t flags);             // import 0x6a0054
extern int32_t __stdcall PeekMessageA(win32_msg *message, void *window, uint32_t filter_min,
    uint32_t filter_max, uint32_t remove);                                                // import 0x63a368
extern int32_t __stdcall TranslateMessage(const win32_msg *message);                      // import 0x63a36c
extern int32_t __stdcall DispatchMessageA(const win32_msg *message);                      // import 0x63a364

typedef int32_t (__stdcall *d3d_test_cooperative_level_fn)(void *device);
typedef int32_t (__stdcall *d3d_create_offscreen_plain_surface_fn)(void *device, uint32_t width,
    uint32_t height, uint32_t format, uint32_t pool, void **surface, void *shared_handle);
typedef int32_t (__stdcall *d3d_get_render_target_fn)(void *device, uint32_t index, void **surface);
typedef int32_t (__stdcall *d3d_stretch_rect_fn)(void *device, void *source, const void *source_rect,
    void *dest, const void *dest_rect, uint32_t filter);
typedef uint32_t (__stdcall *d3d_release_fn)(void *object);
typedef int32_t (__stdcall *d3d_lock_rect_fn)(void *surface, d3d_locked_rect *locked,
    const void *rect, uint32_t flags);
typedef int32_t (__stdcall *d3d_unlock_rect_fn)(void *surface);

#define D3D_VTABLE(object) (*(void ***)(object))

// Plays one Bink movie full screen at 640x480: each frame is decoded into an X8R8G8B8
// offscreen surface and stretched onto the render target, then presented. Escape or space
// (key down followed by key up) or the window close command ends playback early, as does the
// movie reaching its last frame or 0x007196d4 becoming nonzero. A lost device pauses the movie;
// once the device can be reset the surfaces are recreated and playback resumes with a full
// frame copy.
void movie_play_bink(const char *movie_path)
{
    void *offscreen_surface;        // [esp+0x08] after push ebx
    void *render_target;            // [esp+0x0c]
    uint8_t skip;                   // [esp+0x05]
    uint8_t copy_all;               // [esp+0x06] 1 until the first frame after a resume is copied
    uint8_t skip_key_down;          // [esp+0x07] escape or space went down first
    bink_movie_prefix *bink;        // ebp
    win32_msg message;
    d3d_locked_rect locked;
    d3d_present_parameters present_parameters;
    int32_t result;

    render_target = 0;
    offscreen_surface = 0;
    skip = 0;
    copy_all = 1;
    skip_key_down = 0;
    if (movie_playback_abort != 0) {
        return;
    }

    if (((d3d_create_offscreen_plain_surface_fn)D3D_VTABLE(rasterizer_device)[0x90 / 4])(
            rasterizer_device, 0x280, 0x1e0, 0x16 /* D3DFMT_X8R8G8B8 */, 0 /* D3DPOOL_DEFAULT */,
            &offscreen_surface, 0) != 0) {
        return;
    }
    if (((d3d_get_render_target_fn)D3D_VTABLE(rasterizer_device)[0x98 / 4])(
            rasterizer_device, 0, &render_target) != 0) {
        ((d3d_release_fn)D3D_VTABLE(render_target)[0x08 / 4])(render_target);
        return;
    }

    BinkSetSoundSystem((void *)BinkOpenDirectSound, 0);
    bink = BinkOpen(movie_path, 0);
    if (bink != 0) {
        do {
            if (PeekMessageA(&message, 0, 0, 0, 1 /* PM_REMOVE */) != 0) {
                do {
                    TranslateMessage(&message);
                    if (message.message == 0x100) {             // WM_KEYDOWN
                        if (message.wparam == 0x1b || message.wparam == 0x20) {
                            skip_key_down = 1;
                        }
                    } else if (message.message == 0x101) {      // WM_KEYUP
                        if (skip_key_down != 0 && (message.wparam == 0x1b || message.wparam == 0x20)) {
                            skip = 1;
                        }
                    } else if (message.message == 0x112) {      // WM_SYSCOMMAND
                        if (message.wparam == 0xf060) {         // SC_CLOSE
                            skip = 1;
                        }
                    }
                    DispatchMessageA(&message);
                } while (PeekMessageA(&message, 0, 0, 0, 1) != 0);
                if (skip != 0) {
                    break;
                }
            }

            result = ((d3d_test_cooperative_level_fn)D3D_VTABLE(rasterizer_device)[0x0c / 4])(
                rasterizer_device);
            if (result == (int32_t)0x88760869) {                // D3DERR_DEVICENOTRESET
                if (bink->paused == 0) {
                    BinkPause(bink, 1);
                }
                if (render_target != 0) {
                    ((d3d_release_fn)D3D_VTABLE(render_target)[0x08 / 4])(render_target);
                    render_target = 0;
                }
                if (offscreen_surface != 0) {
                    ((d3d_release_fn)D3D_VTABLE(offscreen_surface)[0x08 / 4])(offscreen_surface);
                    offscreen_surface = 0;
                }
                present_parameters = rasterizer_present_parameters;
                rasterizer_device_reset(&present_parameters);
                rasterizer_device_lost = 0;
                ((d3d_create_offscreen_plain_surface_fn)D3D_VTABLE(rasterizer_device)[0x90 / 4])(
                    rasterizer_device, 0x280, 0x1e0, 0x16, 0, &offscreen_surface, 0);
                ((d3d_get_render_target_fn)D3D_VTABLE(rasterizer_device)[0x98 / 4])(
                    rasterizer_device, 0, &render_target);
            } else if (result != 0) {
                if (bink->paused == 0) {
                    BinkPause(bink, 1);
                }
            } else {
                if (bink->paused != 0) {
                    BinkPause(bink, 0);
                    copy_all = 1;
                }
                if (BinkWait(bink) == 0 && offscreen_surface != 0 && render_target != 0) {
                    BinkDoFrame(bink);
                    if (((d3d_lock_rect_fn)D3D_VTABLE(offscreen_surface)[0x34 / 4])(
                            offscreen_surface, &locked, 0, 0) == 0) {
                        BinkCopyToBuffer(bink, (void *)(uintptr_t)locked.bits, locked.pitch, 0x1e0, 0, 0,
                            (copy_all != 0 ? 0x80000000u : 0) + 3 /* BINKSURFACE32 */);
                        copy_all = 0;
                        ((d3d_unlock_rect_fn)D3D_VTABLE(offscreen_surface)[0x38 / 4])(offscreen_surface);
                    }
                    BinkNextFrame(bink);
                    ((d3d_stretch_rect_fn)D3D_VTABLE(rasterizer_device)[0x88 / 4])(
                        rasterizer_device, offscreen_surface, 0, render_target, 0, 0);
                    rasterizer_capture_and_present(0, 0);
                }
            }
        } while (bink->frame_index != bink->frame_count && movie_playback_abort == 0);
        BinkClose(bink);
    }
    ((d3d_release_fn)D3D_VTABLE(offscreen_surface)[0x08 / 4])(offscreen_surface);
    ((d3d_release_fn)D3D_VTABLE(render_target)[0x08 / 4])(render_target);
}

#if 0
Original Ghidra decompilation (0x43ed20); it does not settle and is superseded by the
disassembly, which is the source of the rewrite above:

/* WARNING: Type propagation algorithm not settling */

void movie_play_bink(void)

{
  int iVar1;
  BOOL BVar2;
  int iVar3;
  int iVar4;
  int *unaff_EBP;
  undefined4 *puVar5;
  undefined4 unaff_EDI;
  undefined4 *puVar6;
  bool bVar7;
  int *piVar8;
  undefined4 uStack_a0;
  int *piStack_9c;
  tagMSG tStack_8c;
  undefined4 uStack_70;
  undefined4 local_64;
  undefined4 local_60;
  undefined4 uStack_2c;

  local_60 = 0;
  local_64 = 0;
  if (DAT_007196d4 != 0) {
    return;
  }
  uStack_70 = 0;
  tStack_8c.pt.y = (LONG)&stack0xffffff9c;
  tStack_8c.pt.x = 0;
  tStack_8c.time = 0x16;
  tStack_8c.lParam = 0x1e0;
  tStack_8c.wParam = 0x280;
  tStack_8c.message = (UINT)DAT_0071d174;
  tStack_8c.hwnd = (HWND)0x43ed69;
  iVar1 = (**(code **)(*DAT_0071d174 + 0x90))();
  if (iVar1 != 0) {
    return;
  }
  tStack_8c.hwnd = (HWND)&tStack_8c.time;
  iVar1 = (**(code **)(*DAT_0071d174 + 0x98))();
  if (iVar1 != 0) goto LAB_0043eff5;
  piStack_9c = (int *)0x0;
  uStack_a0 = _BinkOpenDirectSound_4_exref;
  _BinkSetSoundSystem_8();
  iVar1 = _BinkOpen_8();
  if (iVar1 != 0) {
    do {
      BVar2 = PeekMessageA(&tStack_8c,(HWND)0x0,0,0,1);
      if (BVar2 != 0) {
        do {
          TranslateMessage(&tStack_8c);
          if ((undefined1 *)tStack_8c.message == &DAT_00000100) {
            if ((tStack_8c.wParam == 0x1b) || (tStack_8c.wParam == 0x20)) {
              uStack_a0 = (code *)CONCAT13(1,(undefined3)uStack_a0);
            }
          }
          else if ((int *)tStack_8c.message == (int *)0x101) {
            if (uStack_a0._3_1_ != '\0') {
              if (tStack_8c.wParam != 0x1b) {
                bVar7 = tStack_8c.wParam == 0x20;
                goto LAB_0043ee25;
              }
LAB_0043ee27:
              uStack_a0._0_2_ = CONCAT11(1,(undefined1)uStack_a0);
            }
          }
          else if ((int *)tStack_8c.message == (int *)0x112) {
            bVar7 = tStack_8c.wParam == 0xf060;
LAB_0043ee25:
            if (bVar7) goto LAB_0043ee27;
          }
          DispatchMessageA(&tStack_8c);
          BVar2 = PeekMessageA(&tStack_8c,(HWND)0x0,0,0,1);
        } while (BVar2 != 0);
        if (uStack_a0._1_1_ != '\0') goto LAB_0043efe1;
      }
      iVar3 = (**(code **)(*DAT_0071d174 + 0xc))();
      if (iVar3 == -0x7789f797) {
        if (*(int *)(iVar1 + 0xfc) == 0) {
          _BinkPause_8(iVar1);
        }
        if (unaff_EBP != (int *)0x0) {
          (**(code **)(*unaff_EBP + 8))();
          unaff_EBP = (int *)0x0;
        }
        if (piStack_9c != (int *)0x0) {
          (**(code **)(*piStack_9c + 8))();
          piStack_9c = (int *)0x0;
        }
        puVar5 = &DAT_007c04a0;
        puVar6 = &uStack_70;
        for (iVar3 = 0xe; iVar3 != 0; iVar3 = iVar3 + -1) {
          *puVar6 = *puVar5;
          puVar5 = puVar5 + 1;
          puVar6 = puVar6 + 1;
        }
        rasterizer_device_reset();
        DAT_007c10b0 = 0;
        (**(code **)(*DAT_0071d174 + 0x90))(DAT_0071d174,0x280,0x1e0,0x16,0,&piStack_9c);
        (**(code **)(*DAT_0071d174 + 0x98))(DAT_0071d174,0,&stack0xffffff4c);
      }
      else if (iVar3 == 0) {
        if (*(int *)(iVar1 + 0xfc) != 0) {
          _BinkPause_8(iVar1);
          uStack_a0._0_3_ = CONCAT12(1,(undefined2)uStack_a0);
        }
        iVar3 = _BinkWait_4();
        if (((iVar3 == 0) && (piStack_9c != (int *)0x0)) && (unaff_EBP != (int *)0x0)) {
          iVar3 = iVar1;
          _BinkDoFrame_4();
          iVar4 = (**(code **)(*(int *)uStack_a0 + 0x34))(uStack_a0,&stack0xffffff68,0,0);
          if (iVar4 == 0) {
            piVar8 = (int *)0x0;
            _BinkCopyToBuffer_28
                      (iVar1,0,uStack_2c,0x1e0,0,0,
                       (-(uint)((char)((uint)iVar3 >> 0x10) != '\0') & 0x80000000) + 3);
            (**(code **)(*piVar8 + 0x38))(piVar8);
          }
          _BinkNextFrame_4(iVar1);
          (**(code **)(*DAT_0071d174 + 0x88))(DAT_0071d174,iVar3,0,unaff_EDI,0,0);
          FUN_00518180();
        }
      }
      else if (*(int *)(iVar1 + 0xfc) == 0) {
        _BinkPause_8(iVar1);
      }
      if ((*(int *)(iVar1 + 0xc) == *(int *)(iVar1 + 8)) || (DAT_007196d4 != 0)) goto LAB_0043efe1;
    } while( true );
  }
LAB_0043efea:
  (**(code **)(*piStack_9c + 8))();
LAB_0043eff5:
  piStack_9c = (int *)0x43efff;
  (**(code **)(*(int *)tStack_8c.message + 8))();
  return;
LAB_0043efe1:
  _BinkClose_4();
  goto LAB_0043efea;
}

Disassembly anchors used for the rewrite (esp offsets after push ebx / push ebp / push esi,edi):
  43ed35  mov [esp+0x5],bl ; mov [esp+0x6],1 ; mov [esp+0x7],bl  skip, copy_all, skip_key_down
  43ed4f  CreateOffscreenPlainSurface(dev,0x280,0x1e0,0x16,0,&[esp+0x8],0)  jne -> return
  43ed78  GetRenderTarget(dev,0,&[esp+0xc])                                 jne -> 43eff5
  43ed95  BinkSetSoundSystem([0x6a0050],0) ; 43eda1 BinkOpen([esp+0x74]=arg,0) -> ebp
  43ee47  cmp skip,0 ; jne 43efe1 (only after at least one message was pumped)
  43ee5c  cmp eax,0x88760869
  43ef69  flags = (copy_all ? 0x80000000 : 0) + 3 ; 43ef99 copy_all = 0 before UnlockRect
  43efb2  StretchRect(dev,[esp+0x14]=offscreen,0,[esp+0x18]=render_target,0,0)
  43efc2  push 0 ; xor eax,eax ; call 0x518180
  43efcd  cmp [ebp+0xc],[ebp+0x8] ; je close ; cmp [0x7196d4],0 ; je loop
  43efe1  BinkClose ; release [esp+0xc]=offscreen ; pop ebp ; release [esp+0xc]=render_target
#endif
