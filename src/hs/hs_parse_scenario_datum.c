// hs_parse_scenario_datum  (Ghidra: hs_parse_scenario_datum, already named; confirmed by the CEA
// prototype's string overlap on "this is not a valid %s name")
// address 0x486f90, size 149 bytes
// name confidence: 0.85   rewrite confidence: 0.75
// evidence: types/tags.h TagReflexive {count, pointer, definition} matches the {count; pointer}
//   pair read off the array-descriptor argument; the one recovered caller (hs_parse_trigger_volume
//   @0x487030) passes stride 0x60, exactly sizeof(ScenarioTriggerVolume); hs.h's hs_syntax_node
//   (type 0x04, source_offset 0x0c, data 0x10) and globals list (hs_compiled_source, hs_compile_error,
//   hs_compile_error_offset, hs_compile_error_buffer, hs_type_names).
// register convention: node index in EAX (in_EAX); name-field byte offset in EBX (unaff_BX, only
//   the low 16 bits are read); array descriptor pointer in ESI (unaff_ESI); element stride as the
//   recognized stack parameter (param_1). Neither EBX nor ESI is assigned inside this function, so
//   both are true incoming register arguments set up by the (not yet recovered) caller/dispatcher.
//   // blam-cc: EAX -> node_index, EBX -> name_offset, ESI -> array, stack -> stride
// UNSURE: the caller that establishes name_offset/array per hs_type lives below 0x486ce0
//   (hs_parse_primitive, 0x486420) and is out of this batch's range, so the exact per-type values
//   it passes are inferred only from the one in-range caller.

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "hs.h"


#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *hs_syntax_data;                 // 0x0087a474
extern char *hs_compiled_source;                    // 0x006b14c0
extern char *hs_compile_error;                      // 0x006b14d4
extern int32_t hs_compile_error_offset;             // 0x006b14d8
extern char hs_compile_error_buffer[k_hs_error_buffer_size]; // 0x006b14dc
extern char *hs_type_names[k_hs_type_count];        // 0x00688a78

// Generic parser for a quoted name token against an arbitrary-stride scenario tag-block array:
// linearly scans `array` (an element `count` and a `pointer` to the first element, laid out like
// a TagReflexive) comparing each element's TagString name (at `name_offset` bytes into the
// element) case-insensitively against the token text. On a match, stores the element's index in
// the node and returns 1 (success). On no match, formats a "this is not a valid %s name" compiler
// error and returns 0.
char hs_parse_scenario_datum(datum_index node_index, int16_t name_offset, TagReflexive *array,
    int32_t stride)
{
    hs_syntax_node *node;
    int16_t i;
    char *element;

    node = (hs_syntax_node *)((uint8_t *)hs_syntax_data->data + (node_index & 0xffff) * 0x14);

    for (i = 0; i < (int16_t)array->count; i++) {
        element = (char *)array->pointer + i * stride;
        if (_stricmp(element + name_offset, hs_compiled_source + node->source_offset) == 0) {
            node->data.long_value = i;
            return 1;
        }
    }

    sprintf(hs_compile_error_buffer, "this is not a valid %s name", hs_type_names[node->type]);
    hs_compile_error = hs_compile_error_buffer;
    hs_compile_error_offset = node->source_offset;
    return 0;
}

#if 0
Original Ghidra decompilation (0x486f90):

uint hs_parse_scenario_datum(int param_1)

{
  int iVar1;
  uint in_EAX;
  int iVar2;
  short unaff_BX;
  short sVar3;
  int *unaff_ESI;

  iVar1 = *(int *)(DAT_0087a474 + 0x34) + (in_EAX & 0xffff) * 0x14;
  sVar3 = 0;
  if (0 < *unaff_ESI) {
    iVar2 = 0;
    do {
      iVar2 = __stricmp((char *)((int)unaff_BX + iVar2 * param_1 + unaff_ESI[1]),
                        (char *)(*(int *)(iVar1 + 0xc) + DAT_006b14c0));
      if (iVar2 == 0) {
        *(int *)(iVar1 + 0x10) = (int)sVar3;
        return CONCAT31((int3)(char)((ushort)sVar3 >> 8),1);
      }
      sVar3 = sVar3 + 1;
      iVar2 = (int)sVar3;
    } while (iVar2 < *unaff_ESI);
  }
  _sprintf(&DAT_006b14dc,"this is not a valid %s name",
           (&PTR_s_unparsed_00688a78)[*(short *)(iVar1 + 4)]);
  DAT_006b14d4 = &DAT_006b14dc;
  DAT_006b14d8 = *(uint *)(iVar1 + 0xc);
  return *(uint *)(iVar1 + 0xc) & 0xffffff00;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
