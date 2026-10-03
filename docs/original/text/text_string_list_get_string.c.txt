// text_string_list_get_string  (Ghidra: text_string_list_get_string, already named)
// address 0x5578c0, size 73 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: out/phase4/text_types_notes.md: "0x5578c0 text_string_list_get_string reads a
//   UnicodeStringList and returns uint16_t*, with the fallback 0x00671fac
//   L\"<missing string>\"." and "StringListString / UnicodeStringListString: 0x14 stride,
//   size at +0, pointer at +0x0c. 0x5578c0 terminates at (size & ~1) - 2 as a 16-bit
//   store, so it reads the unicode string list." The tag_instances[list_id].data lookup
//   is the same idx*0x20+0x14 pattern documented in types/cache.h.
// register convention: ECX = list_id (datum_index, in_ECX), DX = index (in_DX). No stack
//   arguments (Ghidra's fully-unrecognized signature "text_string_list_get_string(void)").

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "text.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern tag_instance *tag_instances;      // 0x0087bc14
extern uint16_t missing_string_text[];   // 0x00671fac, L"<missing string>"

// blam-cc: ECX=list_id, EDX=index
// Fetches string index out of the unicode string list tag list_id, returning a pointer
// to its (possibly trimmed by one trailing code unit) text, or missing_string_text if
// list_id is -1, index is out of range, or the entry is empty.
uint16_t *text_string_list_get_string(datum_index list_id, int16_t index)
{
    UnicodeStringList *list;
    UnicodeStringListString *entry;

    if (list_id == (datum_index)-1) {
        return missing_string_text;
    }
    list = (UnicodeStringList *)tag_instances[list_id & 0xffff].data;
    if (index < 0 || (int32_t)list->strings.count <= index) {
        return missing_string_text;
    }
    entry = (UnicodeStringListString *)list->strings.pointer + index;
    if (0 < (int32_t)entry->string.size) {
        uint16_t *string = (uint16_t *)entry->string.pointer;
        *(uint16_t *)((uint8_t *)string + ((entry->string.size & ~1u) - 2)) = 0;
        return string;
    }
    return missing_string_text;
}

#if 0
Original Ghidra decompilation (0x5578c0):

undefined ** text_string_list_get_string(void)

{
  uint *puVar1;
  int *piVar2;
  uint uVar3;
  undefined **ppuVar4;
  uint in_ECX;
  short in_DX;

  ppuVar4 = &PTR_DAT_00671fac;
  if (((in_ECX != 0xffffffff) &&
      (piVar2 = *(int **)((in_ECX & 0xffff) * 0x20 + 0x14 + DAT_0087bc14), -1 < in_DX)) &&
     ((int)in_DX < *piVar2)) {
    puVar1 = (uint *)(piVar2[1] + in_DX * 0x14);
    uVar3 = *puVar1;
    if (0 < (int)uVar3) {
      ppuVar4 = (undefined **)puVar1[3];
      *(undefined2 *)((int)ppuVar4 + ((uVar3 & 0xfffffffe) - 2)) = 0;
    }
  }
  return ppuVar4;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
