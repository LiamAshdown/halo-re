// cheat_all_weapons  (Ghidra: FUN_0045a530; RENAMED in the phase 4 review from
// cheat_spawn_all_object_tags -- both of its sources are weapon lists, never all object tags:
// Globals::weapon_list when that is populated, otherwise a tag_iterator filtered on the 'weap'
// group. "cheat_all_weapons" is also the retail debug command name for exactly this effect.)
// address 0x45a530, size 141 bytes
// name confidence: 0.4   rewrite confidence: 0.45
// evidence: symbols/review_queue.txt 0x45a530 "if a specific tag/count override is set in
//   DAT_00746fa0 it spawns that directly; otherwise collects up to 16 tags via tag_iterator_next
//   and passes them to cheat_spawn_objects_near_camera"; matches this batch's own
//   cheat_spawn_objects_near_camera (0x45a800) record layout (a tag handle every 0x10 bytes,
//   here at +0x0c per stack slot).
//
// CORRECTED (phase 4 review, objdump 0x45a56b..0x45a57d): tag_iterator_next takes a
// tag_iterator in ESI (see src/cache/tag_iterator_next.c and types/cache.h). This function
// builds one on its own stack -- `lea esi,[esp+8]`, `mov WORD [esi+4],0` (next_index) and
// `mov DWORD [esi+0x10],'weap'` (group_tag) -- and reloads ESI before every call. The iterator
// therefore only ever walks WEAPON tags, which the first pass' "all object tags" reading and
// the function's inherited name both miss.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern Globals *global_globals; // 0x00746fa0

extern datum_index tag_iterator_next(tag_iterator *iterator); // 0x4425d0, blam-cc: ESI ->
    // iterator (matches src/cache/tag_iterator_next.c)
extern void cheat_spawn_objects_near_camera(TagDependency *tag_array, int16_t count); // this batch, 0x45a800

// Spawns one of every weapon near the camera: the globals tag's own weapon_list when it has
// entries, otherwise the first 16 'weap' tags in the tag index.
void cheat_all_weapons(void)
{
    int16_t count;
    tag_iterator iterator;      // types/cache.h, 0x14 bytes at esp+0x08
    TagDependency slots[16]; // types/game.h, 0x10-byte scratch record at esp+0x1c
    datum_index tag;

    count = 0;
    iterator.next_index = 0;
    iterator.group_tag = (tag_group)0x77656170; // 'weap'
    // RESOLVED (phase 4 review): globals+0x14c/+0x150 is Globals::weapon_list (0xf8 plus
    // seven 0x0c-byte reflexives), an array of GlobalsWeapon -- one TagDependency each.
    if (global_globals->weapon_list.count != 0 && global_globals->weapon_list.pointer != 0) {
        cheat_spawn_objects_near_camera((TagDependency *)global_globals->weapon_list.pointer,
                                         (int16_t)global_globals->weapon_list.count);
                                         // objdump: movsx from a WORD, so the count is signed
        return;
    }

    tag = tag_iterator_next(&iterator);
    while (tag != k_datum_index_none && (uint16_t)count < 0x10) {
        *(datum_index *)&slots[count].tag_id = tag; // TagDependency::tag_id is a TagID pair
        count = count + 1;
        tag = tag_iterator_next(&iterator);
    }
    cheat_spawn_objects_near_camera(slots, count);
}

#if 0
Original Ghidra decompilation (0x45a530), from tools/pack.py 0x45a530:

void FUN_0045a530(void)

{
  int iVar1;
  ushort count;
  undefined1 local_100 [12];
  int aiStack_f4 [61];

  count = 0;
  if ((*(int *)(DAT_00746fa0 + 0x14c) != 0) && (*(int *)(DAT_00746fa0 + 0x150) != 0)) {
    cheat_spawn_objects_near_camera
              (*(int *)(DAT_00746fa0 + 0x150),*(ushort *)(DAT_00746fa0 + 0x14c));
    return;
  }
  iVar1 = tag_iterator_next();
  for (; (iVar1 != -1 && (count < 0x10)); count = count + 1) {
    aiStack_f4[(short)count * 4] = iVar1;
    iVar1 = tag_iterator_next();
  }
  cheat_spawn_objects_near_camera((int)local_100,count);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
