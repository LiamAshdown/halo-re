// network_game_start_new_server_from_profile  (Ghidra: already named)
// address 0x4e40f0, size 88 bytes
// name confidence: 0.5   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md ("Prepares a default server-name/password
// profile on the stack ... and starts a new hosted game using it"); types/networking.h's
// network_client_begin_connect_scratch note explicitly documents this exact 0x7ff-dword
// memcpy as "overrun the array as the binary has it" -- the copy fills one contiguous 2047-dword
// (0x1ffc-byte) buffer whose tail 867*4..867*4+0x11f bytes become the name and password
// arguments network_game_start_new_server_with_name_and_password.c (this batch) actually reads;
// everything else copied is unused by this call path.
// register convention: __cdecl, one recognized parameter (param_1, forwarded unchanged).
// UNSURE: FUN_0053a150's exact role (foreign, builds a default profile directly into the
// destination buffer when no cached template exists yet); UNSURE: the split point between the
// "name" and "password" regions is inferred from network_game_start_new_server_with_name_and_password's
// own reads (wcsncpy of 0x3f chars, then of 8 chars) rather than independently confirmed here.

#include "tags.h"
#include "memory.h"
#include <string.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int32_t saved_player_profile_slots_handle; // 0x00714dd4, -1 == not yet built
extern uint32_t profile_globals_block[0x7ff]; // 0x00712dd8

extern void player_profile_set_default_server_options(void *dest); // foreign, builds a default profile in place, UNSURE shape
extern void network_game_start_new_server_with_name_and_password(uint32_t unused,
    uint16_t *name, uint16_t *password); // this batch, 0x4e4150

// Builds (or copies the cached) default server profile into one 0x1ffc-byte scratch buffer, then
// starts a new hosted game using the name/password fields near its tail.
void network_game_start_new_server_from_profile(uint32_t param_1)
{
    uint32_t profile[0x7ff];

    if (saved_player_profile_slots_handle == -1) {
        player_profile_set_default_server_options(profile);
    } else {
        memcpy(profile, profile_globals_block, sizeof(profile));
    }
    network_game_start_new_server_with_name_and_password(param_1,
        (uint16_t *)((uint8_t *)profile + 867 * 4),
        (uint16_t *)((uint8_t *)profile + 867 * 4 + 288));
}

#if 0
Original Ghidra decompilation (0x4e40f0), from tools/pack.py 0x4e40f0:

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

void network_game_start_new_server_from_profile(undefined4 param_1)

{
  int iVar1;
  undefined4 *puVar2;
  undefined4 *puVar3;
  undefined4 local_2008 [867];
  undefined1 local_127c [288];
  undefined1 local_115c [4432];
  undefined4 uStack_c;

  uStack_c = 0x4e4100;
  if (DAT_00714dd4 == -1) {
    FUN_0053a150();
  }
  else {
    puVar2 = &DAT_00712dd8;
    puVar3 = local_2008;
    for (iVar1 = 0x7ff; iVar1 != 0; iVar1 = iVar1 + -1) {
      *puVar3 = *puVar2;
      puVar2 = puVar2 + 1;
      puVar3 = puVar3 + 1;
    }
  }
  network_game_start_new_server_with_name_and_password(param_1,local_127c,local_115c);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
