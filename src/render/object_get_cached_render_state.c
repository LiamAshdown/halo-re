// object_get_cached_render_state  (Ghidra: shadow_cache_get_or_allocate_entry; renamed per
// out/phase4/render_types_notes.md's misattributed-functions table: "The object render state
// cache, not a shadow cache. Probable names: object_get_cached_render_state,
// object_render_state_refresh, object_get_cached_render_lighting")
// address 0x50f150, size 279 bytes
// name confidence: 0.5   rewrite confidence: 0.8
// evidence: types/render.h cached_object_render_state's own doc: "The eviction in 0x50f150 picks
//   the entry with the oldest last_update_window"; object.cached_render_state_index documented there as "the
//   datum index of the cached_object_render_state of the object (0x50f150 reads and writes it)".
//   Disassembly (objdump -d -M intel, 0x50f150..0x50f17a) confirms both arguments are plain stack
//   parameters (object_index at the caller's first stack slot, forwarded level_of_detail_pixels
//   at the second), matching Ghidra's own clean recognition of them.
// review fix (phase-4 gate): the eviction age is render_window_count (0x007c3104) minus
//   last_update_window, not render_frame_index minus it.
// register convention: stack = (object_index, level_of_detail_pixels).
//   // blam-cc: stack=(object_index, level_of_detail_pixels)
// reconciled: R30 object.unknown_170 -> datum_index cached_render_state_index (same offset 0x170)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "rasterizer.h"
#include "render.h"
#include "fn_memory.h"

extern data_array *object_data;                    // 0x008603b0
extern data_array *object_render_state_cache;       // 0x007c30ec, this module
extern int32_t render_window_count;  // 0x007c3104, this module
extern int32_t render_frame_index;                    // 0x007c3100, this module


extern datum_index datum_next(int16_t after_index, data_array *array); // 0x4d0630, memory module
extern void object_render_state_refresh(datum_index cache_index, datum_index object_index,
                                        real level_of_detail_pixels, uint8_t full_sample);
    // 0x50f270, this module

// Finds the cached render-state entry already associated with object_index (validating it still
// belongs to that object), or allocates a new one, evicting the entry with the oldest
// last_update_window when the cache is full. Either way refreshes the entry (a cheap step for a
// reused entry, a full sample for a newly allocated or evicted one) and stamps the owning object's
// cache-entry index before returning it.
datum_index object_get_cached_render_state(datum_index object_index,
                                           real level_of_detail_pixels) // blam-cc: stack=(object_index, level_of_detail_pixels)
{
    object_header *header = &((object_header *)object_data->data)[(uint16_t)object_index];
    object *obj = header->data;
    datum_index cache_index = obj->cached_render_state_index;

    if (cache_index != k_datum_index_none &&
        ((cached_object_render_state *)object_render_state_cache->data)[(uint16_t)cache_index].object_index ==
            object_index) {
        object_render_state_refresh(cache_index, object_index, level_of_detail_pixels, 0);
        return cache_index;
    }

    cache_index = datum_new(object_render_state_cache);
    if (cache_index == k_datum_index_none) {
        float oldest_age = -3.4028235e+38f; // -FLT_MAX
        datum_index candidate = datum_next(-1, object_render_state_cache);
        int32_t current_window = render_window_count; // mov ebp,[0x7c3104] at 0x50f1c4

        while (candidate != k_datum_index_none) {
            cached_object_render_state *entry =
                &((cached_object_render_state *)object_render_state_cache->data)[(uint16_t)candidate];
            float age = (float)(current_window - entry->last_update_window);
            if (age < 0.0f) {
                age = 1000.0f;
            }
            if (oldest_age < age) {
                cache_index = candidate;
                oldest_age = age;
            }
            candidate = datum_next((int16_t)candidate, object_render_state_cache);
        }
        if (cache_index == k_datum_index_none) {
            return k_datum_index_none;
        }
    }

    object_render_state_refresh(cache_index, object_index, level_of_detail_pixels, 1);
    obj->cached_render_state_index = cache_index;
    return cache_index;
}

#if 0
Original Ghidra decompilation (0x50f150):

uint shadow_cache_get_or_allocate_entry(uint param_1,undefined4 param_2)

{
  int iVar1;
  int iVar2;
  float fVar3;
  int iVar4;
  uint uVar5;
  uint uVar6;
  float local_c;

  iVar2 = DAT_007c30ec;
  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  uVar5 = *(uint *)(iVar1 + 0x170);
  if ((uVar5 != 0xffffffff) &&
     (*(uint *)((uVar5 & 0xffff) * 0x100 + 4 + *(int *)(DAT_007c30ec + 0x34)) == param_1)) {
    FUN_0050f270(uVar5,param_1,param_2,0);
    return uVar5;
  }
  uVar5 = datum_new();
  if (uVar5 == 0xffffffff) {
    local_c = -3.4028235e+38;
    uVar6 = datum_next();
    iVar4 = DAT_007c3104;
    if (uVar6 != 0xffffffff) {
      iVar2 = *(int *)(iVar2 + 0x34);
      do {
        fVar3 = (float)(iVar4 - *(int *)((uVar6 & 0xffff) * 0x100 + 0xc + iVar2));
        if (fVar3 < 0.0) {
          fVar3 = 1000.0;
        }
        if (local_c < fVar3) {
          uVar5 = uVar6;
          local_c = fVar3;
        }
        uVar6 = datum_next();
      } while (uVar6 != 0xffffffff);
    }
    if (uVar5 == 0xffffffff) {
      return 0xffffffff;
    }
  }
  FUN_0050f270(uVar5,param_1,param_2,1);
  *(uint *)(iVar1 + 0x170) = uVar5;
  return uVar5;
}
#endif
