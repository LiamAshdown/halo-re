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
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern char *data_packet_group_error; // 0x006b7f00

// blam-cc (0x4cfee0, below this batch's assigned range, already rewritten at
// src/memory/struct_definition_byte_swap.c): definition, data (as an int32_t offset/address, per
// that file), codes table, out-consumed-bytes/out-consumed-records (NULL when not wanted).
extern void struct_definition_byte_swap(byte_swap_definition *definition, int32_t data,
    int32_t *codes, int32_t *out_consumed_bytes, int32_t *out_consumed_records);
extern byte_swap_definition packet_header_byte_swap_definition; // 0x00696780
// blam-cc (0x4d0c70): see data_packet_group_decode_packet_body.c for the full parameter
// reconstruction and its UNSURE notes.
extern uint8_t data_packet_group_decode_packet_body(uint8_t *buffer, struct_definition *definition, int16_t remaining_length,
    void *dest, uint16_t *out_version_used, int16_t *out_bytes_consumed); // 0x4d0c70, EAX buffer, ESI definition

// REWRITTEN from objdump 0x4d09d0..0x4d0add. EAX points at the remaining length (an int16 the function decrements
// by the header byte); the stack holds (group, decoded_body, buffer, out_type, out_version_used, expected_class). The
// header byte is the last byte, buffer[remaining - 1]; its type indexes group->types (8-byte entries: class, then the
// struct definition); the body is decoded from the START of the buffer (EAX = buffer at 0x4d0a55) with the
// decremented length. Returns 1 when no error string was set.
// blam-cc: EAX -> remaining_length, stack -> group, decoded_body, buffer, out_type, out_version_used, expected_class
int32_t data_packet_group_decode_packet(int16_t *remaining_length, data_packet_group *group,
    void *decoded_body, uint8_t *buffer, int16_t *out_type, uint16_t *out_version_used, int16_t expected_class)
{
    uint8_t *header_byte;
    int8_t type;

    if ((uint16_t)*remaining_length < 1) {
        data_packet_group_error = (char *)"got packet with no header";
        return 0;
    }

    header_byte = buffer + (*remaining_length - 1);
    if (header_byte != 0) {
        struct_definition_byte_swap(&packet_header_byte_swap_definition, (int32_t)header_byte,
            packet_header_byte_swap_definition.codes, 0, 0);
    }
    type = (int8_t)*header_byte;

    if (type < 0 || type >= group->type_count) {
        data_packet_group_error = (char *)"got packet with bad type";
        return 0;
    }
    {
        data_packet_type *entry = &group->types[(int)type];
        if (entry->packet_class != expected_class) {
            data_packet_group_error = (char *)"got packet with mismatched class";
            return 0;
        }
        *remaining_length = *remaining_length - 1;
        if (entry->definition != 0 &&
            data_packet_group_decode_packet_body(buffer, entry->definition, *remaining_length, decoded_body,
                                                 out_version_used, 0) == 0) {
            data_packet_group_error = (char *)"got packet which wouldn't decode";
            return 0;
        }
        *out_type = (int16_t)(int8_t)*header_byte;
        data_packet_group_error = 0;
        return 1;
    }
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
