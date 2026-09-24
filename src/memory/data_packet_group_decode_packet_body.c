// data_packet_group_decode_packet_body  (Ghidra: FUN_004d0c70)
// address 0x4d0c70, size 210 bytes
// name confidence: 0.55 (module summary: "Decodes one packet type's payload from a packet
// group's input buffer, caching the type's field-size computation on first use"; mirrors
// data_packet_group_encode_packet_body exactly, one field at a time)
// rewrite confidence: 0.35 -- LOW, for the same reason as data_packet_group_encode_packet_body.c:
// the elided struct_definition_decode() call needs an input byte_stream* that is not evidenced
// anywhere in this function's own body (only by the callee's own known signature). Added as an
// explicit UNSURE parameter. All other logic is directly visible and preserved exactly.
// evidence: struct_definition::size_computed (+0x10) / ::version (+0x0a) field reads match
// types/memory.h; parameter roles cross-checked against data_packet_group_encode_packet_body.c.
// register convention: version-byte source pointer in EAX (may be NULL for a dry run), struct
// definition* in ESI; stack: remaining input byte count (param_1), decode destination struct
// instance (param_2, forwarded to the elided struct_definition_decode() call -- never read
// directly in this function's visible body, matching encode_packet_body's param_1), out "version
// value actually used" pointer (param_3), out "did we attempt to consume a version byte" flag
// pointer (param_4).

#include "tags.h"
#include "memory.h"

// blam-cc: struct_definition_compute_size(definition, out_size, fields, out_field_count)
extern void struct_definition_compute_size(struct_definition *definition, int16_t *out_size,
    struct_definition_field *fields, int16_t *out_field_count);
// blam-cc: struct_definition_decode(definition, input, version, dest, out_dest_size, fields,
// out_field_count) -- see struct_definition_decode.c.
extern void struct_definition_decode(struct_definition *definition, byte_stream *input,
    int16_t version, void *dest, int16_t *out_dest_size, struct_definition_field *fields,
    int16_t *out_field_count);

int32_t data_packet_group_decode_packet_body(uint8_t *version_byte_src,
    struct_definition *definition, int16_t remaining_length, void *dest, byte_stream *input,
    uint16_t *out_version_used, int16_t *out_attempted_version_byte)
{
    int32_t decoded = 0;
    int32_t out_of_room;
    uint8_t version_byte;
    uint16_t version_used;
    int16_t attempted;

    if (definition->size_computed == 0) {
        struct_definition_compute_size(definition, 0, definition->fields, 0);
        definition->size_computed = 1;
    }

    out_of_room = 0;
    attempted = 0;
    if (definition->version == 0) {
        version_used = 0;
    } else {
        if (remaining_length < 1) {
            out_of_room = 1;
            attempted = 0;
            version_byte = 0;
        } else {
            attempted = 1;
            out_of_room = 0;
            if (version_byte_src == 0) {
                version_byte = 0; // UNSURE: Ghidra's own literal behavior when the source pointer
                    // is NULL despite remaining_length saying a byte is available; preserved, not
                    // "fixed" -- see the equivalent note in struct_definition_encode's case 6.
            } else {
                version_byte = *version_byte_src;
            }
        }
        version_used = (uint16_t)version_byte;
    }

    if ((int16_t)version_used <= definition->version) {
        // UNSURE: `input` is not evidenced by this function's own body; see the file header.
        struct_definition_decode(definition, input, (int16_t)version_used, dest, 0,
            definition->fields, 0);
        if (!out_of_room) {
            decoded = 1;
        }
    }

    if (out_version_used != 0) {
        *out_version_used = version_used;
    }
    if (out_attempted_version_byte != 0) {
        *out_attempted_version_byte = attempted;
    }
    return decoded;
}

#if 0
Original Ghidra decompilation (0x4d0c70):

undefined1 FUN_004d0c70(short param_1,undefined4 param_2,ushort *param_3,undefined2 *param_4)

{
  bool bVar1;
  byte bVar2;
  byte *in_EAX;
  undefined1 uVar3;
  int unaff_ESI;
  ushort uVar4;
  undefined2 local_c;

  uVar3 = 0;
  if (*(char *)(unaff_ESI + 0x10) == '\0') {
    struct_definition_compute_size();
    *(undefined1 *)(unaff_ESI + 0x10) = 1;
  }
  bVar1 = false;
  local_c = 0;
  if (*(short *)(unaff_ESI + 10) == 0) {
    uVar4 = 0;
    goto LAB_004d0cf3;
  }
  if (param_1 < 1) {
    bVar1 = true;
    local_c = 0;
LAB_004d0ced:
    bVar2 = 0;
  }
  else {
    local_c = 1;
    bVar1 = false;
    if (in_EAX == (byte *)0x0) goto LAB_004d0ced;
    bVar2 = *in_EAX;
  }
  uVar4 = (ushort)bVar2;
LAB_004d0cf3:
  if ((short)uVar4 <= *(short *)(unaff_ESI + 10)) {
    struct_definition_decode();
    if (!bVar1) {
      uVar3 = 1;
    }
  }
  if (param_3 != (ushort *)0x0) {
    *param_3 = uVar4;
  }
  if (param_4 != (undefined2 *)0x0) {
    *param_4 = local_c;
  }
  return uVar3;
}
#endif
