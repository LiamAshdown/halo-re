// equipment_new  (Ghidra: missed_4bba90, created by hand this pass -- Ghidra never recovered it
// as a function; only reachable through the equipment object_type_definition row)
// address 0x4bba90, size 68 bytes
// name confidence: 0.6   rewrite confidence: 0.75
// evidence: types/objects.h object_type_definition.query_create (+0x28, "AND style"), already
//   implemented generically by src/objects/object_type_definitions_query_0x28.c, which is
//   called from object_new_with_datum_role_control right after a placement is activated; the
//   equipment row (0x0069b810) carries this address at +0x28 (confirmed by reading the row out
//   of .data at file offset VA-0x400000). types/items.h equipment_data.network_state_valid
//   (0x244), .network_baseline_index (0x245), .network_sequence (0x246); global 0x00719720
//   network_game_mode (0 local, 1 client, 2 host).
// register convention: object index is a plain stack cdecl parameter. Confirmed against
//   objdump -d -M intel bin/halo.exe: 0x4bba90 mov eax,[esp+0x4], matching every other
//   directly-indexed (non object_try_and_get) function in this batch.
// blam-cc: stack -> object_index
// UNSURE: the byte cleared at object+0x9 (inside object.unknown_008, the dword at object+0x008)
//   has no name anywhere in types/objects.h; it is grouped here with the three equipment
//   network bytes because the binary clears all four together, exactly like projectile_new
//   clears object+0x9 alongside its own three network bytes (see out/phase4/projectiles_types_
//   notes.md, "types/objects.h: object 0x18/0x1c and 0x44/0x48 are the interpolation block").
// reconciled: R26 object +0x009 raw byte store -> network_state_009 (object.unknown_008 is now split into uint8 unknown_008 / network_state_009 / unknown_00a[2])

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "items.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data; // 0x008603b0
extern int16_t network_game_mode; // 0x00719720, 0 local, 1 client, 2 host

// The equipment row's query_create hook (object_type_definition +0x28), run once for a freshly
// activated equipment object. When the game is networked it clears the item's replicated-state
// bookkeeping (equipment_data's three network bytes, plus the still-unnamed object+0x9 byte) so
// a locally-created equipment item does not start out claiming a stale network baseline. Always
// reports success.
uint8_t equipment_new(uint32_t object_index) // blam-cc: stack -> object_index
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    equipment_data *ed = (equipment_data *)((uint8_t *)obj + k_item_extension_offset);

    if (network_game_mode == 1 || network_game_mode == 2) {
        ed->network_state_valid = 0;
        ed->network_baseline_index = 0;
        ed->network_sequence = 0;
        obj->network_state_009 = 0; // same store as projectile_new 0x4bda48
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4bba90):

undefined4 missed_4bba90(uint param_1)

{
  int iVar1;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  if ((DAT_00719720 == 1) || (DAT_00719720 == 2)) {
    *(undefined1 *)(iVar1 + 0x244) = 0;
    *(undefined1 *)(iVar1 + 0x245) = 0;
    *(undefined1 *)(iVar1 + 0x246) = 0;
    *(undefined1 *)(iVar1 + 9) = 0;
  }
  return CONCAT31((int3)((uint)iVar1 >> 8),1);
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
