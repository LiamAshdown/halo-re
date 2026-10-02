// rasterizer_capture_and_present  (Ghidra: FUN_00518180)
// address 0x518180, size 704 bytes
// name confidence: 0.55   rewrite confidence: 0.85
// evidence: raw disassembly 0x518180..0x518446 (phase 4 review). Callers: movie_play_bink
//   0x43efc5, movie_capture_frame_export 0x4c9547, screenshot_render 0x4ca33d (EAX = the tile
//   being rendered), rasterizer_render_loading_screen 0x515894 / 0x5158f8 and clipboard code
//   0x541c3b (EAX = 0, stack 0). The phase 3 file had five register arguments that do not exist
//   and a two argument Present.
// What it does: nothing while the device is lost (0x007c10b0). When 0x00722b54 is set it locks
//   and unlocks the back buffer read only (a GPU flush). When 0x007196e0 is set and a capture
//   bitmap with pixels (+0x2c) is given, it copies the game window rows of the back buffer into
//   that bitmap: the window rectangle (0x0069c634 top, left / 0x0069c638 bottom, right) is moved
//   to tile (x * width, y * height) when a tile is given, the bitmap must be X8R8G8B8 or
//   A8R8G8B8 (format 10 / 11) without mipmaps and must contain the rectangle, and each row is
//   copied with bitmap_data_get_row_address 0x43f8e0 (row bytes = bits per pixel of the format
//   from 0x006571f4 * width / 8). Then Present(NULL, NULL, NULL, NULL): D3DERR_DEVICELOST or
//   D3DERR_DRIVERINTERNALERROR mark the device lost, success toggles 0x0071d16e; finally the 64
//   bit present counter 0x0069c648 is incremented.
// register convention: EAX -> tile (int16 x, y; may be NULL), stack -> bitmap (may be NULL).
// blam-cc: EAX -> tile, stack -> bitmap
// UNSURE: the row copy passes x as the dword at [esp+0x26] (low word the tile left, high word
//   the saved bottom); bitmap_data_get_row_address only uses the low word.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include <string.h> // memcpy
#include <stdint.h> // uintptr_t
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint8_t rasterizer_device_lost;                  // 0x007c10b0
extern void *rasterizer_device;                         // 0x0071d174
extern uint32_t config_disable_buffering;                       // 0x00722b54 config: flush the back buffer before present
extern uint32_t screenshots;                       // 0x007196e0 capture enabled
extern uint8_t rasterizer_pending_clear;                // 0x0071d16e
extern uint32_t game_window_top_left;                   // 0x0069c634 int16 top, left
extern uint32_t game_window_bottom_right;               // 0x0069c638 int16 bottom, right
extern int32_t rasterizer_present_counter_low;                        // 0x0069c648 present counter, low dword
extern int32_t rasterizer_present_counter_high;                        // 0x0069c64c present counter, high dword
extern int8_t bitmap_format_bits_per_pixel[];            // 0x006571f4 indexed by BitmapDataFormat

// blam-cc: bitmap in EDI, mip level in EAX, x and y on the stack
extern uint16_t *bitmap_data_get_row_address(BitmapData *bitmap, int32_t mip_level, int32_t x, int32_t y); // 0x43f8e0

typedef int32_t (__stdcall *d3d_get_back_buffer_fn)(void *self, uint32_t swap_chain, uint32_t index, uint32_t type, void **out_surface);
typedef int32_t (__stdcall *d3d_get_desc_fn)(void *surface, d3d_surface_desc *desc);
typedef int32_t (__stdcall *d3d_lock_rect_fn)(void *surface, d3d_locked_rect *locked, const void *rect, uint32_t flags);
typedef int32_t (__stdcall *d3d_unlock_rect_fn)(void *surface);
typedef uint32_t (__stdcall *com_release_fn)(void *self);
typedef int32_t (__stdcall *d3d_present_fn)(void *self, const void *source_rect, const void *dest_rect, void *window,
                                  const void *dirty_region);

static void **vtable_of(void *object) { return *(void ***)object; }

