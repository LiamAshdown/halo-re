// data_packet_group_encode_packet
// address 0x4d0ae0, size 125 bytes
// name confidence: 0.85 (cea-pdb string match: "couldn't encode packet")
// rewrite confidence: 0.35 -- LOW, for the same reason as data_packet_group_encode_packet_body.c:
// this function calls both data_packet_group_encode_packet_body and
// data_packet_group_append_packet_header with far fewer visible arguments than those functions'
// own bodies require (definition/version/version_byte_dest/output for the first; buffer/cursor
// for the second). None of the missing values are evidenced anywhere in this function's own
// decompiled body -- Ghidra shows zero setup code for them, meaning the values were already
// resident in the expected registers by whatever this function's own (also non-standard) calling
// convention is. They are added here as explicit UNSURE parameters so the three files type-check
// and reflect the true data dependency; the *values* actually used at the real call sites in
// halo.exe cannot be recovered from the available decompilation.
// evidence: `*(undefined2 *)(unaff_EBX + 0xc)` matches data_packet_group::maximum_encoded_size
// (types/memory.h, +0x0c) exactly, which is the only concrete struct-offset evidence in this
// function.
// register convention: data_packet_group* in EBX (unaff_EBX, read but never locally assigned);
// stack: source struct instance pointer (param_1, forwarded to encode_packet_body), out "wrote a
// version byte" flag pointer (param_2, forwarded), packet type / header byte value (param_3,
// forwarded to append_packet_header).

#include "tags.h"
#include "memory.h"

extern char *data_packet_group_error; // 0x006b7f00

// blam-cc (0x4d0bc0): see data_packet_group_encode_packet_body.c for the full parameter
// reconstruction and its UNSURE notes.
extern int32_t data_packet_group_encode_packet_body(int16_t version, struct_definition *definition,
    uint8_t *version_byte_dest, byte_stream *output, void *source, int16_t *out_wrote_version_byte,
    int16_t capacity_check);
// blam-cc (0x4d0b60): see data_packet_group_append_packet_header.c.
extern int32_t data_packet_group_append_packet_header(uint8_t *buffer, data_packet_group *group,
    int16_t *cursor, uint8_t header_byte);

int32_t data_packet_group_encode_packet(int16_t version, struct_definition *definition,
    uint8_t *version_byte_dest, byte_stream *output, uint8_t *buffer, int16_t *cursor,
    data_packet_group *group, void *source, int16_t *out_wrote_version_byte, uint8_t packet_type)
{
    char *error = 0;
    int32_t ok;

    ok = data_packet_group_encode_packet_body(version, definition, version_byte_dest, output,
        source, out_wrote_version_byte, (int16_t)group->maximum_encoded_size);
    if (ok == 0) {
        error = "couldn't encode packet";
    } else {
        ok = data_packet_group_append_packet_header(buffer, group, cursor, packet_type);
        if (ok == 0) {
            return data_packet_group_error == 0;
        }
    }
    data_packet_group_error = error;
    return error == 0;
}

#if 0
Original Ghidra decompilation (0x4d0ae0):

bool data_packet_group_encode_packet(undefined4 param_1,undefined4 param_2,undefined4 param_3)

{
  char cVar1;
  int unaff_EBX;
  char *pcVar2;

  pcVar2 = (char *)0x0;
  cVar1 = FUN_004d0bc0(param_1,param_2,*(undefined2 *)(unaff_EBX + 0xc));
  if (cVar1 == '\0') {
    pcVar2 = "couldn\'t encode packet";
  }
  else {
    cVar1 = data_packet_group_append_packet_header(param_3);
    if (cVar1 == '\0') {
      return DAT_006b7f00 == (char *)0x0;
    }
  }
  DAT_006b7f00 = pcVar2;
  return pcVar2 == (char *)0x0;
}
#endif
