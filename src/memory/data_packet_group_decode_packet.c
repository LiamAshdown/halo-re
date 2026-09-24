// data_packet_group_decode_packet
// address 0x4d09d0, size 270 bytes
// name confidence: 0.85 (cea-pdb string match against all three of its error strings)
// rewrite confidence: 0.55 -- the header lookup/dispatch logic is fully visible and preserved
// exactly. One elided call remains: data_packet_group_decode_packet_body needs a `definition`
// pointer and an input `byte_stream*` that Ghidra's decompile of THIS function never shows being
// loaded, even though the header-byte addressing pattern it evidences elsewhere is reused to
// reconstruct the version-byte source pointer with reasonable confidence (see the UNSURE note at
// its use). `definition` itself is not a guess: it is exactly the same
// group->types[type].definition pointer this function already null-checks two lines above.
// evidence: data_packet_group (+0x04 type_count, +0x10 types) and data_packet_type (packet_class
// at +0, definition at +4) from types/memory.h; the three literal error strings.
// register convention: remaining-input-byte-count pointer in EAX (in_EAX); stack: data_packet_group*
// (param_1), decode destination struct instance (param_2), raw input buffer base (param_3),
// out decoded packet type (param_4), out version actually used (param_5, forwarded to
// data_packet_group_decode_packet_body's out_version_used), expected packet class (param_6).

#include "tags.h"
#include "memory.h"

extern char *data_packet_group_error; // 0x006b7f00

// blam-cc (0x4cfee0, below this batch's assigned range, already rewritten at
// src/memory/struct_definition_byte_swap.c): definition, data (as an int32_t offset/address, per
// that file), codes table, out-consumed-bytes/out-consumed-records (NULL when not wanted).
extern void struct_definition_byte_swap(byte_swap_definition *definition, int32_t data,
    int32_t *codes, int32_t *out_consumed_bytes, int32_t *out_consumed_records);
extern byte_swap_definition packet_header_byte_swap_definition; // 0x00696780
// blam-cc (0x4d0c70): see data_packet_group_decode_packet_body.c for the full parameter
// reconstruction and its UNSURE notes.
extern int32_t data_packet_group_decode_packet_body(uint8_t *version_byte_src,
    struct_definition *definition, int16_t remaining_length, void *dest, byte_stream *input,
    uint16_t *out_version_used, int16_t *out_attempted_version_byte);

int32_t data_packet_group_decode_packet(int16_t *remaining_length, data_packet_group *group,
    void *decoded_body, uint8_t *buffer, int16_t *out_type, byte_stream *input,
    uint16_t *out_version_used, int16_t expected_class)
{
    uint8_t *header_byte;
    int8_t type;

    if (*remaining_length == 0) {
        data_packet_group_error = "got packet with no header";
        return 0;
    }

    header_byte = buffer + (*remaining_length - 1);
    if (header_byte != 0) {
        struct_definition_byte_swap(&packet_header_byte_swap_definition, (int32_t)header_byte,
            packet_header_byte_swap_definition.codes, 0, 0);
    }
    type = (int8_t)*header_byte;

    if (-1 < type && type < group->type_count) {
        data_packet_type *entry = &group->types[(int)type];
        if (entry->packet_class != expected_class) {
            data_packet_group_error = "got packet with mismatched class";
            return 0;
        }
        *remaining_length = *remaining_length - 1;
        if (entry->definition != 0) {
            // UNSURE: version_byte_src is reconstructed using the exact same
            // `buffer + (*remaining_length - 1)` formula this function already uses for the
            // header byte, now applied to the post-decrement remaining_length.
            // UNSURE (open question for hook verification): Ghidra shows this call as
            // FUN_004d0c70(*in_EAX, param_2, param_5, 0) -- four arguments. The callee's own
            // decompile likewise never dereferences its param_2, so param_2 is EITHER the
            // destination structure OR the byte_stream that struct_definition_decode reads from;
            // one of the two must additionally arrive in a register Ghidra did not surface. This
            // rewrite splits them into `decoded_body` (= param_2) and a separate `input`, but the
            // opposite assignment is equally consistent with the decompile. A breakpoint on
            // 0x4d0c70 reading EAX/ECX/EDX/ESI/EDI and the two stack slots settles it.
            uint8_t *version_byte_src = buffer + (*remaining_length - 1);
            int32_t decoded = data_packet_group_decode_packet_body(version_byte_src,
                entry->definition, *remaining_length, decoded_body, input, out_version_used, 0);
            if (decoded == 0) {
                data_packet_group_error = "got packet which wouldn't decode";
                return 0;
            }
        }
        *out_type = (int16_t)(int8_t)*header_byte; // Ghidra: *param_4 = (short)*pcVar1, a
            // SIGNED char widened -- immaterial here because the -1 < type test above already
            // rejected any byte with bit 7 set, but kept literal.
        data_packet_group_error = 0;
        return 1;
    }
    data_packet_group_error = "got packet with bad type";
    return 0;
}

#if 0
Original Ghidra decompilation (0x4d09d0):

undefined4
data_packet_group_decode_packet
          (int param_1,undefined4 param_2,int param_3,short *param_4,undefined4 param_5,
          short param_6)

{
  char *pcVar1;
  int iVar2;
  char cVar3;
  short *in_EAX;

  if (*in_EAX == 0) {
    DAT_006b7f00 = "got packet with no header";
    return 0;
  }
  pcVar1 = (char *)(*in_EAX + -1 + param_3);
  if (pcVar1 != (char *)0x0) {
    struct_definition_byte_swap(&PTR_s_packet_header_00696780,pcVar1,PTR_DAT_00696788,0,0);
  }
  cVar3 = *pcVar1;
  if ((-1 < cVar3) && ((short)cVar3 < *(short *)(param_1 + 4))) {
    iVar2 = *(int *)(param_1 + 0x10);
    if (*(short *)(iVar2 + cVar3 * 8) != param_6) {
      DAT_006b7f00 = "got packet with mismatched class";
      return 0;
    }
    *in_EAX = *in_EAX + -1;
    if (*(int *)(iVar2 + cVar3 * 8 + 4) != 0) {
      cVar3 = FUN_004d0c70(*in_EAX,param_2,param_5,0);
      if (cVar3 == '\0') {
        DAT_006b7f00 = "got packet which wouldn\'t decode";
        return 0;
      }
    }
    *param_4 = (short)*pcVar1;
    DAT_006b7f00 = (char *)0x0;
    return 1;
  }
  DAT_006b7f00 = "got packet with bad type";
  return 0;
}
#endif