void rasterizer_capture_and_present(const int16_t *tile, BitmapData *bitmap)
{
    uint8_t ok = 1;
    void *surface;
    d3d_surface_desc desc;
    d3d_locked_rect locked;
    int32_t hr;

    if (rasterizer_device_lost) {
        return;
    }
    if (config_disable_buffering != 0) {
        surface = NULL;
        if (((d3d_get_back_buffer_fn)vtable_of(rasterizer_device)[0x48 / 4])(rasterizer_device, 0, 0, 0, &surface) < 0) {
            ok = 0;
        }
        if (((d3d_get_desc_fn)vtable_of(surface)[0x30 / 4])(surface, &desc) < 0 || !ok ||
            ((d3d_lock_rect_fn)vtable_of(surface)[0x34 / 4])(surface, &locked, NULL, 0x10) < 0) {  // READONLY
            ok = 0;
        } else if (locked.bits != 0 &&
                   ((d3d_unlock_rect_fn)vtable_of(surface)[0x38 / 4])(surface) < 0) {
            ok = 0;
        }
        ((com_release_fn)vtable_of(surface)[2])(surface);
    }
    if (screenshots != 0 && bitmap != NULL && *(uint32_t *)&((struct BitmapData *)bitmap)->pixel_base != 0) {
        int16_t top = (int16_t)(game_window_top_left & 0xffff);
        int16_t left = (int16_t)(game_window_top_left >> 16);
        int16_t bottom = (int16_t)(game_window_bottom_right & 0xffff);
        int16_t right = (int16_t)(game_window_bottom_right >> 16);

        if (tile != NULL) {
            int16_t width = (int16_t)(right - left);
            int16_t height = (int16_t)(bottom - top);

            left = (int16_t)(tile[0] * width);
            top = (int16_t)(tile[1] * height);
            right = (int16_t)(left + width);
            bottom = (int16_t)(top + height);
        }
        if ((bitmap->format == 0xb || bitmap->format == 0xa) && bitmap->mipmap_count == 0 &&
            left >= 0 && top >= 0 && right <= (int16_t)bitmap->width && bottom <= (int16_t)bitmap->height) {
            surface = NULL;
            if (((d3d_get_back_buffer_fn)vtable_of(rasterizer_device)[0x48 / 4])(rasterizer_device, 0, 0, 0, &surface) < 0) {
                ok = 0;
            }
            if (((d3d_get_desc_fn)vtable_of(surface)[0x30 / 4])(surface, &desc) >= 0 && ok &&
                ((d3d_lock_rect_fn)vtable_of(surface)[0x34 / 4])(surface, &locked, NULL, 0x10) >= 0 &&
                locked.bits != 0) {
                int16_t rows = (int16_t)((game_window_bottom_right & 0xffff) - (game_window_top_left & 0xffff));
                int32_t row_bytes = (int32_t)bitmap_format_bits_per_pixel[bitmap->format] *
                                    (int32_t)(int16_t)bitmap->width / 8;
                int16_t row;

                for (row = 0; row < rows; row++) {
                    const uint8_t *source = (const uint8_t *)(uintptr_t)locked.bits + (int32_t)row * locked.pitch;
                    void *destination = bitmap_data_get_row_address(bitmap, 0, left, top + row);

                    memcpy(destination, source, (size_t)row_bytes);
                }
                ((d3d_unlock_rect_fn)vtable_of(surface)[0x38 / 4])(surface);
            }
            ((com_release_fn)vtable_of(surface)[2])(surface);
        }
    }

    hr = ((d3d_present_fn)vtable_of(rasterizer_device)[0x44 / 4])(rasterizer_device, NULL, NULL, NULL, NULL);
    if (hr == (int32_t)0x88760868 || hr == (int32_t)0x88760827) {   // DEVICELOST, DRIVERINTERNALERROR
        rasterizer_device_lost = 1;
    } else if (hr == 0) {
        rasterizer_pending_clear = (uint8_t)(rasterizer_pending_clear == 0);
    }
    {
        uint32_t low = (uint32_t)rasterizer_present_counter_low + 1;
        rasterizer_present_counter_high += (low == 0);
        rasterizer_present_counter_low = (int32_t)low;
    }
}

#if 0
Original Ghidra decompilation (0x518180): phase 3 body replaced from raw disassembly

void FUN_00518180(int param_1)

