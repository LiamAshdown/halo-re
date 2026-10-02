// hs_parse_real  (Ghidra: hs_parse_real, already named)
// address 0x486ae0, size 155 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: CEA-PDB string match ("this is not a valid real number."); validates an optional
// leading '-', digits and at most one '.', then always converts with atof regardless of
// validity (the error path falls through to the same conversion/assignment before returning
// failure, matching Ghidra's shared LAB_00486b5d tail).
// register convention: __cdecl, node_index is the recognized single stack parameter.

#include "tags.h"
#include "memory.h"
#include "hs.h"
#include <ctype.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *hs_syntax_data; // 0x0087a474
extern char *hs_compiled_source;   // 0x006b14c0
extern char *hs_compile_error;     // 0x006b14d4
extern int32_t hs_compile_error_offset; // 0x006b14d8

// Validates and parses a floating point literal token from the script source text into the
// node's real value field, reporting an error (but still storing whatever atof makes of the
// text) if it is not a valid real number.
char hs_parse_real(datum_index node_index)
{
    hs_syntax_node *node;
    char *p;
    char c;
    char valid;
    char has_dot;

    node = (hs_syntax_node *)((uint8_t *)hs_syntax_data->data + (node_index & 0xffff) * hs_syntax_data->size);
    p = hs_compiled_source + node->source_offset;
    has_dot = 0;
    valid = 1;
    if (*p == '-') {
        p = p + 1;
    }
    c = *p;
    for (;;) {
        if (c == '\0') {
            goto convert;
        }
        if (!isdigit((unsigned char)c)) {
            if ((has_dot != 0) || (*p != '.')) {
                hs_compile_error = (char *)"this is not a valid real number.";
                hs_compile_error_offset = node->source_offset;
                valid = 0;
                goto convert;
            }
            has_dot = 1;
        }
        c = p[1];
        p = p + 1;
    }
convert:
    node->data.real_value = (float)atof(hs_compiled_source + node->source_offset);
    return valid;
}

#if 0
Original Ghidra decompilation (0x486ae0):

undefined1 hs_parse_real(uint param_1)

{
  int iVar1;
  char cVar2;
  bool bVar3;
  int iVar4;
  char *pcVar5;
  double dVar6;
  undefined1 local_1;

  iVar4 = *(int *)(*(int *)(DAT_0087a474 + 0x34) + 0xc + (param_1 & 0xffff) * 0x14);
  iVar1 = *(int *)(DAT_0087a474 + 0x34) + (param_1 & 0xffff) * 0x14;
  pcVar5 = (char *)(iVar4 + DAT_006b14c0);
  bVar3 = false;
  local_1 = 1;
  if (*(char *)(iVar4 + DAT_006b14c0) == '-') {
    pcVar5 = pcVar5 + 1;
  }
  cVar2 = *pcVar5;
  do {
    if (cVar2 == '\0') {
LAB_00486b5d:
      dVar6 = _atof((char *)(*(int *)(iVar1 + 0xc) + DAT_006b14c0));
      *(float *)(iVar1 + 0x10) = (float)dVar6;
      return local_1;
    }
    iVar4 = _isdigit((int)cVar2);
    if (iVar4 == 0) {
      if ((bVar3) || (*pcVar5 != '.')) {
        DAT_006b14d4 = "this is not a valid real number.";
        DAT_006b14d8 = *(undefined4 *)(iVar1 + 0xc);
        local_1 = 0;
        goto LAB_00486b5d;
      }
      bVar3 = true;
    }
    cVar2 = pcVar5[1];
    pcVar5 = pcVar5 + 1;
  } while( true );
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
