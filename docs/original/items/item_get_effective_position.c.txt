// item_get_effective_position  (Ghidra: item_get_effective_position, already named)
// address 0x4bd740, size 122 bytes
// name confidence: 0.5   rewrite confidence: 0.55
// evidence: types/items.h item_flags._item_in_inventory_bit (0x01); types/objects.h object
//   (bounding_center 0x0a0, owner_linkage 0x0c0); global 0x008603b0 object_data, 0x0087a480
//   player_data (data_array, stride 0x200); callees object_try_and_get, datum_get.
// register convention: object index in ECX (unaff_ECX, matching object_try_and_get's usual
//   hidden slot -- confirmed by disassembly, objdump -d -M intel bin/halo.exe, 0x4bd740..:
//   `push 0x1c; call 0x4f6ec0` passes only the mask, so the index is already live in ECX at
//   entry); output real_point3d * in EDI (unaff_EDI).
//   // blam-cc: ECX -> object_index, EDI -> out_position
// UNSURE: the return value is Ghidra's CONCAT31(garbage, AL) shape; narrowed to a plain bool
//   here, matching src/objects/object_disconnect_from_map.c's precedent.
// TYPES-GAP: the player record type is not defined anywhere in types/ yet (the players module
//   has not been written). Only one field is needed here, at player+0x34 -- read directly by
//   raw offset with a local comment rather than inventing a `player` struct.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "items.h"
#include "game.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data; // 0x008603b0
extern data_array *player_data; // 0x0087a480, players module, stride 0x200 (types/units.h)

extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern void *datum_get(datum_index handle, data_array *array); // 0x4d0680, handle in EDX, array in ESI

// Returns an item's world position, following the attachment chain to the controlling
// player's current object when the item itself is held (item_flags bit 0x01) rather than
// resting in the world. Returns false and zeroes *out_position when no position could be
// resolved.
uint8_t item_get_effective_position(datum_index object_index, real_point3d *out_position)
    // blam-cc: ECX -> object_index, EDI -> out_position
{
    object *obj = object_try_and_get(object_index, _object_mask_item);

    out_position->x = 0.0f;
    out_position->y = 0.0f;
    out_position->z = 0.0f;

    if (obj == 0) {
        return 0;
    }

    if ((((item_data *)((uint8_t *)obj + k_item_data_offset))->flags & _item_in_inventory_bit) != 0) {
        uint32_t owner_linkage = obj->owner_linkage;
        if (owner_linkage == (uint32_t)k_datum_index_none) {
            return 0;
        }
        {
            uint8_t *player = (uint8_t *)datum_get(owner_linkage, player_data);
            if (player == 0) {
                return 0;
            }
            {
                // the player's unit (struct player: the local is named player too)
                uint32_t controlled_object_index = (uint32_t)((struct player *)player)->unit;
                if (controlled_object_index == (uint32_t)k_datum_index_none) {
                    return 0;
                }
                obj = ((object_header *)object_data->data)[controlled_object_index & 0xffff].data;
            }
        }
    }

    *out_position = obj->bounding_center;
    return 1;
}

#if 0
Original Ghidra decompilation (0x4bd740):

uint item_get_effective_position(void)

{
  undefined4 uVar1;
  int iVar2;
  uint uVar3;
  undefined4 *unaff_EDI;

  iVar2 = object_try_and_get(0x1c);
  *unaff_EDI = 0;
  unaff_EDI[1] = 0;
  unaff_EDI[2] = 0;
  uVar3 = 0;
  if (iVar2 != 0) {
    if ((*(byte *)(iVar2 + 500) & 1) == 0) {
LAB_004bd79d:
      *unaff_EDI = *(undefined4 *)(iVar2 + 0xa0);
      unaff_EDI[1] = *(undefined4 *)(iVar2 + 0xa4);
      uVar1 = *(undefined4 *)(iVar2 + 0xa8);
      unaff_EDI[2] = uVar1;
      return CONCAT31((int3)((uint)uVar1 >> 8),1);
    }
    uVar3 = *(uint *)(iVar2 + 0xc0);
    if (uVar3 != 0xffffffff) {
      iVar2 = datum_get();
      uVar3 = 0;
      if ((iVar2 != 0) && (uVar3 = *(uint *)(iVar2 + 0x34), uVar3 != 0xffffffff)) {
        iVar2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar3 & 0xffff) * 0xc);
        goto LAB_004bd79d;
      }
    }
  }
  return uVar3 & 0xffffff00;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
