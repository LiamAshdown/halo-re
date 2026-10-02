// crc32_update  (Ghidra: crc32_update, already named)
// address 0x4d02d0, size 81 bytes
// name confidence: 0.85   rewrite confidence: 0.9
// evidence: out/phase4/memory_types_notes.md "crc32"; globals crc32_lookup_table_initialized
// (0x00719cd8) and crc32_lookup_table (0x006b7b00), callee crc32_build_table.
// register convention: none -- all three arguments are Ghidra-recognized stack parameters.

#include "tags.h"
#include "memory.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern crc32_table crc32_lookup_table;                  // 0x006b7b00
extern uint8_t crc32_lookup_table_initialized;           // 0x00719cd8

extern void crc32_build_table(crc32_table *table); // blam-cc: table in EDX

// Computes/continues a CRC-32 checksum over `length` bytes of `data`, folding into *crc.
// Builds the CRC-32 lookup table on first use.
void crc32_update(uint32_t *crc, uint8_t *data, int32_t length)
{
    uint32_t value;

    if (crc32_lookup_table_initialized == 0) {
        crc32_build_table(&crc32_lookup_table);
        crc32_lookup_table_initialized = 1;
    }
    value = *crc;
    if (0 < length) {
        do {
            value = (value >> 8) ^ crc32_lookup_table.entries[(*data ^ value) & 0xff];
            data = data + 1;
            length = length - 1;
        } while (length != 0);
    }
    *crc = value;
}

#if 0
Original Ghidra decompilation (0x4d02d0):

void crc32_update(uint *param_1,byte *param_2,int param_3)

{
  byte bVar1;
  uint uVar2;

  if (DAT_00719cd8 == '\0') {
    crc32_build_table();
    DAT_00719cd8 = '\x01';
  }
  uVar2 = *param_1;
  if (0 < param_3) {
    do {
      bVar1 = *param_2;
      param_2 = param_2 + 1;
      uVar2 = uVar2 >> 8 ^ (&DAT_006b7b00)[(bVar1 ^ uVar2) & 0xff];
      param_3 = param_3 + -1;
    } while (param_3 != 0);
  }
  *param_1 = uVar2;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
