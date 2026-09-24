// object_try_and_get  (Ghidra: object_try_and_get, already named)
// address 0x4f6ec0, size 92 bytes
// name confidence: 0.85 (already carries this name from an earlier phase; matches
//   functions.md's summary: "Resolves an object handle to its data pointer only if the object
//   still exists and matches the requested type mask, otherwise returns null")
// rewrite confidence: 0.85 (already used and its signature declared as an extern by
//   src/objects/object_unlink_cluster_or_notify_parent.c, this rewrite matches that extern
//   exactly)
// evidence: types/objects.h object_header (identifier 0x00, type 0x03); global 0x008603b0
//   object_data.
// register convention: object handle in ECX, type mask on the stack. Already established by
//   object_unlink_cluster_or_notify_parent.c's extern declaration; re-verified here against
//   Ghidra's own "object_try_and_get(uint param_1)" with in_ECX as the handle.
//   // blam-cc: ECX -> object_index, stack -> type_mask

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern data_array *object_data; // 0x008603b0

object *object_try_and_get(datum_index object_index, uint32_t type_mask) // blam-cc: ECX -> object_index, stack -> type_mask
{
    object_header *found = (object_header *)0;

    if ((object_index != k_datum_index_none) && ((int16_t)object_index >= 0) &&
        ((int16_t)object_index < object_data->maximum_count)) {
        object_header *header = (object_header *)object_data->data + (int16_t)object_index;
        if ((header->identifier != 0) &&
            (((int16_t)(object_index >> 16) == 0) || (header->identifier == (int16_t)(object_index >> 16)))) {
            found = header;
        }
    }

    if ((found != (object_header *)0) && ((type_mask & (1 << (found->type & 0x1f))) != 0)) {
        return found->data;
    }
    return (object *)0;
}

#if 0
Original Ghidra decompilation (0x4f6ec0):

undefined4 object_try_and_get(uint param_1)

{
  short *psVar1;
  undefined4 uVar2;
  short sVar3;
  int in_ECX;
  short sVar4;
  short *psVar5;

  psVar5 = (short *)0x0;
  if (((in_ECX != -1) && (sVar3 = (short)in_ECX, -1 < sVar3)) &&
     (sVar3 < *(short *)(DAT_008603b0 + 0x20))) {
    psVar1 = (short *)((int)*(short *)(DAT_008603b0 + 0x22) * (int)sVar3 +
                      *(int *)(DAT_008603b0 + 0x34));
    sVar3 = *psVar1;
    if ((sVar3 != 0) && ((sVar4 = (short)((uint)in_ECX >> 0x10), sVar4 == 0 || (sVar3 == sVar4)))) {
      psVar5 = psVar1;
    }
  }
  uVar2 = 0;
  if ((psVar5 != (short *)0x0) && ((param_1 & 1 << (*(byte *)((int)psVar5 + 3) & 0x1f)) != 0)) {
    uVar2 = *(undefined4 *)(psVar5 + 4);
  }
  return uVar2;
}
#endif
