// object_lists_dispose_empty  (Ghidra: object_lists_dispose_empty, already named; matches
// out/phase4/hs_types_notes.md's own description of this exact function)
// address 0x48b340, size 152 bytes
// name confidence: 0.55   rewrite confidence: 0.75
// evidence: types/hs.h object_list_header (reference_count at 0x04); this function's own raw
//   bytes are hs_types_notes.md's cited evidence for that field being a holder count rather than
//   an element count.
// register convention: none (void).

#include "tags.h"
#include "memory.h"
#include "hs.h"
#include "fn_hs.h"

extern datum_index datum_next(int16_t after_index, data_array *array);
    // blam-cc: DX -> after_index, EDI -> array; memory module, 0x4d0630

    // this module, 0x48b220
extern void datum_delete(data_array *array, datum_index handle); // blam-cc: EAX -> array,
    // EDX -> handle; memory module, 0x4d0510

extern data_array *object_list_header_data;    // 0x0087a464
extern data_array *object_list_reference_data; // 0x0087a468

// Sweeps every object_list_header whose reference_count (holder count) has dropped to zero,
// deleting its reference chain and then the header itself.
void object_lists_dispose_empty(void)
{
    datum_index header_index;
    object_list_header *header;

    header_index = datum_next(-1, object_list_header_data);
    while (header_index != k_datum_index_none) {
        header = (object_list_header *)((uint8_t *)object_list_header_data->data +
            (header_index & 0xffff) * 0x0c);
        if (header->reference_count == 0) {
            object_list_reference_chain_delete(object_list_reference_data, header->first_reference);
            datum_delete(object_list_header_data, header_index);
        }
        header_index = datum_next((int16_t)header_index, object_list_header_data);
    }
}

#if 0
Original Ghidra decompilation (0x48b340):

void object_lists_dispose_empty(void)

{
  int iVar1;
  uint uVar2;
  short *psVar3;
  short sVar4;
  int iVar5;

  iVar1 = DAT_0087a464;
  uVar2 = FUN_004d0630();
  do {
    do {
      if (uVar2 == 0xffffffff) {
        return;
      }
      if ((*(short *)(*(int *)(iVar1 + 0x34) + 4 + (uVar2 & 0xffff) * 0xc) == 0) &&
         (uVar2 != 0xffffffff)) {
        object_list_reference_chain_delete();
        datum_delete();
      }
      iVar5 = uVar2 + 1;
      uVar2 = 0xffffffff;
      sVar4 = (short)iVar5;
    } while ((sVar4 < 0) || (*(short *)(iVar1 + 0x2e) <= sVar4));
    psVar3 = (short *)((int)sVar4 * (int)*(short *)(iVar1 + 0x22) + *(int *)(iVar1 + 0x34));
    do {
      if (*psVar3 != 0) {
        uVar2 = (int)*psVar3 << 0x10 | (int)(short)iVar5;
        break;
      }
      iVar5 = iVar5 + 1;
      psVar3 = (short *)((int)psVar3 + (int)*(short *)(iVar1 + 0x22));
    } while ((short)iVar5 < *(short *)(iVar1 + 0x2e));
  } while( true );
}
#endif
