// garbage_update  (Ghidra: missed_4bc510, created by hand this pass -- Ghidra never recovered it
// as a function; only reachable through the garbage object_type_definition row)
// address 0x4bc510, size 100 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: the garbage row (0x0069b8d8) carries this address at +0x34 (query_34), the same
//   column that holds item_update (0x4bc5c0) and weapon_update (0x4c1530) on their own rows;
//   both of those are also `int`-returning per-tick hooks. types/objects.h object.network_role
//   (0x004, "object_delete dispatches on 0 versus 3", already correctly documented there and
//   re-confirmed by src/objects/object_delete.c, this function's own close analog). Callees
//   object_delete_unparented (0x4f5aa0, EDI -> object_index) and object_delete_recursive
//   (0x4f59d0, cdecl (object_index, recurse_siblings)), both already established in
//   src/objects/object_delete.c.
// register convention: object index is a plain stack cdecl parameter; confirmed against objdump
//   -d -M intel bin/halo.exe (0x4bc510 mov edi,[esp+0xc], read after the two register-save
//   pushes, i.e. [esp+0x4] on entry) -- and that same EDI is what feeds
//   object_delete_unparented's own EDI convention at the 0x4bc55f call site.
// blam-cc: stack -> object_index
// UNSURE: the field this decrements, garbage_data + 0x00 (object + 0x22c), has no name in
//   types/items.h; it is garbage_new's (0x4bc490, this batch) random despawn countdown.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "items.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data; // 0x008603b0

extern void object_delete_unparented(uint32_t object_index);            // 0x4f5aa0, EDI -> object_index
extern void object_delete_recursive(uint32_t object_index, uint8_t recurse_siblings); // 0x4f59d0, cdecl

// The garbage row's "update" hook (object_type_definition +0x34). Counts garbage_data + 0x00's
// random despawn timer down by one tick; once it reaches zero, deletes the object the same way
// object_delete does (immediately if network_role == 0, recursively if it is 3, otherwise leaves
// it alone). Returns whether the countdown is still running (i.e. whether the object survives
// this tick), matching item_update/weapon_update's own int return convention for this vtable
// column.
int32_t garbage_update(uint32_t object_index) // blam-cc: stack -> object_index
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    int16_t *despawn_countdown = (int16_t *)((uint8_t *)obj + k_item_extension_offset); // garbage_data + 0x00, UNSURE field name
    int32_t still_alive;

    (*despawn_countdown)--;
    still_alive = *despawn_countdown > 0;

    if (!still_alive) {
        if (obj->network_role == 0) {
            object_delete_unparented(object_index);
        } else if (obj->network_role != 3) {
            return still_alive;
        }
        object_delete_recursive(object_index, 0);
    }
    return still_alive;
}

#if 0
Original Ghidra decompilation (0x4bc510):

bool missed_4bc510(uint param_1)

{
  short *psVar1;
  int iVar2;
  bool bVar3;
  int iVar4;
  int iVar5;

  iVar4 = DAT_008603b0;
  iVar5 = (param_1 & 0xffff) * 0xc;
  iVar2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar5);
  psVar1 = (short *)(iVar2 + 0x22c);
  *psVar1 = *psVar1 + -1;
  bVar3 = 0 < *(short *)(iVar2 + 0x22c);
  if (!bVar3) {
    iVar2 = *(int *)(*(int *)(*(int *)(iVar4 + 0x34) + 8 + iVar5) + 4);
    if (iVar2 == 0) {
      object_delete_unparented();
    }
    else if (iVar2 != 3) {
      return bVar3;
    }
    object_delete_recursive(param_1,0);
  }
  return bVar3;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
