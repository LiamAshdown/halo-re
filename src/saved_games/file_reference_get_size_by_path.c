// file_reference_get_size_by_path  (Ghidra: FUN_00555b00, renamed)
// address 0x555b00, size 139 bytes
// name confidence: 0.5   rewrite confidence: 0.75
// evidence: out/phase4/saved_games_functions.md summary "Retrieves a file's size on disk by
// path (without opening it) via GetFileAttributesExA." Confirmed against objdump
// 0x555b00..0x555b45: ESI (no assignment in the prologue) feeds path_build_full exactly like
// file_reference_delete/_open, and the one stack argument ([ebp+8]) is the out-size pointer.
// register convention: reference record in ESI; out-size pointer as the one stack argument.
//   // blam-cc: ESI -> ref, stack -> out_size
// FIXED (register inputs, objdump): ESI (the reference record, already a C parameter) was read
//   at 0x555b21 but missing from the machine-checked "blam-cc" annotation; reworded so the
//   checker recognizes it.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "fn_saved_games.h"


// blam-cc: ESI -> ref, stack -> out_size
// Retrieves ref's full-path file size (low dword only) without opening it. Returns 1 on
// success (storing the size through out_size), 0 on failure (after reporting the Win32 error).
uint8_t file_reference_get_size_by_path(file_reference_record *ref, uint32_t *out_size)
{
    char full_path[0x800];
    win32_file_attribute_data attributes;
    int32_t ok;

    path_build_full(ref->path, full_path, ref->location);
    ok = GetFileAttributesExA(full_path, 0 /* GetFileExInfoStandard */, &attributes);
    if (ok != 0) {
        *out_size = attributes.file_size_low;
        return 1;
    }
    saved_games_report_last_error();
    return 0;
}

#if 0
Original Ghidra decompilation (0x555b00):

undefined4 FUN_00555b00(undefined4 *param_1)

{
  BOOL BVar1;
  DWORD dwMessageId;
  int iVar2;
  undefined4 *puVar3;
  undefined1 local_92c [32];
  undefined4 local_90c;
  CHAR local_908;
  undefined4 local_907;
  CHAR local_808 [2052];

  local_908 = '\0';
  puVar3 = &local_907;
  for (iVar2 = 0x3f; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar3 = 0;
    puVar3 = puVar3 + 1;
  }
  *(undefined2 *)puVar3 = 0;
  *(undefined1 *)((int)puVar3 + 2) = 0;
  FUN_005560d0();
  BVar1 = GetFileAttributesExA(&local_908,GetFileExInfoStandard,local_92c);
  if (BVar1 != 0) {
    *param_1 = local_90c;
    return 1;
  }
  dwMessageId = GetLastError();
  FormatMessageA(0x12ff,(LPCVOID)0x0,dwMessageId,0,local_808,0x800,(va_list *)0x0);
  SetLastError(0);
  return 0;
}
#endif
