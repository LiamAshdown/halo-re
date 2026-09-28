// data_new  (Ghidra: data_new, already named)
// address 0x4d0370, size 93 bytes
// name confidence: 0.7   rewrite confidence: 0.8
// evidence: out/phase4/memory_types_notes.md "data_array (0x38 header)"; every header field is
// written here exactly as documented (name via strncpy(31), maximum_count, size, valid=0,
// 'd@t@' signature, data = this+0x38); independently confirmed by the data_array_header
// byte-swap definition at 0x0068e39c.
// register convention: name as the recognized parameter (param_1); maximum_count as the
// recognized parameter (param_2); element size in BX (unaff_BX).

#include "crt.h"
#include "win32.h"
#include "tags.h"
#include "memory.h"


// blam-cc: element size in EBX, then the recognized stack parameters (name, maximum_count)
// Allocates and initializes a Blam data_array: header with name (copied, truncated to 31 chars
// plus NUL), maximum element count, element size, and a data pointer to its packed element
// storage immediately following the header. Returns NULL if the allocation fails.
data_array *data_new(int16_t element_size, char *name, int16_t maximum_count)
{
    data_array *array;
    uint8_t *zero;
    int32_t i;

    array = (data_array *)GlobalAlloc(0, (int32_t)maximum_count * (int32_t)element_size + 0x38);
    if (array != 0) {
        zero = (uint8_t *)array;
        for (i = 0xe; i != 0; i = i - 1) {
            zero[0] = 0;
            zero[1] = 0;
            zero[2] = 0;
            zero[3] = 0;
            zero = zero + 4;
        }
        strncpy(array->name, name, 0x1f);
        array->maximum_count = maximum_count;
        array->size = element_size;
        array->signature = k_data_array_signature; // '@t@d' in memory order, reads 'd@t@'
        array->data = (uint8_t *)array + 0x38;
        array->valid = 0;
    }
    return array;
}

#if 0
Original Ghidra decompilation (0x4d0370):

char * data_new(char *param_1,short param_2)

{
  char *_Dest;
  int iVar1;
  short unaff_BX;
  char *pcVar2;

  _Dest = GlobalAlloc(0,(int)param_2 * (int)unaff_BX + 0x38);
  if (_Dest != (char *)0x0) {
    pcVar2 = _Dest;
    for (iVar1 = 0xe; iVar1 != 0; iVar1 = iVar1 + -1) {
      pcVar2[0] = '\0';
      pcVar2[1] = '\0';
      pcVar2[2] = '\0';
      pcVar2[3] = '\0';
      pcVar2 = pcVar2 + 4;
    }
    _strncpy(_Dest,param_1,0x1f);
    *(short *)(_Dest + 0x20) = param_2;
    *(short *)(_Dest + 0x22) = unaff_BX;
    _Dest[0x28] = '@';
    _Dest[0x29] = 't';
    _Dest[0x2a] = '@';
    _Dest[0x2b] = 'd';
    *(char **)(_Dest + 0x34) = _Dest + 0x38;
    _Dest[0x24] = '\0';
  }
  return _Dest;
}
#endif
