// render_objects_collect  (Ghidra: FUN_0050eac0; new name)
// address 0x50eac0, size 171 bytes
// name confidence: 0.55   rewrite confidence: 0.8
// evidence: objdump -d -M intel 0x50eac0..0x50eb6a. The only work is two calls into
//   structure_bsp_collect_visible_objects 0x554420 that fill rendered_objects (0x006b8dc4,
//   k_maximum_rendered_objects entries) and set rendered_object_count (0x006b8dc0), the list
//   render_objects 0x50e930 and render_object_shadows 0x50eb70 walk. The first call walks the
//   collideable cluster reference lists (0x008603d0 / 0x008603d4, iterator 0x4f5f00 / 0x4f5f40),
//   the second the noncollideable ones (0x008603c0 / 0x008603c4, iterator 0x4f5e90 / 0x4f5ed0);
//   both share the bounds callback 0x50e8d0 (render_object_get_cull_sphere), the "not yet
//   visited" predicate 0x4f96f0 and the "mark visited" callback 0x4f9720, which compare and store
//   object.cluster_stamp (+0x14) against object_cluster_stamp (0x008603cc). The stamp is bumped
//   first so an object linked into several visible clusters is collected once. Ghidra's argument
//   order is right (push order at 0x50eacb..0x50eaea: out, 0x100, begin, next, bounds,
//   predicate, accept, confirmed against the [esp+N] slots 0x554420 calls through); only the
//   callback names it shows are wrong.
//   CEA has no symbol for this function; the name follows the rendered_objects global it fills.
// register convention: none (void, no arguments).
//   // blam-cc: void
// UNSURE: the names of the five callbacks (0x4f5f00 is still named
//   object_resolve_collideable_reference in functions.txt and 0x4f96f0 object_disconnect_from_map;
//   both names are wrong for what the bodies do, see the README of this module).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "fn_objects.h"

extern int32_t object_cluster_stamp;           // 0x008603cc, objects module
extern object_globals *object_globals_pointer; // 0x006b8cbc, objects module
extern int16_t rendered_object_count;          // 0x006b8dc0, this module
extern datum_index rendered_objects[0x100];    // 0x006b8dc4, this module
extern uint8_t rendered_objects_full_warning;  // 0x0071cfbe, this module

// structures module (src/structures/structure_bsp_collect_visible_objects.c); cdecl, 7 stack
// arguments. The callback slots are declared as void* here because the structures file types
// the bounds callback with a different (wrong) argument order; see the README.
extern int16_t structure_bsp_collect_visible_objects(datum_index *out_handles, int16_t max_count,
    void *iterate_begin, void *iterate_next, void *get_bounds, void *predicate, void *accept);
    // 0x554420

// cluster reference iterators and the visit stamp callbacks, objects module (cdecl)
extern datum_index object_cluster_collideable_iterate_begin(uint32_t *cursor, int16_t cluster_index);
    // 0x4f5f00 (functions.txt: object_resolve_collideable_reference)
extern datum_index object_cluster_collideable_iterate_next(uint32_t *cursor);       // 0x4f5f40
extern datum_index object_cluster_noncollideable_iterate_begin(uint32_t *cursor, int16_t cluster_index);
    // 0x4f5e90
extern datum_index object_cluster_noncollideable_iterate_next(uint32_t *cursor);    // 0x4f5ed0

    // 0x4f96f0 (functions.txt: object_disconnect_from_map)


extern void render_object_get_cull_sphere(datum_index object_index, real_point3d *center,
                                          float *radius); // 0x50e8d0, this module (cdecl)

// Rebuilds rendered_objects: every object in a visible cluster whose render bounding sphere
// passes the frustum test, collideable objects first, capped at 0x100 entries. Latches
// rendered_objects_full_warning the first time the list fills up.
void render_objects_collect(void)
{
    int16_t count;

    object_cluster_stamp++;
    object_globals_pointer->collecting_in_clusters = 1;

    count = structure_bsp_collect_visible_objects(rendered_objects, 0x100,
        (void *)object_cluster_collideable_iterate_begin,
        (void *)object_cluster_collideable_iterate_next,
        (void *)render_object_get_cull_sphere,
        (void *)object_cluster_stamp_not_visited,
        (void *)object_cluster_stamp_mark_visited);
    rendered_object_count = count;

    // The second limit is computed from the full dword at 0x006b8dc0 (mov edx,[0x6b8dc0];
    // sub 0x100,edx); the high half (0x006b8dc2) is never written, so this is 0x100 - count.
    rendered_object_count += structure_bsp_collect_visible_objects(&rendered_objects[count],
        (int16_t)(0x100 - rendered_object_count),
        (void *)object_cluster_noncollideable_iterate_begin,
        (void *)object_cluster_noncollideable_iterate_next,
        (void *)render_object_get_cull_sphere,
        (void *)object_cluster_stamp_not_visited,
        (void *)object_cluster_stamp_mark_visited);

    object_globals_pointer->collecting_in_clusters = 0;

    if (rendered_object_count == 0x100 && !rendered_objects_full_warning) {
        rendered_objects_full_warning = 1;
    }
}

#if 0
Original Ghidra decompilation (0x50eac0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0050eac0(void)

{
  short sVar1;

  DAT_008603cc = DAT_008603cc + 1;
  *(undefined1 *)(DAT_006b8cbc + 1) = 1;
  sVar1 = FUN_00554420(&DAT_006b8dc4,0x100,object_resolve_collideable_reference,&LAB_004f5f40,
                       &LAB_0050e8d0,object_disconnect_from_map,&LAB_004f9720);
  _DAT_006b8dc0 = CONCAT22(DAT_006b8dc0_2,sVar1);
  sVar1 = FUN_00554420(&DAT_006b8dc4 + sVar1,0x100 - _DAT_006b8dc0,&LAB_004f5e90,&LAB_004f5ed0,
                       &LAB_0050e8d0,object_disconnect_from_map,&LAB_004f9720);
  DAT_006b8dc0 = DAT_006b8dc0 + sVar1;
  *(undefined1 *)(DAT_006b8cbc + 1) = 0;
  if ((DAT_006b8dc0 == 0x100) && (DAT_0071cfbe == '\0')) {
    DAT_0071cfbe = '\x01';
  }
  return;
}
#endif
