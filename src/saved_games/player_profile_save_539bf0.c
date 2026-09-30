// player_profile_save_539bf0  (Ghidra: player_profile_save_539bf0, already named)
// address 0x539bf0, size 36 bytes
// name confidence: 0.7   rewrite confidence: 0.9
// evidence: out/phase4/saved_games_functions.md; CEA/PDB string match on "profile not saved
// since it was a default profile" (player_profile_save, interface module -- name kept as
// Ghidra's own disambiguated player_profile_save_539bf0 since a plain player_profile_save
// would collide with that interface-module function).
// register convention: handle in EAX; profile as the one stack argument.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "fn_saved_games.h"

extern void console_out_printf(uint8_t color_index, const char *format, ...); // 0x4c6860, not in this module
extern void player_profile_write_data(int32_t handle, saved_player_profile *profile); // 0x53a950

// blam-cc: handle in EAX
// Phase 4 review (objdump 0x539bf0..0x539c13): profile is the one stack argument
// (mov ecx,[esp+4] / push ecx / push eax at 0x539c05), forwarded with the EAX handle.
void player_profile_save_539bf0(int32_t handle, saved_player_profile *profile)
{
    if (handle == -1) {
        console_out_printf(0, "profile not saved since it was a default profile");
        return;
    }
    player_profile_write_data(handle, profile);
}

#if 0
Original Ghidra decompilation (0x539bf0):

void player_profile_save_539bf0(void)

{
  int in_EAX;

  if (in_EAX == -1) {
    console_out_printf('\0',"profile not saved since it was a default profile");
    return;
  }
  player_profile_write_data();
  return;
}
#endif
