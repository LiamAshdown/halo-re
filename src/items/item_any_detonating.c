// item_any_detonating  (Ghidra: FUN_004bcf50; renamed per types/items.h: "item_any_detonating
// (0x4bcf50) iterates _object_mask_item looking for > 0")
// address 0x4bcf50, size 95 bytes
// name confidence: 0.55   rewrite confidence: 0.6
// evidence: types/items.h item_data.detonation_countdown (0x1f8); types/objects.h
//   object_iterator (type_mask 0x00, flags_mask 0x04, index 0x06, handle 0x08),
//   _object_mask_item (0x1c), _object_header_active_bit (0x01); callee object_iterator_next.
// register convention: none (void).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "items.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern object *object_iterator_next(object_iterator *iterator); // 0x4f6f20

// Reports whether any live item object currently has a positive detonation_countdown.
uint32_t item_any_detonating(void)
{
    object_iterator iterator;
    object *obj;

    iterator.type_mask = _object_mask_item;
    iterator.flags_mask = _object_header_active_bit;
    iterator.index = 0;
    iterator.handle = (datum_index)0xffffffff;

    obj = object_iterator_next(&iterator);
    while (obj != 0) {
        item_data *item = (item_data *)((uint8_t *)obj + k_item_data_offset);
        if (item->detonation_countdown > 0) {
            return 1;
        }
        obj = object_iterator_next(&iterator);
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4bcf50):

undefined4 FUN_004bcf50(void)

{
  int iVar1;
  undefined4 local_10;
  undefined1 local_c;
  undefined2 local_a;
  undefined4 local_8;
  undefined4 local_4;

  local_4 = 0x86868686;
  local_10 = 0x1c;
  local_c = 1;
  local_a = 0;
  local_8 = 0xffffffff;
  iVar1 = object_iterator_next(&local_10);
  while( true ) {
    if (iVar1 == 0) {
      return 0;
    }
    if (0 < *(short *)(iVar1 + 0x1f8)) break;
    iVar1 = object_iterator_next(&local_10);
  }
  return 1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
