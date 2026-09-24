// control_profile_variant_write_wait_and_clear  (Ghidra: FUN_0053bae0, renamed)
// address 0x53bae0, size 94 bytes
// name confidence: 0.5   rewrite confidence: 0.8
// evidence: out/phase4/saved_games_functions.md summary "Waits for the background
// profile-rename worker thread to finish and clears its shared parameter block."; corrected by
// out/phase4/saved_games_types_notes.md "Misattributed / misnamed functions" note: this waits on
// the asynchronous *game variant* writer thread (network_thread_record *variant_write_thread,
// 0x00721324), not a profile rename. The zeroed 0x29-dword block at 0x00721288 covers
// variant_write_request (0x9c bytes) plus variant_write_thread and the low word of
// default_game_variant_count right after it (types/saved_games.h "global 0x00721288" /
// "0x00721324" / "0x00721328").
// register convention: no arguments; operates entirely on module globals.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"

extern network_thread_record *variant_write_thread; // 0x00721324
extern variant_write_request variant_write_request_state; // 0x00721288 (0x29-dword zeroed block
                                                            // reaches through variant_write_thread
                                                            // and the low word of default_game_variant_count)

extern int32_t GetExitCodeThread(void *thread, uint32_t *exit_code); // Win32
extern int32_t CloseHandle(void *object); // Win32

// blam-cc: no arguments
// Blocks until the asynchronous game-variant writer thread (if any) has exited, closes its
// handle and clears its thread-table record, then zeroes the shared variant-write parameter
// block (and the thread pointer / default-count word immediately after it).
void control_profile_variant_write_wait_and_clear(void)
{
    uint32_t exit_code;
    uint8_t *zero_cursor;
    int32_t i;
    int32_t got_code;

    if (variant_write_thread != 0) {
        do {
            do {
                got_code = GetExitCodeThread(variant_write_thread->handle, &exit_code);
            } while (got_code == 0);
        } while (exit_code == 0x103);
        CloseHandle(variant_write_thread->handle);
        variant_write_thread->handle = 0;
        variant_write_thread->in_use = 0;
    }
    zero_cursor = (uint8_t *)&variant_write_request_state;
    for (i = 0x29; i != 0; i--) {
        *(uint32_t *)zero_cursor = 0;
        zero_cursor += 4;
    }
    return;
}

#if 0
Original Ghidra decompilation (0x53bae0):

void FUN_0053bae0(void)

{
  BOOL BVar1;
  int iVar2;
  undefined4 *puVar3;
  DWORD local_4;

  if (DAT_00721324 != (undefined4 *)0x0) {
    do {
      do {
        BVar1 = GetExitCodeThread((HANDLE)*DAT_00721324,&local_4);
        puVar3 = DAT_00721324;
      } while (BVar1 == 0);
    } while (local_4 == 0x103);
    CloseHandle((HANDLE)*DAT_00721324);
    *puVar3 = 0;
    *(undefined1 *)(puVar3 + 1) = 0;
  }
  puVar3 = &DAT_00721288;
  for (iVar2 = 0x29; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar3 = 0;
    puVar3 = puVar3 + 1;
  }
  return;
}
#endif
