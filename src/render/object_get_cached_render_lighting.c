// object_get_cached_render_lighting  (Ghidra: render_get_cluster_ambient_light_sample; renamed
// per out/phase4/render_types_notes.md's misattributed-functions table: "The object render state
// cache, not a shadow cache. Probable names: object_get_cached_render_state,
// object_render_state_refresh, object_get_cached_render_lighting")
// address 0x50ea00, size 74 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: types/render.h object_render_data.lighting: "object_get_cached_render_lighting
//   0x50ea00; 0 when the object is not lit" and its render_uncached_object_lighting global note.
//   Disassembly (objdump -d -M intel, 0x50ea00..0x50ea49) confirms ESI is a genuine hidden
//   register argument (object_index, forwarded to object_sample_ambient_lighting /
//   object_gather_light_list in EAX) alongside one stack argument forwarded verbatim to
//   object_get_cached_render_state as its second parameter.
// register convention: ESI = object_index, stack = level_of_detail_pixels.
//   // blam-cc: ESI=object_index, stack=level_of_detail_pixels

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "render.h"

extern data_array *object_render_state_cache; // 0x007c30ec, this module
extern render_lighting render_uncached_object_lighting; // 0x006b91c8, this module

extern datum_index object_get_cached_render_state(datum_index object_index,
                                                   real level_of_detail_pixels); // 0x50f150, this
                                                                                // module
extern void object_sample_ambient_lighting(datum_index object_index, render_lighting *out); // 0x4f20b0,
                                            // foreign; blam-cc: EAX=object_index, stack=out
extern void object_gather_light_list(datum_index object_index, render_lighting *out); // 0x4f2430,
                                            // foreign; blam-cc: EAX=object_index, stack=out

// Returns a pointer to the (cached, or freshly sampled and uncached) ambient lighting values for
// an object: if the object render-state cache has (or can make) room, returns a pointer straight
// into that cached entry's lighting field; otherwise falls back to sampling directly into the
// shared render_uncached_object_lighting scratch global.
render_lighting *object_get_cached_render_lighting(datum_index object_index,
                                                   real level_of_detail_pixels) // blam-cc: ESI=object_index, stack=level_of_detail_pixels
{
    datum_index cache_index = object_get_cached_render_state(object_index, level_of_detail_pixels);

    if (cache_index != k_datum_index_none) {
        return &((cached_object_render_state *)object_render_state_cache->data)[(uint16_t)cache_index].lighting;
    }

    object_sample_ambient_lighting(object_index, &render_uncached_object_lighting);
    object_gather_light_list(object_index, &render_uncached_object_lighting);
    return &render_uncached_object_lighting;
}

#if 0
Original Ghidra decompilation (0x50ea00):

undefined * render_get_cluster_ambient_light_sample(void)

{
  uint uVar1;

  uVar1 = shadow_cache_get_or_allocate_entry();
  if (uVar1 != 0xffffffff) {
    return (undefined *)((uVar1 & 0xffff) * 0x100 + 0x14 + *(int *)(DAT_007c30ec + 0x34));
  }
  object_sample_ambient_lighting(&DAT_006b91c8);
  object_gather_light_list(&DAT_006b91c8);
  return &DAT_006b91c8;
}
#endif
