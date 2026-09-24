// data_file_read_header  (Ghidra: data_file_read_header, already named)
// address 0x443b30, size 100 bytes
// name confidence: 0.8   rewrite confidence: 0.7
// evidence: out/phase4/cache_types_notes.md data_file section: "data_file_read_header @0x443b30
// does ReadFile(handle, base, 0x10, ...) and then if (*base != expected_id)"; field offsets
// (file 0x3c == index 0xf, name 0x38 == index 0xe) match types/cache.h data_file exactly.
// register convention: data_file pointer in ESI (unaff_ESI); expected_file_id is the recognized
// stack parameter (param_1).
// UNSURE: the return value packs a 1-byte bool with undefined high bits (Ghidra's
// CONCAT31/`& 0xffffff00` patterns); only the low byte is meaningful, so it is returned as a
// plain 0/1 int, matching the house convention in src/memory/bit_stream_write_bit.c.

#include "tags.h"
#include "cache.h"

extern int32_t printf(const char *format, ...); // 0x62427c _printf
extern int32_t ReadFile(void *file, void *buffer, uint32_t bytes_to_read, uint32_t *bytes_read, void *overlapped); // 0x0063a2d8 IAT

// blam-cc: data_file pointer in ESI (unaff_ESI), expected_file_id as the recognized stack parameter
// Reads and validates the 16-byte on-disk header of an opened data file (bitmaps.map/sounds.map)
// directly into the data_file object (file_id, data_offset, table_offset, entry_count) and
// checks file_id against expected_file_id. Zeroes those four fields and prints a diagnostic on a
// short/failed read or a file-id mismatch.
int32_t data_file_read_header(data_file *file, int32_t expected_file_id)
{
    uint32_t bytes_read;

    if (ReadFile(file->file, file, 0x10, &bytes_read, (void *)0) != 0 && bytes_read == 0x10) {
        if (file->file_id != expected_file_id) {
            file->file_id = 0;
            file->data_offset = 0;
            file->table_offset = 0;
            file->entry_count = 0;
            printf("Invalid data file id in data file %s\n", file->name);
            return 0;
        }
        return 1;
    }
    printf("Failed to read data file header %s\n", file->name);
    return 0;
}

#if 0
Original Ghidra decompilation (0x443b30):

uint data_file_read_header(int param_1)

{
  BOOL BVar1;
  uint uVar2;
  int *unaff_ESI;
  DWORD local_4;

  BVar1 = ReadFile((HANDLE)unaff_ESI[0xf],unaff_ESI,0x10,&local_4,(LPOVERLAPPED)0x0);
  if ((BVar1 != 0) && (local_4 == 0x10)) {
    if (*unaff_ESI != param_1) {
      *unaff_ESI = 0;
      unaff_ESI[1] = 0;
      unaff_ESI[2] = 0;
      unaff_ESI[3] = 0;
      uVar2 = _printf("Invalid data file id in data file %s\n",unaff_ESI[0xe]);
      return uVar2 & 0xffffff00;
    }
    return CONCAT31((int3)((uint)*unaff_ESI >> 8),1);
  }
  uVar2 = _printf("Failed to read data file header %s\n",unaff_ESI[0xe]);
  return uVar2 & 0xffffff00;
}
#endif
