// texture_cache_get  (Ghidra: FUN_00444550; renamed, named directly in types/cache.h's
// texture_cache_entry section: "the index*0x10 + entries->data arithmetic in texture_cache_get
// @0x444550" and "texture_cache_get returns &entry->texture, so its 25 callers hold a void **")
// address 0x444550, size 424 bytes
// name confidence: 0.75  rewrite confidence: 0.70
// evidence: BitmapData field offsets in types/tags.h (flags 0x0e bit 7 == make_it_actually_work,
// bit 8 == external; pointer 0x24; _pad_28/_pad_2c; bitmap_tag_id 0x20; type 0x0a) and
// texture_cache_entry field offsets in types/cache.h (io_request_index 0x02, loaded 0x04,
// converted 0x05, texture 0x0c) all match. Directly parallel to sound_cache_touch @0x443e10,
// already rewritten in this module, for the watchdog-poll loop shape and the age-touch idiom.
// Confirmed field-by-field against raw disassembly (objdump -d -M intel
// --start-address=0x444550 --stop-address=0x4446f8 bin/halo.exe), which is also where the
// three per-bitmap-type conversion calls' three different calling conventions (two stack-passed,
// one EBX-register) and the final fallback call's argument (bitmap in EAX) come from -- none of
// that is visible in Ghidra's own decompile of this function. The "%s" format string at
// 0x0065efec was likewise read directly out of .rdata (Ghidra didn't surface it as a string
// reference for this function).
// register convention: BitmapData *bitmap in EAX (in_EAX); wait and allocate_if_missing are the
// two recognized stack parameters (param_1, param_2), in that order. Both are byte-wide: every
// use of `wait` in the body is `mov al,BYTE PTR [esp+0x1c]; test al,al` (0x004445ba, 0x004446c8,
// 0x004446dd), never a full-dword test, so it is declared uint8_t here even though Ghidra types
// param_1 as undefined4 and the one place it is forwarded (the priority argument to
// texture_cache_page_allocate at 0x00444574) pushes the whole dword -- that callee reads only
// the low byte as well.
//
// UNSURE: FUN_00523fa0, FUN_00524100, FUN_00524270, FUN_005243c0 and FUN_00549960 (FUN_00515c30 is rasterizer_get_capture_surface)
// are all outside this batch's assigned range and are declared here only as opaque externs
// (their calling conventions, where recoverable from this function's own call sites, are noted
// on each prototype). The function's return value is genuinely asymmetric in the original: the
// non-cached path (flags bit 7 clear) returns BitmapData::_pad_28's own value (a D3D texture
// pointer), while the cached path returns the *address* of texture_cache_entry::texture (a
// void **) -- both typed here as plain `void *`, matching the original's `int` return exactly.

// phase-4 review pass: body re-checked instruction by instruction against `objdump -d -M
// intel` of this address range; every field offset, branch and argument below now matches
// the machine code rather than only Ghidra's pseudo-C.
#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "math.h"

extern data_array *texture_cache_entries;  // 0x006ac538
extern struct cache *texture_cache;        // 0x006ac540
extern uint8_t debug_texture_cache_prints; // 0x006f17f6, console toggle, read but not owned here
extern tag_instance *tag_instances;        // 0x0087bc14
extern cache_io_request *cache_io_requests; // 0x006ac4a0
extern int64_t performance_frequency;      // 0x006ac8f8/0x006ac8fc
extern int32_t frame_watchdog_time;        // 0x0072520c

extern void console_print_va(const char *format, ...);       // 0x4c6920

extern uint32_t texture_cache_page_allocate(BitmapData *bitmap, uint8_t priority); // this module, texture_cache_page_allocate.c

extern uint32_t sound_idle_update(void); // outside this module; frame-watchdog pump, UNSURE
extern uint8_t rasterizer_bitmap_create_hardware_texture(BitmapData *bitmap); // 0x523fa0; ESI -> bitmap

