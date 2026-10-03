// data_packet_group_encode_packet_body  (Ghidra: FUN_004d0bc0)
// address 0x4d0bc0, size 175 bytes
// name confidence: 0.55 (module summary: "Encodes one packet type's payload into a packet
// group's buffer, caching the type's field-size computation on first use"; paired 1:1 with
// data_packet_group_decode_packet_body / FUN_004d0c70 by identical structure)
// rewrite confidence: 0.35 -- LOW. The control flow and the version/size-cache logic that is
// directly visible in this function's own body is preserved exactly. But TWO callee invocations
// (struct_definition_compute_size and struct_definition_encode) are shown by Ghidra with an
// EMPTY argument list, and this function's only caller (data_packet_group_encode_packet,
// 0x4d0ae0) is *itself* shown calling this one with only its 3 declared stack arguments, no
// visible setup of the ESI/EDI/AX values this function reads. That means Ghidra lost the
// argument-passing code at every hop in this chain (consistent with a hand-tuned/non-standard
// register calling convention MSVC 7.1 does not model cleanly), not just at one elided call. The
// register argument NAMES below (definition/version/version_byte_dest) are pinned down with high
// confidence from this function's own body; the additional `output` parameter needed to supply
// the elided struct_definition_encode() call is a best-effort reconstruction -- see the two
// UNSURE notes at its use.
// evidence: struct_definition::size_computed (+0x10) / ::version (+0x0a) field reads match
// types/memory.h exactly.
// register convention: version-to-encode-at in AX (EAX), struct_definition* in ESI,
// version-byte-destination pointer in EDI; stack: source struct instance pointer (param_1,
// presumably forwarded untouched into the elided struct_definition_encode() call -- it is never
// read directly in this function's visible body), out "was a version byte written" flag
// (param_2), and a capacity/enable value copied from the packet group's maximum_encoded_size by
// the caller (param_3).

#include "tags.h"
#include "memory.h"

// blam-cc: struct_definition_compute_size(definition, out_size, fields, out_field_count)
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void struct_definition_compute_size(struct_definition *definition, int16_t *out_size,
    struct_definition_field *fields, int16_t *out_field_count);
// blam-cc: struct_definition_encode(definition, output, version, source, out_source_size, fields,
// out_field_count) -- see struct_definition_encode.c.
extern void struct_definition_encode(struct_definition *definition, byte_stream *output,
    int16_t version, void *source, int16_t *out_source_size, struct_definition_field *fields,
    int16_t *out_field_count);

int32_t data_packet_group_encode_packet_body(int16_t version, struct_definition *definition,
    uint8_t *version_byte_dest, byte_stream *output, void *source, int16_t *out_wrote_version_byte,
    int16_t capacity_check)
{
    int32_t out_of_room = 0;
    int16_t wrote_version_byte = 0;

    if (definition->size_computed == 0) {
        struct_definition_compute_size(definition, 0, definition->fields, 0);
        definition->size_computed = 1;
    }

    if (version == -1) {
        version = definition->version;
    }
    if (0 < definition->version) {
        if (capacity_check < 1) {
            out_of_room = 1;
        } else {
            *version_byte_dest = (uint8_t)version;
            wrote_version_byte = 1;
        }
    }

    // UNSURE: Ghidra shows this call with an empty argument list (see file header); `output` is
    // not evidenced by this function's own body (only by struct_definition_encode's own known
    // signature) and is added here as an extra parameter so the call is well-typed. `source` and
    // `definition->fields`/NULL are the only values in scope that fit struct_definition_encode's
    // remaining parameters.
    struct_definition_encode(definition, output, version, source, 0, definition->fields, 0);

    *out_wrote_version_byte = wrote_version_byte;
    return !out_of_room;
}

#if 0
Original Ghidra decompilation (0x4d0bc0):

bool FUN_004d0bc0(undefined4 param_1,undefined2 *param_2,short param_3)

{
  bool bVar1;
  short in_AX;
  int unaff_ESI;
  undefined1 *unaff_EDI;
  undefined2 local_c;

  if (*(char *)(unaff_ESI + 0x10) == '\0') {
    struct_definition_compute_size();
    *(undefined1 *)(unaff_ESI + 0x10) = 1;
  }
  bVar1 = false;
  local_c = 0;
  if (in_AX == -1) {
    in_AX = *(short *)(unaff_ESI + 10);
  }
  if (0 < *(short *)(unaff_ESI + 10)) {
    if (param_3 < 1) {
      bVar1 = true;
    }
    else {
      *unaff_EDI = (char)in_AX;
      local_c = 1;
    }
  }
  struct_definition_encode();
  *param_2 = local_c;
  return !bVar1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
