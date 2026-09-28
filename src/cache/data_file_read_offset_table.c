// data_file_read_offset_table  (Ghidra: data_file_read_offset_table, already named)
// address 0x443c20, size 118 bytes
// name confidence: 0.8   rewrite confidence: 0.75
// evidence: out/phase4/cache_types_notes.md data_file section: seeks to +0x08, allocates
// [0x0c]*0xc into +0x10, copies [0x0c] to +0x14; the *0xc is the only evidence for
// data_file_reference's 0x0c stride. Matches types/cache.h data_file (table_offset 0x08,
// entry_count 0x0c, references 0x10, reference_count 0x14, file 0x3c, name 0x38).
// register convention: data_file pointer in EDI (unaff_EDI).

#include "win32.h"
#include "tags.h"
#include "cache.h"

extern int32_t printf(const char *format, ...); // 0x62427c _printf

// blam-cc: data_file pointer in EDI (unaff_EDI)
// Reads a data file's array of 12-byte offset/size reference entries (one per external bitmap
// or sound), starting at table_offset, into a newly allocated table sized by entry_count.
// Prints a diagnostic and returns 0 on any seek/read failure or short read.
uint8_t data_file_read_offset_table(data_file *file)
{
    uint32_t table_size;
    void *buffer;
    uint32_t bytes_read;

    if (SetFilePointer(file->file, file->table_offset, (void *)0, 0) != 0xffffffff) {
        table_size = file->entry_count * 0xc;
        buffer = GlobalAlloc(0, table_size);
        file->references = (data_file_reference *)buffer;
        if (ReadFile(file->file, buffer, table_size, &bytes_read, (void *)0) != 0 && bytes_read == table_size) {
            file->reference_count = file->entry_count;
            return 1;
        }
    }
    printf("Invalid format in data file %s\n", file->name);
    return 0;
}

#if 0
Original Ghidra decompilation (0x443c20):

undefined1 data_file_read_offset_table(void)

{
  DWORD DVar1;
  HGLOBAL lpBuffer;
  BOOL BVar2;
  SIZE_T dwBytes;
  int unaff_EDI;
  SIZE_T local_4;

  DVar1 = SetFilePointer(*(HANDLE *)(unaff_EDI + 0x3c),*(LONG *)(unaff_EDI + 8),(PLONG)0x0,0);
  if (DVar1 != 0xffffffff) {
    dwBytes = *(int *)(unaff_EDI + 0xc) * 0xc;
    lpBuffer = GlobalAlloc(0,dwBytes);
    *(HGLOBAL *)(unaff_EDI + 0x10) = lpBuffer;
    BVar2 = ReadFile(*(HANDLE *)(unaff_EDI + 0x3c),lpBuffer,dwBytes,&local_4,(LPOVERLAPPED)0x0);
    if ((BVar2 != 0) && (local_4 == dwBytes)) {
      *(undefined4 *)(unaff_EDI + 0x14) = *(undefined4 *)(unaff_EDI + 0xc);
      return 1;
    }
  }
  _printf("Invalid format in data file %s\n",*(undefined4 *)(unaff_EDI + 0x38));
  return 0;
}
#endif
