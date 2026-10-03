// object_datum_consume_pending_flag
// address 0x4f46b0, size 78 bytes
// name confidence: 0.45 (still FUN_004f46b0 in Ghidra; types/objects.h's object.flags comment
//   names this exact test-and-clear-bit-0x04000000 pattern "object_datum_consume_pending_flag",
//   which is the core of what this function does)
// rewrite confidence: 0.55
// evidence: types/objects.h object_header (data field), object (flags at 0x10,
//   _object_changed_bit 0x04000000, _object_at_rest_bit 0x00000020); global 0x008603b0
//   object_data.
// register convention: object index in ECX (in_ECX).
// UNSURE: this function also reads and writes object+0x08 and object+0x09 as individual bytes.
//   types/objects.h marks object 0x008 ("unknown_008", declared uint32_t) as "written at create
//   but never read anywhere in this module" -- that note predates this function being examined.
//   The two bytes are kept as raw offsets rather than folded into the existing uint32_t field,
//   since their true width/meaning (a flag byte and a one-shot latch byte, per functions.md) is
//   not settled and the header is not redefined here.

// RETURN TYPE (phase-4 review pass): Ghidra returns this as CONCAT31(garbage, AL) / bool,
//   i.e. only the low byte is defined -- the top three bytes are whatever happened to be in
//   the register. The return type is uint8_t so no caller can depend on the garbage, which
//   is the same correction already recorded for object_type_definitions_query_0x44 0x4f41d0.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data; // 0x008603b0

uint8_t object_datum_consume_pending_flag(uint32_t object_index) // blam-cc: ECX -> object_index
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    uint8_t *raw = (uint8_t *)obj;
    int changed = (obj->flags & _object_changed_bit) != 0;

    if (changed) {
        obj->flags &= ~(uint32_t)_object_changed_bit;
    }
    if (raw[8] != 0 && (obj->flags & _object_at_rest_bit) != 0) {
        uint8_t latch = raw[9];
        raw[9] = 1;
        return latch == 0 || changed;
    }
    raw[9] = 0;
    return changed;
}

#if 0
Original Ghidra decompilation (0x4f46b0):

bool FUN_004f46b0(void)

{
  char cVar1;
  int iVar2;
  uint in_ECX;
  bool bVar3;

  iVar2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_ECX & 0xffff) * 0xc);
  bVar3 = (*(uint *)(iVar2 + 0x10) & 0x4000000) != 0;
  if (bVar3) {
    *(uint *)(iVar2 + 0x10) = *(uint *)(iVar2 + 0x10) & 0xfbffffff;
  }
  if ((*(char *)(iVar2 + 8) != '\0') && ((*(byte *)(iVar2 + 0x10) & 0x20) != 0)) {
    cVar1 = *(char *)(iVar2 + 9);
    *(undefined1 *)(iVar2 + 9) = 1;
    return cVar1 == '\0' || bVar3;
  }
  *(undefined1 *)(iVar2 + 9) = 0;
  return bVar3;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
