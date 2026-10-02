// game_variant_write_request_start  (Ghidra: FUN_0053c0b0, renamed)
// address 0x53c0b0, size 141 bytes
// name confidence: 0.4   rewrite confidence: 0.6
// evidence: out/phase4/saved_games_functions.md summary "Kicks off a background worker thread
// (FUN_0053c150) to apply a pending player-profile id/name change asynchronously," corrected by
// out/phase4/saved_games_types_notes.md "Misattributed / misnamed functions": this is the
// asynchronous *game variant* writer, not a profile rename -- it fills
// variant_write_request_state (types/saved_games.h variant_write_request, 0x00721288) with the
// handle and game_variant Ghidra shows both parameters fully resolved for, then starts
// game_variant_write_thread_proc (0x53c150) via network_thread_create, storing the new thread in
// variant_write_thread (0x00721324). Waits for and clears any still-running previous writer
// first, exactly like saved_game_get_variant.c (0x53bee0) and
// control_profile_variant_write_wait_and_clear.c (0x53bae0).
// register convention: __cdecl, plain stack arguments (handle, variant).

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern network_thread_record *variant_write_thread; // 0x00721324
extern variant_write_request variant_write_request_state; // 0x00721288

extern uint32_t game_variant_write_thread_proc(variant_write_request *request); // 0x53c150, this module
extern int32_t network_thread_create(uint8_t flags, void *start_address, void *parameter,
    network_thread_record **out_handle); // 0x440460

// blam-cc: __cdecl, plain stack arguments (handle, variant)
// Waits for and clears any in-flight game-variant writer thread, fills
// variant_write_request_state.handle/variant from the arguments, then starts
// game_variant_write_thread_proc as a new thread over it.
void game_variant_write_request_start(int32_t handle, game_variant *variant)
{
    uint32_t exit_code;

    if (variant_write_thread != 0) {
        do {
            do {
            } while (GetExitCodeThread(variant_write_thread->handle, (LPDWORD)&exit_code) == 0);
        } while (exit_code == 0x103);
        CloseHandle(variant_write_thread->handle);
        variant_write_thread->handle = 0;
        variant_write_thread->in_use = 0;
        variant_write_thread = 0;
    }
    variant_write_request_state.handle = handle;
    variant_write_request_state.variant = *variant;
    network_thread_create(0, (void *)game_variant_write_thread_proc, &variant_write_request_state,
        &variant_write_thread);
}

#if 0
Original Ghidra decompilation (0x53c0b0):

void FUN_0053c0b0(undefined4 param_1,undefined4 *param_2)

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
    DAT_00721324 = (undefined4 *)0x0;
  }
  DAT_00721288 = param_1;
  puVar3 = &DAT_0072128c;
  for (iVar2 = 0x26; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar3 = *param_2;
    param_2 = param_2 + 1;
    puVar3 = puVar3 + 1;
  }
  network_thread_create(0,FUN_0053c150,&DAT_00721288,&DAT_00721324);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
