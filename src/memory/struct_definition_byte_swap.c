// struct_definition_byte_swap  (Ghidra: struct_definition_byte_swap, already named)
// address 0x4cfee0, size 512 bytes
// name confidence: 0.6   rewrite confidence: 0.55
// evidence: out/phase4/memory_types_notes.md "byte_swap_definition (0x14) and its code tables";
// the case values (-0x66/-100/-0x65/-8/-4/-2) match types/memory.h byte_swap_code exactly
// (_byte_swap_definition_reference=-102, _byte_swap_begin_struct=-100, _byte_swap_end_struct=-101,
// _byte_swap_int64/-32/-16=-8/-4/-2); codes[1] = record count, codes[2..] = field codes, matching
// the byte_swap_definition::codes layout documented in types/memory.h.
// register convention: cc unknown in the pack; the recursive calls pass all five arguments the
// same way Ghidra shows them (definition, data offset flag, codes, out_size, out_record_count),
// which is preserved verbatim as this function's own parameter list -- no in_/unaff_ registers
// appear in this function's body.
// UNSURE: "definition" (param_1) is threaded through every recursive call but never dereferenced
// by this function itself; it is only actually read one level down, through the -102 reference
// case, where the new definition pointer (read from the codes table) becomes param_1 of the
// nested call. Its purpose at this level is unclear, possibly a leftover owning-definition
// argument for error reporting that this build never uses. Preserved exactly as decompiled.

#include "tags.h"
#include "memory.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// blam-cc: (definition, data, codes, out_size, out_record_count) -- see file header
// Recursively interprets a byte_swap_definition code table (types/memory.h byte_swap_code) to
// byte-swap (or, when data is NULL, merely measure) one structure instance described by codes.
// codes[1] is the number of concatenated structure records in the table; codes[2..] are the
// field codes for the first record. On return, *out_size (if non-NULL) holds the total byte
// size of the record(s) processed and *out_record_count (if non-NULL) holds the number of int32
// code-table entries consumed, so a caller iterating a table with more than one record (e.g. a
// nested _struct_field_struct_array element) knows where the next record starts.
void struct_definition_byte_swap(byte_swap_definition *definition, int32_t data,
    int32_t *codes, int32_t *out_size, int32_t *out_record_count)
{
    int32_t *field;
    int32_t code;
    int32_t record_count;
    int32_t codes_consumed;
    int32_t offset;       // running byte offset within the record (also the running data address
                           // delta when data is nonzero)
    int32_t nested_size;
    int32_t nested_codes_consumed;
    uint32_t low;
    uint32_t high;
    int32_t field_data;

    record_count = codes[1];
    offset = 0;
    // UNSURE: Ghidra keeps the *out_record_count value in its own local (local_1c), assigned only
    // at end_of_record, so when codes[1] < 1 the original writes an UNINITIALIZED stack slot
    // through out_record_count. Seeding 0 here is the one place in this module where reproducing
    // the original exactly was not possible; for codes[1] >= 1 the two are identical, because
    // local_1c ends up holding the last record's consumed count + 1 just as this does.
    codes_consumed = 0;
    if (record_count < 1) {
        goto done;
    }

next_record:
    codes_consumed = 2;
    for (;;) {
        field = codes + codes_consumed;
        code = *field;
        switch (code) {
        case _byte_swap_definition_reference: { // -102
            byte_swap_definition *referenced = (byte_swap_definition *)field[1];
            int32_t referenced_data = (data == 0) ? 0 : offset + data;
            struct_definition_byte_swap(referenced, referenced_data, referenced->codes,
                &nested_size, 0);
            codes_consumed = codes_consumed + 2;
            offset = offset + nested_size;
            break;
        }
        case _byte_swap_end_struct: // -101
            goto end_of_record;
        case _byte_swap_begin_struct: { // -100
            int32_t nested_data = (data == 0) ? 0 : offset + data;
            struct_definition_byte_swap(definition, nested_data, field, &nested_size,
                &nested_codes_consumed);
            codes_consumed = codes_consumed + nested_codes_consumed;
            offset = offset + nested_size;
            break;
        }
        case _byte_swap_int64: // -8
            if (data != 0) {
                low = *(uint32_t *)(offset + data);
                high = *(uint32_t *)(offset + 4 + data);
                *(uint32_t *)(offset + data) =
                    (high >> 0x10 | ((high & 0xff0000) >> 0x10 | high & 0xff00) << 0x10) >> 8 |
                    high << 0x18;
                *(uint32_t *)(offset + 4 + data) =
                    (low << 0x10 | ((low & 0xff00) << 0x10 | low & 0xff0000) >> 0x10) << 8 |
                    low >> 0x18;
            }
            codes_consumed = codes_consumed + 1;
            offset = offset + 8;
            break;
        case _byte_swap_int32: // -4
            if (data != 0) {
                field_data = *(int32_t *)(offset + data);
                *(uint32_t *)(offset + data) =
                    ((uint32_t)field_data & 0xff0000 | (uint32_t)field_data >> 0x10) >> 8 |
                    ((uint32_t)field_data << 0x10 | (uint32_t)field_data & 0xff00) << 8;
            }
            codes_consumed = codes_consumed + 1;
            offset = offset + 4;
            break;
        case _byte_swap_int16: // -2
            if (data != 0) {
                uint16_t *p = (uint16_t *)(offset + data);
                *p = (uint16_t)(((*p & 0xff) << 8) | ((*p >> 8) & 0xff));
            }
            codes_consumed = codes_consumed + 1;
            offset = offset + 2;
            break;
        default:
            if (0 < code) { // positive code: skip `code` bytes, nothing swapped
                codes_consumed = codes_consumed + 1;
                offset = offset + code;
            }
            break;
        }
    }

end_of_record:
    codes_consumed = codes_consumed + 1;
    record_count = record_count - 1;
    if (record_count == 0) {
        goto done;
    }
    goto next_record;

done:
    if (out_size != 0) {
        *out_size = offset;
    }
    if (out_record_count != 0) {
        *out_record_count = codes_consumed;
    }
}

