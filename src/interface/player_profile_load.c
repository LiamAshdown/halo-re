// player_profile_load  (Ghidra: player_profile_load, already named)
// address 0x495970, size 238 bytes
// name confidence: 0.55   rewrite confidence: 0.65
// evidence: out/phase4/interface_functions.md "Loads a player profile record into the
// active-profile globals and applies its video, audio, and network[ing] settings."; the 0x2004
// byte per-player stride matches out/phase4/interface_types_notes.md's "saved player profile
// record (0x2004 bytes)"; player_profile_apply_video_options.c / player_profile_apply_audio_
// options.c are this function's own FUN_00495580/FUN_004957d0 callees.
// Review pass (phase 4): the register convention was inverted in the first rewrite. The
// disassembly is `mov ebx, eax; movsx eax, bx; imul eax, eax, 0x2004` (AX is the player index
// that scales the stride and is still in BX for player_profile_refresh_settings_cache), the
// stack dword is what gets stored at 0x714dd4 + index * 0x2004 and later compared with -1
// (the profile id), and EDX is the 0x1ffc byte source record. All eleven call sites
// (0x49535d, 0x495496, 0x495e42, 0x49e064, 0x49e14f, 0x4a0ba3, 0x4a1a54, 0x4a2a72, 0x4c9d86,
// 0x539d39, 0x539de3) push the slot and set EDX to a record; 0x49e064 (EAX from
// player_profile_find_index_by_id) and 0x49e14f (EAX = EBP) compute the index, the rest pass 0.
// blam-cc: AX -> player_index, EDX -> source_profile, stack -> profile_id
// The profile record's working copy, video/audio options and the per-record network port pair
// at +0x1002/+0x1004 are addressed from ebp = 0x712dd8 + index * 0x2004. 0x698208/0x69820c are
// dwords compared against zero-extended words.
// UNSURE: control_profile_reestablish_device_slot_mappings (EAX -> record) and saved_game_get_directory_by_handle (EAX -> slot, ESI -> name buffer) are
// profile-module functions not rewritten in this tree. The tail is a tail jump into
// saved_game_last_profile_clear with its one stack argument overwritten by 0x718e80.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

extern int32_t current_profile_index;               // 0x00714dd4, record 0 field +0x1ffc
extern uint8_t profile_globals_block[];             // 0x00712dd8, stride 0x2004 per player
extern game_engine_definition *current_game_engine; // 0x006f1d20 (current_game_engine, opaque here)
extern uint8_t unknown_0071c2d0;                    // 0x0071c2d0, UNSURE
extern uint32_t network_channel_port_a;             // 0x00698208, UNSURE name
extern uint32_t network_channel_port_b;             // 0x0069820c, UNSURE name
extern uint32_t unknown_007227b8;                   // 0x007227b8, UNSURE: copy of port_a
extern int32_t cached_profile_slot;                 // 0x0068e66c (per player_profile_subsystem_initialize.c)
extern char last_profile_name[];                    // 0x00718e80 (per player_profile_subsystem_initialize.c)

extern void player_profile_refresh_settings_cache(int16_t player_index); // 0x496060, BX
extern void control_profile_reestablish_device_slot_mappings(uint8_t *profile_record);         // 0x53b620; blam-cc: EAX -> profile_record
extern int32_t player_profile_apply_video_options(uint8_t *settings); // 0x495580
extern void player_profile_apply_audio_options(uint8_t *settings);     // 0x4957d0
extern void network_channels_close(void);  // 0x441480
extern void network_channels_open(void);   // 0x441300
extern uint8_t saved_game_get_directory_by_handle(int32_t slot, char *out_name); // 0x53d080; blam-cc: EAX -> slot, ESI -> out_name
extern void saved_game_last_profile_clear(char *name);     // 0x53d220

// blam-cc: AX -> player_index, EDX -> source_profile, stack -> profile_id
// Copies a loaded 0x1ffc byte profile record into player_index's slot of the module's profile
// globals, records profile_id as that slot's profile index, refreshes the settings cache and
// applies the video/audio settings, reopens the network channels if the record's port pair
// differs from what is currently bound, and (for a real, non-default profile) updates the
// cached profile slot bookkeeping and the last-profile marker.
void player_profile_load(int16_t player_index, void *source_profile, int32_t profile_id)
{
    uint8_t *record;
    uint32_t *dst_words;
    uint32_t *src_words;
    uint32_t i;

    record = profile_globals_block + (int32_t)player_index * 0x2004;
    *(int32_t *)(record + 0x1ffc) = profile_id;   // 0x714dd4 + index * 0x2004
    dst_words = (uint32_t *)record;
    src_words = (uint32_t *)source_profile;
    for (i = 0x7ff; i != 0; i--) {
        *dst_words++ = *src_words++;
    }

    player_profile_refresh_settings_cache(player_index);
    control_profile_reestablish_device_slot_mappings(record);
    player_profile_apply_video_options(record);
    player_profile_apply_audio_options(record);

    if (current_game_engine == (void *)0 && unknown_0071c2d0 == 0 &&
        (network_channel_port_a != *(uint16_t *)(record + 0x1002) ||
         network_channel_port_b != *(uint16_t *)(record + 0x1004))) {
        network_channels_close();
        network_channel_port_a = *(uint16_t *)(record + 0x1002);
        network_channel_port_b = *(uint16_t *)(record + 0x1004);
        network_channels_open();
        unknown_007227b8 = network_channel_port_a;
    }

    if (profile_id != -1) {
        if (cached_profile_slot != current_profile_index) {
            if (current_profile_index != -1) {
                saved_game_get_directory_by_handle(current_profile_index, last_profile_name);
            }
            cached_profile_slot = current_profile_index;
        }
        if (last_profile_name[0] != '\0') {
            saved_game_last_profile_clear(last_profile_name);
        }
    }
}

#if 0
Original Ghidra decompilation (0x495970):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void player_profile_load(int param_1)

{
  short in_AX;
  int iVar1;
  int iVar2;
  int iVar3;
  undefined4 *in_EDX;
  undefined4 *puVar4;

  iVar1 = (int)in_AX;
  iVar2 = iVar1 * 0x2004;
  (&DAT_00714dd4)[iVar1 * 0x801] = param_1;
  puVar4 = &DAT_00712dd8 + iVar1 * 0x801;
  for (iVar3 = 0x7ff; iVar3 != 0; iVar3 = iVar3 + -1) {
    *puVar4 = *in_EDX;
    in_EDX = in_EDX + 1;
    puVar4 = puVar4 + 1;
  }
  FUN_00496060();
  FUN_0053b620();
  FUN_00495580();
  FUN_004957d0();
  if (((DAT_006f1d20 == 0) && (DAT_0071c2d0 == '\0')) &&
     ((DAT_00698208 != *(ushort *)(&DAT_00713dda + iVar2) ||
      (_DAT_0069820c != *(ushort *)(&DAT_00713ddc + iVar2))))) {
    network_channels_close();
    DAT_00698208 = (uint)*(ushort *)(&DAT_00713dda + iVar2);
    _DAT_0069820c = (uint)*(ushort *)(&DAT_00713ddc + iVar2);
    network_channels_open();
    DAT_007227b8 = DAT_00698208;
  }
  if (param_1 != -1) {
    if (DAT_0068e66c != DAT_00714dd4) {
      if (DAT_00714dd4 != -1) {
        FUN_0053d080();
      }
      DAT_0068e66c = DAT_00714dd4;
    }
    if (DAT_00718e80 != '\0') {
      saved_game_last_profile_clear();
      return;
    }
  }
  return;
}
#endif
