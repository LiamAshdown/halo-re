// file_reference_exists  (Ghidra: file_reference_exists, already named)
// address 0x555720, size 114 bytes
// name confidence: 0.8   rewrite confidence: 0.8
// evidence: already named by Ghidra/CEA. Confirmed against objdump 0x555720..0x555770: `mov
// esi,eax` at entry copies the reference record, matching file_reference_create's identical
// prologue shape.
// register convention: reference record in EAX.
// blam-cc: EAX -> ref
// FIXED (register inputs, objdump): this file had no parseable blam-cc note (prose only); added
// "EAX -> ref" (read live at entry, 0x55572e `mov esi,eax`).

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"

extern void path_build_full(char *source, char *destination, int16_t location); // 0x5560d0, this module
extern void saved_games_report_last_error(void); // 0x556170, this module


// blam-cc: EAX -> ref
// Returns 1 if ref's full path currently exists on disk, 0 otherwise. Reports the Win32 error
// unless it is ERROR_FILE_NOT_FOUND (2) or ERROR_PATH_NOT_FOUND (3).
uint8_t file_reference_exists(file_reference_record *ref)
{
    char full_path[0x100];
    uint32_t attributes;
    uint32_t error;

    path_build_full(ref->path, full_path, ref->location);
    attributes = GetFileAttributesA(full_path);
    if (attributes != 0xffffffff) {
        return 1;
    }
    error = GetLastError();
    if (error != 2) {
        error = GetLastError();
        if (error != 3) {
            saved_games_report_last_error();
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x555720):

undefined4 file_reference_exists(void)

{
  DWORD DVar1;
  int iVar2;
  undefined4 *puVar3;
  CHAR local_108;
  undefined4 local_107;

  local_108 = '\0';
  puVar3 = &local_107;
  for (iVar2 = 0x3f; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar3 = 0;
    puVar3 = puVar3 + 1;
  }
  *(undefined2 *)puVar3 = 0;
  *(undefined1 *)((int)puVar3 + 2) = 0;
  FUN_005560d0();
  DVar1 = GetFileAttributesA(&local_108);
  if (DVar1 != 0xffffffff) {
    return 1;
  }
  DVar1 = GetLastError();
  if (DVar1 != 2) {
    DVar1 = GetLastError();
    if (DVar1 != 3) {
      FUN_00556170();
    }
  }
  return 0;
}
#endif