{
  short sVar1;
  short *in_EAX;
  int iVar2;
  int iVar3;
  uint uVar4;
  undefined4 *puVar5;
  int iVar6;
  uint uVar7;
  undefined2 unaff_BX;
  int unaff_EBP;
  int unaff_ESI;
  undefined4 *puVar8;
  int unaff_EDI;
  bool bVar9;
  int *piVar10;
  int *piVar11;
  undefined2 uStack_46;
  int local_40 [3];
  undefined1 auStack_34 [8];
  short local_2c;
  short sStack_2a;
  short local_28;
  short sStack_26;
  
  uStack_46 = (undefined2)((uint)unaff_ESI >> 0x10);
  bVar9 = true;
  if (DAT_007c10b0 == '\0') {
    if (DAT_00722b54 != 0) {
      piVar11 = (int *)0x0;
      local_40[0] = 0;
      piVar10 = DAT_0071d174;
      iVar2 = (**(code **)(*DAT_0071d174 + 0x48))(DAT_0071d174,0);
      bVar9 = -1 < iVar2;
      iVar2 = (**(code **)(*piVar11 + 0x30))(piVar11,auStack_34);
      if ((iVar2 < 0) ||
         ((bVar9 && ((iVar2 = (**(code **)(*piVar10 + 0x34))(piVar10,&stack0xffffffb0,0,0x10),
                     iVar2 < 0 ||
                     ((&stack0x00000000 != &DAT_00000040 &&
                      (iVar2 = (**(code **)(*piVar10 + 0x38))(piVar10), iVar2 < 0)))))))) {
        bVar9 = false;
      }
      (**(code **)(*piVar10 + 8))(piVar10);
    }
    if (((DAT_007196e0 != 0) && (param_1 != 0)) && (*(int *)(param_1 + 0x2c) != 0)) {
      local_2c = (short)DAT_0069c634;
      sStack_2a = (short)((uint)DAT_0069c634 >> 0x10);
      local_28 = (short)DAT_0069c638;
      sStack_26 = (short)((uint)DAT_0069c638 >> 0x10);
      sVar1 = sStack_26;
      if (in_EAX != (short *)0x0) {
        sVar1 = sStack_26 - sStack_2a;
        local_28 = local_28 - local_2c;
        sStack_2a = *in_EAX * sVar1;
        local_2c = in_EAX[1] * local_28;
        local_28 = local_2c + local_28;
        sVar1 = sVar1 + sStack_2a;
      }
      if (((((*(short *)(param_1 + 0xc) == 0xb) || (*(short *)(param_1 + 0xc) == 10)) &&
           ((iVar2 = 0, *(short *)(param_1 + 0x14) == 0 && ((-1 < sStack_2a && (-1 < local_2c))))))
          && (sVar1 <= *(short *)(param_1 + 4))) && (local_28 <= *(short *)(param_1 + 6))) {
        piVar10 = local_40;
        piVar11 = (int *)0x0;
        local_40[0] = 0;
        iVar3 = (**(code **)(*DAT_0071d174 + 0x48))(DAT_0071d174,0,0,0);
        if (iVar3 < 0) {
          bVar9 = false;
        }
        iVar3 = (**(code **)(*piVar10 + 0x30))(piVar10,auStack_34);
        if ((((-1 < iVar3) && (bVar9)) &&
            (iVar3 = (**(code **)(*piVar11 + 0x34))(piVar11,&stack0xffffffb0,0,0x10), -1 < iVar3))
           && (unaff_EDI != 0)) {
          iVar6 = DAT_0069c638 - DAT_0069c634;
          iVar3 = (int)(char)(&DAT_006571f4)[*(short *)(param_1 + 0xc)] *
                  (int)*(short *)(param_1 + 4);
          uVar4 = (int)(iVar3 + (iVar3 >> 0x1f & 7U)) >> 3;
          if (0 < (short)iVar6) {
            iVar3 = 0;
            do {
              puVar5 = (undefined4 *)
                       bitmap_data_get_row_address(CONCAT22(unaff_BX,uStack_46),unaff_ESI + iVar2);
              puVar8 = (undefined4 *)(iVar3 * unaff_EBP + unaff_EDI);
              for (uVar7 = uVar4 >> 2; uVar7 != 0; uVar7 = uVar7 - 1) {
                *puVar5 = *puVar8;
                puVar8 = puVar8 + 1;
                puVar5 = puVar5 + 1;
              }
              iVar2 = iVar2 + 1;
              iVar3 = iVar3 + 1;
              for (uVar7 = uVar4 & 3; uVar7 != 0; uVar7 = uVar7 - 1) {
                *(undefined1 *)puVar5 = *(undefined1 *)puVar8;
                puVar8 = (undefined4 *)((int)puVar8 + 1);
                puVar5 = (undefined4 *)((int)puVar5 + 1);
              }
            } while ((short)iVar2 < (short)iVar6);
          }
          (**(code **)(*piVar11 + 0x38))(piVar11);
        }
        (**(code **)(*piVar11 + 8))(piVar11);
      }
    }
    iVar2 = (**(code **)(*DAT_0071d174 + 0x44))(DAT_0071d174,0,0);
    if ((iVar2 == -0x7789f798) || (iVar2 == -0x7789f7d9)) {
      DAT_007c10b0 = '\x01';
    }
    else if (iVar2 == 0) {
      DAT_0071d16e = DAT_0071d16e == '\0';
    }
    bVar9 = 0xfffffffe < DAT_0069c648;
    DAT_0069c648 = DAT_0069c648 + 1;
    DAT_0069c64c = DAT_0069c64c + (uint)bVar9;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