// blam-cc: bitmap in EAX; outside this module, UNSURE
extern void rasterizer_bitmap_upload_2d_mipmaps(BitmapData *bitmap); // 2D texture conversion, stack-passed
extern void rasterizer_bitmap_upload_cubemap_mipmaps_by_face(BitmapData *bitmap); // cube map conversion, stack-passed
// blam-cc: bitmap in EBX; outside this module, UNSURE
extern void rasterizer_bitmap_upload_cubemap_mipmaps(BitmapData *bitmap); // 3D texture conversion, register-passed
// blam-cc: bitmap in EAX; outside this module, UNSURE
extern void *rasterizer_get_capture_surface(uint8_t *object, void *fallback); // 0x515c30; EAX -> object, ECX -> fallback:
    // the default texture for the bitmap's type (+0xa): 2D/3D, cube map, or type 3

// blam-cc: bitmap in EAX, wait as the first recognized stack parameter, allocate_if_missing as
// the second
// Fetches the cached Direct3D texture object for a bitmap. If the bitmap's streaming flag
// (make_it_actually_work) is clear, the D3D texture lives directly on the tag and is returned
// as-is. Otherwise: allocates a texture-cache page first if the bitmap has none yet and
// allocate_if_missing is set; touches the page's LRU age; if a caller is waiting and the page
// hasn't started loading, raises its read priority (spewing the tag path to the console when
// debug_texture_cache_prints is set); then either polls once or busy-waits (Sleep(0), pumping
// the frame watchdog via FUN_00549960 when stalled too long) until the page is loaded, running
// the per-bitmap-type D3D conversion and freeing the staging buffer on first use. Returns the
// address of the cache entry's texture field, 0 if the page never became resident and wait is
// not set, or the FUN_00515c30 fallback's result if wait is set and nothing else is available.
void *texture_cache_get(BitmapData *bitmap, uint8_t wait, uint8_t allocate_if_missing)
{
    texture_cache_entry *entry;
    void *result;
    large_integer counter;
    int32_t elapsed_ms;
    int16_t bitmap_type;

    if ((bitmap->flags & 0x80) != 0) { // make_it_actually_work
        if (bitmap->pointer == 0xffffffff && allocate_if_missing != 0) {
            texture_cache_page_allocate(bitmap, wait);
        }

        if (bitmap->pointer != 0xffffffff) {
            entry = (texture_cache_entry *)((uint8_t *)texture_cache_entries->data +
                (bitmap->pointer & 0xffff) * sizeof(texture_cache_entry));

            // Touch the generic cache container's LRU bookkeeping entry for this datum directly
            // (cache_entry::age, types/memory.h), marking it as used at the cache's current age.
            ((cache_entry *)((uint8_t *)texture_cache->entries->data +
                (bitmap->pointer & 0xffff) * sizeof(cache_entry)))->age = texture_cache->age;

            if (wait != 0 && entry->loaded == 0) {
                if (debug_texture_cache_prints != 0) {
                    // movsx edx,WORD PTR [esi+0x20] at 0x004445d2: the tag index is scaled as
                    // a signed short, so the cast is kept rather than TagID's uint16_t.
                    console_print_va("%s",
                        tag_instances[(int16_t)bitmap->bitmap_tag_id.index].path);
                }
                cache_io_requests[entry->io_request_index].priority = 1;
            }

            for (;;) {
                if (entry->loaded != 0) {
                    if (entry->converted == 0) {
                        entry->converted = 1;
                        rasterizer_bitmap_create_hardware_texture(bitmap); // 0x444622: ESI = bitmap
                        bitmap_type = bitmap->type;
                        if (bitmap_type == 0) {
                            rasterizer_bitmap_upload_2d_mipmaps(bitmap);
                        } else if (bitmap_type == 1) {
                            rasterizer_bitmap_upload_cubemap_mipmaps(bitmap);
                        } else if (bitmap_type == 2) {
                            rasterizer_bitmap_upload_cubemap_mipmaps_by_face(bitmap);
                        }
                        if (*(void **)bitmap->_pad_2c != (void *)0) {
                            GlobalFree(*(void **)bitmap->_pad_2c);
                            *(void **)bitmap->_pad_2c = (void *)0;
                        }
                        entry->texture = *(void **)&bitmap->hardware_texture;
                    }
                    result = &entry->texture;
                    break;
                }

                QueryPerformanceCounter((LARGE_INTEGER *)&counter);
                elapsed_ms = (int32_t)((counter.quad_part * 1000) / performance_frequency);
                if (0x84 < (uint32_t)(elapsed_ms - frame_watchdog_time)) {
                    sound_idle_update();
                }
                if (wait == 0) {
                    return (void *)0;
                }
                Sleep(0);
            }
        } else {
            result = (void *)0;
        }
    } else {
        result = *(void **)&bitmap->hardware_texture;
    }

    if (wait != 0 && result == (void *)0) {
        // 0x4446e9: the per-type default texture. ECX is left over from earlier code at that call and is only
        // returned for a type above 3, which no valid bitmap has, so the fallback is 0
        result = rasterizer_get_capture_surface((uint8_t *)bitmap, (void *)0);
        return result;
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x444550):

int FUN_00444550(undefined4 param_1,char param_2)

{
  short sVar1;
  int in_EAX;
  uint uVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  undefined8 uVar6;
  LARGE_INTEGER local_8;

  iVar4 = 0;
  if (*(char *)(in_EAX + 0xe) < '\0') {
    if ((*(int *)(in_EAX + 0x24) == -1) && (param_2 != '\0')) {
      FUN_00444800(param_1);
    }
    if (*(uint *)(in_EAX + 0x24) != 0xffffffff) {
      uVar2 = *(uint *)(in_EAX + 0x24) & 0xffff;
      iVar5 = uVar2 * 0x10 + *(int *)(DAT_006ac538 + 0x34);
      *(undefined4 *)(*(int *)(*(int *)(DAT_006ac540 + 0x3c) + 0x34) + 0x14 + uVar2 * 0x1c) =
           *(undefined4 *)(DAT_006ac540 + 0x30);
      if (((char)param_1 != '\0') && (*(char *)(iVar5 + 4) == '\0')) {
        if (DAT_006f17f6 != '\0') {
          console_print_va("%s",*(undefined4 *)
                                 (*(short *)(in_EAX + 0x20) * 0x20 + 0x10 + DAT_0087bc14));
        }
        *(undefined1 *)(*(short *)(iVar5 + 2) * 0x30 + 0x1c + DAT_006ac4a0) = 1;
      }
      iVar4 = 0;
      do {
        if (*(char *)(iVar5 + 4) == '\0') {
          QueryPerformanceCounter(&local_8);
          uVar6 = __allmul(local_8.s.LowPart,local_8.s.HighPart,1000,0);
          iVar3 = __alldiv(uVar6,DAT_006ac8f8,DAT_006ac8fc);
          if (0x84 < (uint)(iVar3 - DAT_0072520c)) {
            FUN_00549960();
          }
          if ((char)param_1 == '\0') {
            return iVar4;
          }
          Sleep(0);
        }
        else {
          if (*(char *)(iVar5 + 5) == '\0') {
            *(undefined1 *)(iVar5 + 5) = 1;
            FUN_00523fa0();
            sVar1 = *(short *)(in_EAX + 10);
            if (sVar1 == 0) {
              FUN_00524100();
            }
            else if (sVar1 == 1) {
              FUN_00524270();
            }
            else if (sVar1 == 2) {
              FUN_005243c0();
            }
            if (*(HGLOBAL *)(in_EAX + 0x2c) != (HGLOBAL)0x0) {
              GlobalFree(*(HGLOBAL *)(in_EAX + 0x2c));
              *(undefined4 *)(in_EAX + 0x2c) = 0;
            }
            *(undefined4 *)(iVar5 + 0xc) = *(undefined4 *)(in_EAX + 0x28);
          }
          iVar4 = iVar5 + 0xc;
          if (iVar4 != 0) break;
        }
        if ((char)param_1 == '\0') {
          return iVar4;
        }
      } while( true );
    }
  }
  else {
    iVar4 = *(int *)(in_EAX + 0x28);
  }
  if (((char)param_1 != '\0') && (iVar4 == 0)) {
    iVar4 = FUN_00515c30();
    return iVar4;
  }
  return iVar4;
}
#endif
