// growable_array_remove_element  (Ghidra: growable_array_remove_element, already named)
// address 0x4cf890, size 96 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: same growable_array struct as growable_array_add_element (0x4cf810); callee
// memmove and GlobalAlloc/GlobalFree/GlobalReAlloc confirm the same three-field layout.
// register convention: array pointer in ESI (unaff_ESI), index in EDI (unaff_EDI); exposed in
// that order (ESI before EDI) per the EAX,ECX,EDX,EBX,ESI,EDI,stack rule.

#include "crt.h"
#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "fn_memory.h"


#define GMEM_MOVEABLE 0x0002

// blam-cc: array in ESI, index in EDI
// Removes the element at `index`, compacting the elements above it down by one slot and
// shrinking (or freeing) the GlobalAlloc-backed storage to match the new count. No return value.
void growable_array_remove_element(growable_array *array, uint32_t index)
{
    uint32_t new_count;
    uint32_t element_size;
    uint8_t *dst;
    uint32_t bytes;
    void *data;

    new_count = (uint32_t)array->count - 1;
    array->count = (int32_t)new_count;
    if (index < new_count) {
        element_size = (uint32_t)array->element_size;
        dst = (uint8_t *)array->data + element_size * index;
        memmove(dst, dst + element_size, (new_count - index) * element_size);
    }
    data = array->data;
    bytes = (uint32_t)array->element_size * (uint32_t)array->count;
    if (data != 0) {
        if (bytes != 0) {
            array->data = GlobalReAlloc(data, bytes, GMEM_MOVEABLE);
            return;
        }
        GlobalFree(data);
        array->data = 0;
        return;
    }
    array->data = GlobalAlloc(0, bytes);
}

#if 0
Original Ghidra decompilation (0x4cf890):

void growable_array_remove_element(void)

{
  int iVar1;
  void *_Dst;
  SIZE_T dwBytes;
  HGLOBAL pvVar2;
  int iVar3;
  int *unaff_ESI;
  int unaff_EDI;
  
  iVar3 = unaff_ESI[1] + -1;
  unaff_ESI[1] = iVar3;
  if (unaff_EDI < iVar3) {
    iVar1 = *unaff_ESI;
    _Dst = (void *)(iVar1 * unaff_EDI + unaff_ESI[2]);
    _memmove(_Dst,(void *)(iVar1 + (int)_Dst),(iVar3 - unaff_EDI) * iVar1);
  }
  pvVar2 = (HGLOBAL)unaff_ESI[2];
  dwBytes = *unaff_ESI * unaff_ESI[1];
  if (pvVar2 != (HGLOBAL)0x0) {
    if (dwBytes != 0) {
      pvVar2 = GlobalReAlloc(pvVar2,dwBytes,2);
      unaff_ESI[2] = (int)pvVar2;
      return;
    }
    GlobalFree(pvVar2);
    unaff_ESI[2] = 0;
    return;
  }
  pvVar2 = GlobalAlloc(0,dwBytes);
  unaff_ESI[2] = (int)pvVar2;
  return;
}
#endif
