// data_file_read_data_block  (Ghidra: data_file_read_data_block, already named)
// address 0x443ba0, size 115 bytes
// name confidence: 0.8   rewrite confidence: 0.75
// evidence: out/phase4/cache_types_notes.md data_file section: seeks to +0x04, allocates
// [0x08]-[0x04] bytes into +0x20, writes that size to both +0x1c and +0x18 on success. Matches
// types/cache.h data_file exactly (data_offset 0x04, table_offset 0x08, data 0x20,
// data_capacity 0x1c, data_size 0x18, file 0x3c, name 0x38).
// register convention: data_file pointer in ESI (unaff_ESI).

#include "tags.h"
#include "cache.h"

extern int32_t printf(const char *format, ...); // 0x62427c _printf
extern void *__stdcall GlobalAlloc(uint32_t flags, uint32_t bytes); // 0x0063a0b0 IAT
extern uint32_t __stdcall SetFilePointer(void *file, int32_t distance, void *distance_high, uint32_t method); // 0x0063a2b0 IAT
extern int32_t __stdcall ReadFile(void *file, void *buffer, uint32_t bytes_to_read, uint32_t *bytes_read, void *overlapped); // 0x0063a2d8 IAT

// blam-cc: data_file pointer in ESI (unaff_ESI)
// Reads a data file's raw payload block (bitmap/sound data, from data_offset up to
// table_offset) into a newly allocated buffer, storing it and its size on the data_file. Prints
// a diagnostic and returns 0 on any seek/read failure or short read.
uint8_t data_file_read_data_block(data_file *file)
{
    uint32_t block_size;
    void *buffer;
    uint32_t bytes_read;

    if (SetFilePointer(file->file, file->data_offset, (void *)0, 0) != 0xffffffff) {
        block_size = file->table_offset - file->data_offset;
        buffer = GlobalAlloc(0, block_size);
        file->data = buffer;
        if (ReadFile(file->file, buffer, block_size, &bytes_read, (void *)0) != 0 && bytes_read == block_size) {
            file->data_capacity = block_size;
            file->data_size = block_size;
            return 1;
        }
    }
    printf("Invalid format in data file %s\n", file->name);
    return 0;
}

#if 0
Original Ghidra decompilation (0x443ba0):

undefined1 data_file_read_data_block(void)

{
  DWORD DVar1;
  HGLOBAL lpBuffer;
  BOOL BVar2;
  int unaff_ESI;
  SIZE_T dwBytes;
  SIZE_T local_4;

  DVar1 = SetFilePointer(*(HANDLE *)(unaff_ESI + 0x3c),*(LONG *)(unaff_ESI + 4),(PLONG)0x0,0);
  if (DVar1 != 0xffffffff) {
    dwBytes = *(int *)(unaff_ESI + 8) - *(int *)(unaff_ESI + 4);
    lpBuffer = GlobalAlloc(0,dwBytes);
    *(HGLOBAL *)(unaff_ESI + 0x20) = lpBuffer;
    BVar2 = ReadFile(*(HANDLE *)(unaff_ESI + 0x3c),lpBuffer,dwBytes,&local_4,(LPOVERLAPPED)0x0);
    if ((BVar2 != 0) && (local_4 == dwBytes)) {
      *(SIZE_T *)(unaff_ESI + 0x1c) = dwBytes;
      *(SIZE_T *)(unaff_ESI + 0x18) = dwBytes;
      return 1;
    }
  }
  _printf("Invalid format in data file %s\n",*(undefined4 *)(unaff_ESI + 0x38));
  return 0;
}
#endif
