// player_profile_verify_thread_wait_and_clear  (Ghidra: FUN_00539a40, renamed)
// address 0x539a40, size 94 bytes
// name confidence: 0.4   rewrite confidence: 0.7
// evidence: out/phase4/saved_games_functions.md summary "Waits for a background profile-
// verification thread to finish, then clears the cached default-profile buffer it produced."
// types/saved_games.h globals: 0x0072127c player_profile_thread (network_thread_record*),
// 0x0071d280 default_profile_data, 0x0071f27c unknown_0071f27c[0x2000] ("zeroed with the
// default profile (0x1001 dwords from 0x0071d280 in FUN_00539a40 / init)"). Named to parallel
// the sibling s2 rewrite's control_profile_variant_write_wait_and_clear (0x53bae0).
// register convention: no parameters, no return value.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "fn_saved_games.h"

extern network_thread_record *player_profile_thread; // 0x0072127c
extern saved_player_profile default_profile_data; // 0x0071d280


void player_profile_verify_thread_wait_and_clear(void)
{
    uint32_t exit_code;
    uint32_t *clear;
    int32_t i;

    if (player_profile_thread != 0) {
        do {
            while (GetExitCodeThread(player_profile_thread->handle, &exit_code) == 0) {
                /* keep polling */
            }
        } while (exit_code == 0x103 /* STILL_ACTIVE */);
        CloseHandle(player_profile_thread->handle);
        player_profile_thread->handle = 0;
        player_profile_thread->in_use = 0;
    }

    clear = (uint32_t *)&default_profile_data;
    for (i = 0x1001; i != 0; i = i - 1) {
        *clear = 0;
        clear = clear + 1;
    }
}

#if 0
Original Ghidra decompilation (0x539a40):

void FUN_00539a40(void)

{
  BOOL BVar1;
  int iVar2;
  undefined4 *puVar3;
  DWORD local_4;

  if (DAT_0072127c != (undefined4 *)0x0) {
    do {
      do {
        BVar1 = GetExitCodeThread((HANDLE)*DAT_0072127c,&local_4);
        puVar3 = DAT_0072127c;
      } while (BVar1 == 0);
    } while (local_4 == 0x103);
    CloseHandle((HANDLE)*DAT_0072127c);
    *puVar3 = 0;
    *(undefined1 *)(puVar3 + 1) = 0;
  }
  puVar3 = &DAT_0071d280;
  for (iVar2 = 0x1001; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar3 = 0;
    puVar3 = puVar3 + 1;
  }
  return;
}
#endif
