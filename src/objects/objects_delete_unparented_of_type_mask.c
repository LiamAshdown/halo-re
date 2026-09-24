// objects_delete_unparented_of_type_mask
// address 0x4f47c0, size 149 bytes
// name confidence: 0.75 (still FUN_004f47c0 in Ghidra; types/objects.h's
//   _object_mask_scenery_and_light_fixture comment names this function explicitly: "0x240 --
//   objects_delete_unparented_of_type_mask and objects_update_player_visibility_masks both
//   iterate with 0x240")
// rewrite confidence: 0.6
// evidence: types/objects.h object_iterator (type_mask/flags_mask/index/handle),
//   object_header, object (render_cache_slot at 0xba, network_role at 0x004, whose header
//   comment already says "object_delete dispatches on 0 versus 3"); global 0x008603b0
//   object_data; callee object_iterator_next 0x4f6f20.
// register convention: none (void).
// evidence for the simplification below: the iterator returns an object pointer directly
//   (object_iterator_next), and the original re-derives the very same object through
//   object_data using the iterator's own "handle" field rather than reusing the pointer it just
//   got back; both name the identical object, so the rewrite reuses the pointer instead of
//   repeating the lookup.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern object *object_iterator_next(object_iterator *iterator); // 0x4f6f20
extern void object_delete_unparented(uint32_t object_index); // blam-cc: EDI -> object_index // 0x4f5aa0, this batch; UNSURE: called with no
                                            //   visible arguments in the original
extern void object_delete_recursive(uint32_t object_index, uint8_t recurse_siblings); // 0x4f59d0, this batch

void objects_delete_unparented_of_type_mask(void)
{
    object_iterator iterator;
    object *obj;
    int32_t role;

    iterator.type_mask = _object_mask_scenery_and_light_fixture;
    iterator.flags_mask = 0;
    iterator.index = 0;
    iterator.handle = k_datum_index_none;

    obj = object_iterator_next(&iterator);
    while (obj != 0) {
        if (obj->render_cache_slot == -1) {
            role = obj->network_role;
            if (role == 0) {
                object_delete_unparented(iterator.handle);
            }
            if (role == 0 || role == 3) {
                object_delete_recursive(iterator.handle, 0);
            }
        }
        obj = object_iterator_next(&iterator);
    }
}

#if 0
Original Ghidra decompilation (0x4f47c0):

void FUN_004f47c0(void)

{
  uint uVar1;
  int iVar2;
  undefined4 local_10;
  undefined1 local_c;
  undefined2 local_a;
  uint local_8;
  undefined4 local_4;

  local_4 = 0x86868686;
  local_10 = 0x240;
  local_c = 0;
  local_a = 0;
  local_8 = 0xffffffff;
  iVar2 = object_iterator_next(&local_10);
  uVar1 = local_8;
  do {
    if (iVar2 == 0) {
      return;
    }
    local_8 = uVar1;
    if (*(short *)(iVar2 + 0xba) == -1) {
      iVar2 = *(int *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar1 & 0xffff) * 0xc) + 4);
      if (iVar2 == 0) {
        FUN_004f5aa0();
      }
      else if (iVar2 != 3) goto LAB_004f4845;
      FUN_004f59d0(uVar1,0);
    }
LAB_004f4845:
    iVar2 = object_iterator_next(&local_10);
    uVar1 = local_8;
  } while( true );
}
#endif
