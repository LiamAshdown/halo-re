// object_release_render_cache_slot  (named by types/objects.h's own object.render_cache_slot
// comment: "0x0ba render cache slot | object_reserve_render_cache_slot 0x4f9ac0,
// object_release_render_cache_slot 0x4f9b00")
// address 0x4f9b00, size 99 bytes
// name confidence: 0.85 (fixed by the types notes' own citation of this address by this name)
// rewrite confidence: 0.9 (VERIFIED against objdump)
// evidence: types/objects.h object (render_cache_slot 0x0ba); global 0x008603b0 object_data,
//   global 0x006b8cb8 object_name_list (reused as the render-cache slot table).
// register convention: object index in EDI. Confirmed against objdump-consistent pattern:
//   Ghidra shows only "unaff_EDI", no stack access.
//   // blam-cc: EDI -> object_index
// UNSURE: the loop bound at player_globals_table+0x204 is not otherwise established in this
//   module; kept as a raw offset off the same DAT_00746f8c global used elsewhere in this batch.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern data_array *object_data; // 0x008603b0
extern uint8_t *player_globals_table; // 0x00746f8c, see objects_set_ambient_cluster_override.c
extern datum_index *object_name_list; // 0x006b8cb8

void object_release_render_cache_slot(uint32_t object_index) // blam-cc: EDI -> object_index
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;

    if (obj->render_cache_slot != -1) {
        int32_t count = *(int32_t *)(player_globals_table + 0x204); // UNSURE: see file header
        int16_t i;

        obj->render_cache_slot = -1;

        for (i = 0; i < count; i++) {
            if (object_name_list[i] == object_index) {
                object_name_list[i] = k_datum_index_none;
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x4f9b00):

void FUN_004f9b00(void)

{
  int iVar1;
  int iVar2;
  int iVar3;
  short sVar4;
  uint unaff_EDI;

  iVar2 = DAT_00746f8c;
  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (unaff_EDI & 0xffff) * 0xc);
  if (*(short *)(iVar1 + 0xba) != -1) {
    *(undefined2 *)(iVar1 + 0xba) = 0xffff;
    iVar1 = DAT_006b8cb8;
    sVar4 = 0;
    if (0 < *(int *)(iVar2 + 0x204)) {
      iVar3 = 0;
      do {
        if (*(uint *)(iVar1 + iVar3 * 4) == unaff_EDI) {
          *(undefined4 *)(iVar1 + iVar3 * 4) = 0xffffffff;
        }
        sVar4 = sVar4 + 1;
        iVar3 = (int)sVar4;
      } while (iVar3 < *(int *)(iVar2 + 0x204));
    }
  }
  return;
}
#endif