#if 0
Original Ghidra decompilation (0x4cfee0):

void struct_definition_byte_swap
               (undefined4 param_1,int param_2,int param_3,int *param_4,int *param_5)

{
  int *piVar1;
  uint uVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  int local_1c;
  int local_18;
  int local_14;
  int local_10;
  int local_c;
  uint local_8;

  local_18 = *(int *)(param_3 + 4);
  iVar5 = 0;
  if (local_18 < 1) {
LAB_004d00c6:
    if (param_4 != (int *)0x0) {
      *param_4 = iVar5;
    }
    if (param_5 != (int *)0x0) {
      *param_5 = local_1c;
    }
    return;
  }
LAB_004cff00:
  iVar4 = 2;
  do {
    piVar1 = (int *)(param_3 + iVar4 * 4);
    iVar3 = *piVar1;
    switch(iVar3) {
    case -0x66:
      if (param_2 == 0) {
        iVar3 = 0;
      }
      else {
        iVar3 = iVar5 + param_2;
      }
      struct_definition_byte_swap(piVar1[1],iVar3,*(undefined4 *)(piVar1[1] + 8),&local_c,0);
      iVar4 = iVar4 + 2;
      iVar5 = iVar5 + local_c;
      break;
    case -0x65:
      goto switchD_004cff2c_caseD_ffffff9b;
    case -100:
      if (param_2 == 0) {
        iVar3 = 0;
      }
      else {
        iVar3 = iVar5 + param_2;
      }
      struct_definition_byte_swap(param_1,iVar3,piVar1,&local_10,&local_14);
      iVar4 = iVar4 + local_14;
      iVar5 = iVar5 + local_10;
      break;
    default:
      if (0 < iVar3) {
        iVar4 = iVar4 + 1;
        iVar5 = iVar5 + iVar3;
      }
      break;
    case -8:
      if (param_2 != 0) {
        local_8 = *(uint *)(iVar5 + param_2);
        uVar2 = *(uint *)(iVar5 + 4 + param_2);
        *(uint *)(iVar5 + param_2) =
             (uVar2 >> 0x10 | ((uVar2 & 0xff0000) >> 0x10 | uVar2 & 0xff00) << 0x10) >> 8 |
             uVar2 << 0x18;
        *(uint *)(iVar5 + 4 + param_2) =
             (local_8 << 0x10 | ((local_8 & 0xff00) << 0x10 | local_8 & 0xff0000) >> 0x10) << 8 |
             local_8 >> 0x18;
      }
      iVar4 = iVar4 + 1;
      iVar5 = iVar5 + 8;
      break;
    case -4:
      if (param_2 != 0) {
        uVar2 = *(uint *)(iVar5 + param_2);
        *(uint *)(iVar5 + param_2) =
             (uVar2 & 0xff0000 | uVar2 >> 0x10) >> 8 | (uVar2 << 0x10 | uVar2 & 0xff00) << 8;
      }
      iVar4 = iVar4 + 1;
      iVar5 = iVar5 + 4;
      break;
    case -2:
      if (param_2 != 0) {
        *(ushort *)(iVar5 + param_2) =
             CONCAT11((char)*(undefined2 *)(iVar5 + param_2),
                      (char)((ushort)*(undefined2 *)(iVar5 + param_2) >> 8));
      }
      iVar4 = iVar4 + 1;
      iVar5 = iVar5 + 2;
    }
  } while( true );
switchD_004cff2c_caseD_ffffff9b:
  local_1c = iVar4 + 1;
  local_18 = local_18 + -1;
  if (local_18 == 0) goto LAB_004d00c6;
  goto LAB_004cff00;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
