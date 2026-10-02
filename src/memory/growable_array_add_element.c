// growable_array_add_element  (Ghidra: growable_array_add_element, already named)
// address 0x4cf810, size 124 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: out/phase4/memory_types_notes.md "growable_array (0x0c)"; callees GlobalAlloc/
// GlobalFree/GlobalReAlloc; types/memory.h growable_array {element_size, count, data}. The
// element/count/data triple is read/written exactly as unaff_ESI[0..2] in the Ghidra output.
// register convention: array pointer arrives in ESI (unaff_ESI), never materialized as a formal
// parameter because it is never reassigned in this function -- exposed here as the sole
// parameter per the EAX,ECX,EDX,EBX,ESI,EDI,stack order.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif


#define GMEM_MOVEABLE 0x0002

// blam-cc: array in ESI
// Appends one zero-initialized element, growing the GlobalAlloc-backed storage as needed.
// Returns the new element's index, or -1 (k_datum_index_none) on allocation failure.
uint32_t growable_array_add_element(growable_array *array)
{
    uint32_t new_count;
    uint32_t bytes;
    void *new_data;
    uint32_t old_count;
    uint32_t element_size;
    uint8_t *dst;
    uint32_t i;

    if (array->count < 0x7fffffff) {
        new_count = array->count + 1;
        bytes = (uint32_t)array->element_size * new_count;
        new_data = array->data;
        if (new_data == 0) {
            new_data = GlobalAlloc(0, bytes);
        } else {
            if (bytes == 0) {
                GlobalFree(new_data);
                return 0xffffffff;
            }
            new_data = GlobalReAlloc(new_data, bytes, GMEM_MOVEABLE);
        }
        if (new_data != 0) {
            element_size = (uint32_t)array->element_size;
            old_count = (uint32_t)array->count;
            dst = (uint8_t *)new_data + element_size * old_count;
            // Ghidra unrolls this as a dword loop plus a trailing byte loop; a plain zero-fill
            // of element_size bytes produces the identical memory contents.
            for (i = 0; i < element_size; i = i + 1) {
                dst[i] = 0;
            }
            array->count = (int32_t)new_count;
            array->data = new_data;
            return old_count;
        }
    }
    return 0xffffffff;
}

#if 0
Original Ghidra decompilation (0x4cf810):

uint growable_array_add_element(void)

{
  uint uVar1;
  uint uVar2;
  SIZE_T dwBytes;
  HGLOBAL pvVar3;
  uint uVar4;
  uint uVar5;
  uint *unaff_ESI;
  undefined4 *puVar6;
  
  if ((int)unaff_ESI[1] < 0x7fffffff) {
    uVar1 = unaff_ESI[1] + 1;
    dwBytes = *unaff_ESI * uVar1;
    pvVar3 = (HGLOBAL)unaff_ESI[2];
    if (pvVar3 == (HGLOBAL)0x0) {
      pvVar3 = GlobalAlloc(0,dwBytes);
    }
    else {
      if (dwBytes == 0) {
        GlobalFree(pvVar3);
        return 0xffffffff;
      }
      pvVar3 = GlobalReAlloc(pvVar3,dwBytes,2);
    }
    if (pvVar3 != (HGLOBAL)0x0) {
      uVar5 = *unaff_ESI;
      uVar2 = unaff_ESI[1];
      puVar6 = (undefined4 *)(uVar5 * uVar2 + (int)pvVar3);
      for (uVar4 = uVar5 >> 2; uVar4 != 0; uVar4 = uVar4 - 1) {
        *puVar6 = 0;
        puVar6 = puVar6 + 1;
      }
      for (uVar5 = uVar5 & 3; uVar5 != 0; uVar5 = uVar5 - 1) {
        *(undefined1 *)puVar6 = 0;
        puVar6 = (undefined4 *)((int)puVar6 + 1);
      }
      unaff_ESI[1] = uVar1;
      unaff_ESI[2] = (uint)pvVar3;
      return uVar2;
    }
  }
  return 0xffffffff;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
