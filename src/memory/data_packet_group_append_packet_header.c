// data_packet_group_append_packet_header
// address 0x4d0b60, size 86 bytes
// name confidence: 0.85 (cea-pdb string match: "couldn't append header to encoded packet")
// rewrite confidence: 0.6 -- body logic is fully recovered and unambiguous; the three register
// arguments are named from this function's OWN body only (not inferred from a caller), so their
// roles are solid, but see data_packet_group_encode_packet.c for why the call chain that reaches
// this function could not be fully reconstructed.
// evidence: types/memory.h data_packet_group (maximum_encoded_size at +0x0c) and
// packet_header_byte_swap_definition (0x00696780) / its codes (0x00696770).
// register convention: buffer base in ECX, data_packet_group* in EDX, running-cursor int16* in
// ESI (all unaff_/in_ style in Ghidra's own decompile of this function), header byte value on the
// stack.
// blam-cc: ECX -> buffer, EDX -> group, ESI -> cursor, stack -> header_byte
// FIXED (register inputs, objdump): ESI carries cursor (read at 0x4d0b60, movsx eax,[esi], the
// first instruction); the prose note described it but did not use a parseable "REG -> name"
// mapping, so it was dropped.

#include "tags.h"
#include "memory.h"

extern char *data_packet_group_error; // 0x006b7f00
extern byte_swap_definition packet_header_byte_swap_definition; // 0x00696780

// blam-cc (0x4cfee0, below this batch's assigned range, already rewritten at
// src/memory/struct_definition_byte_swap.c): definition, data (as an int32_t offset/address, per
// that file), codes table, out-consumed-bytes/out-consumed-records (NULL when not wanted).
extern void struct_definition_byte_swap(byte_swap_definition *definition, int32_t data,
    int32_t *codes, int32_t *out_consumed_bytes, int32_t *out_consumed_records);

// VERIFIED against disassembly 0x4d0b60..0x4d0bb5 (2026-09-30): signed cursor, unsigned limit test, the byte store, the 5
//   argument struct_definition_byte_swap call (definition 0x696780, codes [0x696788]), cursor increment and both error
//   states match. A remaining difftest header-bit mismatch would come from struct_definition_byte_swap, not from here.
int32_t data_packet_group_append_packet_header(uint8_t *buffer, data_packet_group *group,
    int16_t *cursor, uint8_t header_byte)
{
    uint8_t *dest = buffer + *cursor;

    if ((uint32_t)(*cursor + 1) < (uint32_t)group->maximum_encoded_size) {
        *dest = header_byte;
        struct_definition_byte_swap(&packet_header_byte_swap_definition, (int32_t)dest,
            packet_header_byte_swap_definition.codes, 0, 0);
        *cursor = *cursor + 1;
        data_packet_group_error = 0;
        return 1;
    }
    data_packet_group_error = (char *)"couldn't append header to encoded packet";
    return 0;
}

#if 0
Original Ghidra decompilation (0x4d0b60):

undefined4 data_packet_group_append_packet_header(undefined1 param_1)

{
  int in_ECX;
  undefined1 *puVar1;
  int in_EDX;
  short *unaff_ESI;

  puVar1 = (undefined1 *)(in_ECX + *unaff_ESI);
  if ((int)*unaff_ESI + 1U < *(uint *)(in_EDX + 0xc)) {
    *puVar1 = param_1;
    struct_definition_byte_swap(&PTR_s_packet_header_00696780,puVar1,PTR_DAT_00696788,0,0);
    *unaff_ESI = *unaff_ESI + 1;
    DAT_006b7f00 = (char *)0x0;
    return 1;
  }
  DAT_006b7f00 = "couldn\'t append header to encoded packet";
  return 0;
}
#endif
