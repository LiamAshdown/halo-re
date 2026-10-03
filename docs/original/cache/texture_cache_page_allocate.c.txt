// texture_cache_page_allocate  (Ghidra: FUN_00444800; renamed, named directly in
// out/phase4/cache_types_notes.md's cache_io_completion section and in types/cache.h's
// texture_cache_entry section: "texture_cache_page_allocate @0x444800 writes +0x02, +0x08 and
// +0x0c" / "does the same at 0x00444879 with {&entry->loaded, 0, 0}")
// address 0x444800, size 194 bytes
// name confidence: 0.75  rewrite confidence: 0.80
// evidence: types/cache.h texture_cache_entry field offsets (io_request_index 0x02, loaded
// 0x04, bitmap 0x08, texture 0x0c); BitmapData field offsets in types/tags.h (pixel_data_offset
// 0x18, pixel_data_size 0x1c, pointer 0x24, flags 0x0e/0x0f); confirmed against raw
// disassembly (objdump -d -M intel --start-address=0x444800 --stop-address=0x4448d0
// bin/halo.exe), which shows `mov esi,eax` at entry (bitmap in EAX) and the priority argument
// arriving on the stack, exactly matching sound_cache_page_allocate's own header note and its
// already-rewritten sibling call to cache_io_request_new.
// register convention: BitmapData *bitmap in EAX (in_EAX); priority is the recognized stack
// parameter (param_1). The cache_io_completion record {&entry->loaded, 0, 0} is built on
// this function's stack and passed to cache_io_request_new in ESI (`lea esi,[esp+0x28]` at
// 0x0044489f, the three dwords written at 0x00444879/0x0044487d/0x00444881), the same ESI
// convention cache_io_request_new.c documents.
//
// UNSURE: bitmap_compute_texture_data_size (0x5146c0) is outside this module; its
// register convention (bitmap in EAX) is confirmed by disassembly (no argument setup between
// `mov esi,eax` and `call 0x5146c0`) but its body is not rewritten here.

// phase-4 review pass: body re-checked instruction by instruction against `objdump -d -M
// intel` of this address range; every field offset, branch and argument below now matches
// the machine code rather than only Ghidra's pseudo-C.
#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "cache.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern struct cache *texture_cache;        // 0x006ac540
extern data_array *texture_cache_entries;  // 0x006ac538

extern datum_index cache_allocate_block(struct cache *self, uint32_t requested_bytes); // 0x4d1840
extern datum_index datum_new_at_index_with_salt(datum_index requested_handle, data_array *array); // 0x4d03d0

// blam-cc: bitmap in EAX; outside this module, UNSURE (see file header note)
extern uint32_t bitmap_compute_texture_data_size(BitmapData *bitmap); // 0x5146c0

extern int16_t cache_io_request_new(cache_io_completion *completion, // blam-cc: ESI
    int32_t offset, uint32_t size, void *destination, uint8_t priority,
    uint8_t data_file_index); // 0x442b20, this module

// blam-cc: bitmap in EAX (in_EAX), priority as the recognized stack parameter
// Allocates a texture-cache page (a 4-block/4-byte slot handle, per k_texture_cache_block_shift
// -- the cache rations slots, not pixel storage) for a bitmap and issues an async read to fill
// a freshly GlobalAlloc'd staging buffer with its pixel data. The staging buffer is sized to
// the larger of the bitmap's own pixel_data_size and the driver-computed texture data size.
// Carries the bitmap's existing D3D texture pointer (BitmapData::_pad_28) over into the new
// entry, and reads bitmap flag bit 8 (external) to pick bitmaps.map vs the resident map file as
// the read's source. Returns 1 on success, 0 if the texture cache has no free slot.
uint32_t texture_cache_page_allocate(BitmapData *bitmap, uint8_t priority)
{
    uint32_t computed_size;
    datum_index cache_slot;
    uint32_t alloc_size;
    void *staging_buffer;
    texture_cache_entry *entry;
    uint8_t data_file_index;
    cache_io_completion completion;

    computed_size = bitmap_compute_texture_data_size(bitmap);
    cache_slot = cache_allocate_block(texture_cache, 4);
    if (cache_slot == (datum_index)0xffffffff) {
        return 0;
    }

    alloc_size = bitmap->pixel_data_size;
    if ((int32_t)alloc_size < (int32_t)computed_size) {
        alloc_size = computed_size;
    }
    staging_buffer = GlobalAlloc(0, alloc_size);

    datum_new_at_index_with_salt(cache_slot, texture_cache_entries);
    entry = (texture_cache_entry *)((uint8_t *)texture_cache_entries->data +
        (cache_slot & 0xffff) * sizeof(texture_cache_entry));

    bitmap->pointer = cache_slot;
    bitmap->pixel_base = staging_buffer;
    entry->bitmap = bitmap;
    entry->texture = *(void **)&bitmap->hardware_texture;

    data_file_index = (bitmap->flags & 0x100) ? (uint8_t)_cache_io_data_file_bitmaps
                                               : (uint8_t)_cache_io_data_file_cache;
    completion.flag = &entry->loaded;
    completion.procedure = 0;
    completion.data = 0;
    entry->io_request_index = cache_io_request_new(&completion, bitmap->pixel_data_offset,
        bitmap->pixel_data_size, staging_buffer, priority, data_file_index);

    return 1;
}

#if 0
Original Ghidra decompilation (0x444800):

undefined4 FUN_00444800(undefined4 param_1)

{
  undefined2 uVar1;
  int in_EAX;
  SIZE_T SVar2;
  uint uVar3;
  SIZE_T dwBytes;
  HGLOBAL pvVar4;
  int extraout_EDX;
  int iVar5;

  SVar2 = bitmap_compute_texture_data_size();
  uVar3 = FUN_004d1840(DAT_006ac540,4);
  if (uVar3 != 0xffffffff) {
    dwBytes = *(SIZE_T *)(in_EAX + 0x1c);
    if ((int)*(SIZE_T *)(in_EAX + 0x1c) < (int)SVar2) {
      dwBytes = SVar2;
    }
    pvVar4 = GlobalAlloc(0,dwBytes);
    datum_new_at_index_with_salt();
    iVar5 = (uVar3 & 0xffff) * 0x10 + *(int *)(extraout_EDX + 0x34);
    *(uint *)(in_EAX + 0x24) = uVar3;
    *(HGLOBAL *)(in_EAX + 0x2c) = pvVar4;
    *(int *)(iVar5 + 8) = in_EAX;
    *(undefined4 *)(iVar5 + 0xc) = *(undefined4 *)(in_EAX + 0x28);
    uVar1 = cache_io_request_new
                      (*(undefined4 *)(in_EAX + 0x18),*(undefined4 *)(in_EAX + 0x1c),pvVar4,param_1,
                       (*(byte *)(in_EAX + 0xf) & 1) != 0);
    *(undefined2 *)(iVar5 + 2) = uVar1;
    return 1;
  }
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
