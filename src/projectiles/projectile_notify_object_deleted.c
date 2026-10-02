// projectile_notify_object_deleted  (Ghidra: missed_4bf0c0, created by hand this pass -- Ghidra
// never recovered it as a function; only reachable through the projectile object_type_definition
// row)
// address 0x4bf0c0, size 48 bytes
// name confidence: 0.7   rewrite confidence: 0.85
// evidence: named already in out/phase4/projectiles_types_notes.md ("+0x3c
//   projectile_notify_object_deleted -- missed" and "tracked_object_index ... 0x238 ...
//   projectile_notify_object_deleted (0x4bf0c0) clears it when that object dies"). The
//   projectile row (0x0069b9a0) carries this address at +0x3c, the generic notify_3c column
//   whose broadcaster is src/objects/object_type_definitions_notify_0x3c.c, called (with both
//   arguments) from src/objects/object_clear_references_to_object.c -- the exact "an object just
//   died; scrub every reference to it" sweep. types/projectiles.h projectile_data
//   .tracked_object_index (0x238).
// register convention: both arguments are Ghidra-recognized stack parameters; confirmed against
//   objdump -d -M intel bin/halo.exe (0x4bf0c0 mov eax,[esp+0x4] / cmp ecx,[esp+0x8]) -- pure
//   cdecl, matching object_clear_references_to_object's own call
//   `object_type_definitions_notify_0x3c(iterator.handle, dying_object_index)`.
// blam-cc: stack -> (object_index, dying_object_index)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "projectiles.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data; // 0x008603b0

// The projectile row's notify_3c hook (object_type_definition +0x3c), run for every object when
// some other object dies (object_clear_references_to_object's sweep). If this projectile was
// tracking the object that just died, forgets it.
void projectile_notify_object_deleted(uint32_t object_index, datum_index dying_object_index)
    // blam-cc: stack -> (object_index, dying_object_index)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    projectile_data *proj = (projectile_data *)((uint8_t *)obj + k_projectile_data_offset);

    if (proj->tracked_object_index == dying_object_index) {
        proj->tracked_object_index = (datum_index)k_datum_index_none;
    }
}

#if 0
Original Ghidra decompilation (0x4bf0c0):

void missed_4bf0c0(uint param_1,int param_2)

{
  int iVar1;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  if (*(int *)(iVar1 + 0x238) == param_2) {
    *(undefined4 *)(iVar1 + 0x238) = 0xffffffff;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
