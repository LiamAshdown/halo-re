// saved_game_delete_by_display_name  (Ghidra: FUN_0053b9b0, renamed)
// address 0x53b9b0, size 292 bytes
// name confidence: 0.55   rewrite confidence: 0.8
// evidence: out/phase4/saved_games_functions.md summary "Finds the saved-game whose display
// name matches a given string and deletes it, then signals completion."; out/phase4/
// saved_games_types_notes.md "0x53b9b0 wcsicmp; ... 0x53b9b0 tests bit 1" (saved_player_profile
// flags) and "0x53c960 skips XDeleteSaveGame" (handle bit 30) confirm the fields used here.
// Confirmed against objdump 0x53b9b0..0x53badc: ECX holds the narrow search name at entry; the
// enumerate call at 0x53ba2e pushes (type=0, &out_handles, builtin_only=0) and loads EBX with
// the address of a local capacity/count word pre-set to 100 (0x64); player_profile_get is
// called with the handle pushed on the stack and ECX = the out-profile buffer address;
// saved_game_delete_by_handle is called with EDI = the handle; both exits load EDI with the
// wide name buffer before calling input_apply_named_device_default_profile (matched by its own
// prologue passing EDI on to _wcsicmp).
// Phase 4 review: matched objdump 0x53b9b0..0x53bad8; player_profile_get is (index, ECX
// out_buffer), the call now passes (handle, &profile).
// register convention: search name (narrow C string) in ECX. No stack arguments.

#include "crt.h"
#include <string.h>
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

extern saved_player_profile default_profile_data; // 0x0071d280

extern void saved_game_enumerate_by_type(uint16_t type, int32_t *out_handles, uint8_t builtin_only,
    uint16_t *capacity_and_count); // 0x53c4e0, this module, blam-cc: EBX capacity_and_count
extern uint8_t player_profile_get(int32_t index, saved_player_profile *out_buffer); // 0x53a770, blam-cc: ECX out_buffer
extern uint8_t saved_game_delete_by_handle(int32_t handle); // 0x53c960, this module, blam-cc: EDI handle
extern void input_apply_named_device_default_profile(const uint16_t *name); // 0x4901b0, blam-cc: EDI name

// blam-cc: search name (narrow) in ECX
// Converts the caller's narrow search name to a bounded (0x1ff character) wide string, then
// enumerates up to 100 player-profile saved games. For each real profile whose flags have bit
// 0x2 set and whose name matches the search string (case-insensitively), deletes that saved
// game and applies the named default device profile. If nothing matches, still applies the
// named default device profile before returning.
void saved_game_delete_by_display_name(const char *name)
{
    int32_t handles[100];
    uint16_t capacity_and_count;
    uint16_t name_wide[0x200];
    int32_t length;
    int32_t i;
    saved_player_profile profile;
    uint8_t ok;
    int32_t handle;

    length = (int32_t)strlen(name);
    if ((uint32_t)(length * 2 + 2) > 0x400) {
        length = 0x1ff;
    }
    if ((uint32_t)(length * 2 + 2) < 0x401) {
        name_wide[length] = 0;
        for (i = length - 1; i >= 0; i--) {
            name_wide[i] = (uint16_t)(uint8_t)name[i];
        }
    }

    capacity_and_count = 100;
    saved_game_enumerate_by_type(0, handles, 0, &capacity_and_count); // 0x53ba21: push 0, &handles, 0; EBX=&count
    for (i = 0; i < (int32_t)capacity_and_count; i++) {
        handle = handles[i];
        if (handle == -1) {
            profile = default_profile_data;
        } else {
            ok = player_profile_get(handle, &profile);
            if (ok != 0 && (profile.flags & 0x2) != 0 && _wcsicmp((const wchar_t *)name_wide, (const wchar_t *)profile.name) == 0) {
                if (handle != -1) {
                    saved_game_delete_by_handle(handle);
                }
                input_apply_named_device_default_profile(name_wide);
                return;
            }
        }
    }
    input_apply_named_device_default_profile(name_wide);
    return;
}

#if 0
Original Ghidra decompilation (0x53b9b0):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

void FUN_0053b9b0(void)

{
  char cVar1;
  char *pcVar2;
  int iVar3;
  char *in_ECX;
  ushort uVar4;
  undefined4 *puVar5;
  undefined4 *puVar6;
  int local_2598 [100];
  wchar_t local_2408;
  ushort auStack_2406 [511];
  undefined4 local_2008;
  byte local_1eec;
  undefined4 uStack_c;

  uStack_c = 0x53b9c0;
  pcVar2 = in_ECX;
  do {
    cVar1 = *pcVar2;
    pcVar2 = pcVar2 + 1;
  } while (cVar1 != '\0');
  iVar3 = (int)pcVar2 - (int)(in_ECX + 1);
  if (0x400 < iVar3 * 2 + 2U) {
    iVar3 = 0x1ff;
  }
  if (iVar3 * 2 + 2U < 0x401) {
    auStack_2406[iVar3 + -1] = 0;
    iVar3 = iVar3 + -1;
    while (-1 < iVar3) {
      auStack_2406[iVar3 + -1] = (ushort)(byte)in_ECX[iVar3];
      iVar3 = iVar3 + -1;
    }
  }
  saved_game_enumerate_by_type(0,local_2598,0);
  uVar4 = 0;
  do {
    if (local_2598[uVar4] == -1) {
      puVar5 = &DAT_0071d280;
      puVar6 = &local_2008;
      for (iVar3 = 0x7ff; iVar3 != 0; iVar3 = iVar3 + -1) {
        *puVar6 = *puVar5;
        puVar5 = puVar5 + 1;
        puVar6 = puVar6 + 1;
      }
    }
    else {
      cVar1 = player_profile_get(local_2598[uVar4]);
      if (((cVar1 != '\0') && ((local_1eec & 2) != 0)) &&
         (iVar3 = __wcsicmp(&local_2408,(wchar_t *)((int)&local_2008 + 2)), iVar3 == 0)) {
        if (local_2598[uVar4] != -1) {
          saved_game_delete_by_handle();
        }
        input_apply_named_device_default_profile();
        return;
      }
    }
    uVar4 = uVar4 + 1;
    if (99 < uVar4) {
      input_apply_named_device_default_profile();
      return;
    }
  } while( true );
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
