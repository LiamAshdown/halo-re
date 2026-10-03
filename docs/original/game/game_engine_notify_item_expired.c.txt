// game_engine_notify_item_expired  (Ghidra: FUN_0045f510; named per
// out/phase4/game_functions.md)
// address 0x45f510, size 77 bytes
// name confidence: 0.4   rewrite confidence: 0.4
// evidence: out/phase4/game_functions.md ("Notifies the active game variant callback when an
// unclaimed item object is about to be despawned"); types/objects.h object (parent_object
// +0x11c); types/items.h item_data (flags +0x1f4, _item_in_inventory_bit).
// register convention: object handle in EDX (in_EDX).
//   // blam-cc: EDX -> object_index
// VERIFIED against disassembly 0x45f510..0x45f55c (2026-09-30); fixed: the +0x44 callback gets the object handle (push edx).
// UNSURE: bit 0x20 of the type-specific extension dword at object+0x22c (weapon_data.flags /
// equipment_data.unknown_22c / garbage_data.unknown_22c, whichever the object actually is) is
// documented nowhere in types/items.h; kept as a raw offset/bit test. game_engine_definition's
// +0x44 slot is named `object_expired` in types/game.h, matching this call exactly.

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
extern game_engine_definition *current_game_engine; // 0x006f1d20

// blam-cc: EDX -> object_index
// If `object_index` is a root (unparented) item that is not currently held and whose
// type-specific extension has the (unresolved) 0x20 bit set, clears that bit and, if the active
// game engine implements object_expired, notifies it.
void game_engine_notify_item_expired(datum_index object_index)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    item_data *item = (item_data *)((uint8_t *)obj + sizeof(object));
    uint32_t *extension_flags = (uint32_t *)((uint8_t *)obj + 0x22c);

    if (obj->parent_object == (datum_index)0xffffffff &&
        (item->flags & _item_in_inventory_bit) == 0 &&
        (*extension_flags & 0x20) != 0) {
        *extension_flags = *extension_flags & 0xffffffdf;
        if (current_game_engine != 0 && current_game_engine->object_expired != 0) {
            ((void (*)(datum_index))current_game_engine->object_expired)(object_index); // 0x45f558: push edx
        }
    }
}

#if 0
Original Ghidra decompilation (0x45f510), from tools/pack.py 0x45f510:

void FUN_0045f510(void)

{
  int iVar1;
  uint in_EDX;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EDX & 0xffff) * 0xc);
  if (((*(int *)(iVar1 + 0x11c) == -1) && ((*(byte *)(iVar1 + 500) & 1) == 0)) &&
     ((*(uint *)(iVar1 + 0x22c) & 0x20) != 0)) {
    *(uint *)(iVar1 + 0x22c) = *(uint *)(iVar1 + 0x22c) & 0xffffffdf;
    if (*(code **)(DAT_006f1d20 + 0x44) != (code *)0x0) {
      (**(code **)(DAT_006f1d20 + 0x44))();
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
