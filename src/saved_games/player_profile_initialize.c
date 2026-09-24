// player_profile_initialize  (Ghidra: player_profile_initialize, already named)
// address 0x53a1c0, size 1089 bytes
// name confidence: 0.55   rewrite confidence: 0.8
// evidence: out/phase4/saved_games_functions.md; out/phase4/saved_games_types_notes.md's five
// carry-over block sizes (0x93c, 0x110, 0x108, 0x10b, 0x880 dwords/bytes) match this function's
// five merge-copy loops exactly once each Ghidra "separate" local array (local_1edc, local_1490,
// local_15a0, local_1388, local_f00) is recognized as a view into the single local_2008
// (existing-profile) buffer at its own field offset (0x2008 - each local's own stack offset
// gives 0x12c, 0xb78, 0xa68, 0xc80, 0x1108 respectively, exactly the block boundaries the notes
// document) -- there is no separate merge-source struct, just one saved_player_profile-sized
// local. cache_file_index (0x006ac494, -1 == no map loaded, per src/cache/cache_file_load.c)
// gates the default-gamepad-slot fill. saved_game_enumerate_by_type's (EBX capacity_and_count,
// stack type/out_handles/builtin_only) signature and player_profile_get's argument order are
// taken from this session's own player_profile_get.c (index, out_buffer) -- note
// src/saved_games/saved_game_delete_by_display_name.c (a different rewriter) declared
// player_profile_get with the arguments in the opposite order; both name the same two values,
// this file's own definition of player_profile_get is authoritative.
// register convention: __cdecl (Ghidra-recognized); profile, local_player_index and
// merge_existing are the recognized stack parameters (Ghidra's own param_1/param_2/param_3).
// Phase 4 review (objdump 0x53a1c0..0x53a60c, line by line): the enumerate call is
// EBX = &capacity (1), stack (type 0, &handle, builtin_only 0), confirmed at 0x53a543; the
// two 0.1885 floats are exactly 0x3e4104fc (0.188495576f, the first rewrite rounded them to
// 0.1885f, a different float); the explicit keyboard / mouse unbound fill is restored; the
// early zero store is +0x130 (unknown_130), not button_set. Everything else matched.
// reconciled: R21 saved_player_profile unknown_12f/131/93c[6]/954/955/960[2] -> look_inverted, look_inverted_driving, forward_rate..mouse_strafe_scale, mouse_look_x/y_sensitivity, gamepad_axis_scale_x/y

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"

extern uint32_t no_simd_matrix_multiply_flag; // 0x007196f4, read as a dword (src/math/math_initialize.c name); nonzero selects the low-end defaults
extern uint32_t unknown_00722bac; // machine class threshold, matches player_profile_set_default_video_options
extern uint32_t unknown_00722ba8; // machine class threshold
extern int32_t saved_player_profile_slots_handle; // 0x00714dd4
extern saved_player_profile_slot saved_player_profile_slots[k_maximum_local_player_profiles]; // 0x00712dd8
extern int16_t cache_file_index; // 0x006ac494, -1 when no map is loaded

extern void control_profile_reset_digital_bindings(saved_player_profile *profile); // 0x539ff0
extern void control_profile_reset_analog_bindings(saved_player_profile *profile); // 0x53a0d0
extern uint8_t player_profile_set_default_video_options(saved_player_profile *profile, uint8_t allow_display_query); // 0x53b000
extern void saved_game_enumerate_by_type(uint16_t *capacity_and_count, uint16_t type,
    int32_t *out_handles, uint8_t builtin_only); // 0x53c4e0, this module, blam-cc: EBX capacity_and_count
extern uint8_t player_profile_get(int32_t index, saved_player_profile *out_buffer); // 0x53a770, this module
extern void control_profile_fill_default_gamepad_slots(saved_player_profile *profile); // 0x53b7f0, this module
extern void *memset(void *dest, int32_t value, uint32_t count);
extern void *memcpy(void *dest, const void *src, uint32_t count);

// Builds a fresh default profile in place: zeroes it, sets identity/version/flags from
// local_player_index, resets every control binding to unbound plus a set of default bindings,
// applies default video options, sets audio/network/misc defaults, and -- when merge_existing
// is set -- copies the controls/video/audio/extra-settings/gamepad blocks from an existing
// profile (the current local slot if one is active, otherwise the first enumerated real profile
// saved game, falling back to leaving the freshly-built defaults alone if neither is available)
// on top of the freshly-built defaults. Finally, if a map is loaded, fills any empty gamepad
// slots with default device profiles.
void player_profile_initialize(saved_player_profile *profile, int32_t local_player_index, uint8_t merge_existing)
{
    saved_player_profile existing;
    uint16_t capacity;
    int32_t handles[1];
    int32_t handle;

    memset(profile, 0, sizeof(*profile));
    profile->version = k_saved_player_profile_version;
    profile->player_color = -1;
    profile->unknown_130 = 0;
    profile->look_sensitivity = 3;
    profile->look_inverted = 0;
    profile->look_inverted_driving = 0;
    profile->flags |= ((uint16_t)(uint8_t)local_player_index << 8) | 1;
    profile->unknown_133 = 0;

    {
        int32_t gp, i, j;
        // 0x53a21f..0x53a26f: keyboard, mouse button and mouse axis bindings to unbound
        // (the two reset calls below refill the same range, so this is belt and braces in
        // the original too)
        for (i = 0; i < k_control_keyboard_key_count; i = i + 1) {
            profile->keyboard_bindings[i] = k_control_binding_unbound;
        }
        for (i = 0; i < k_control_mouse_button_count; i = i + 1) {
            profile->mouse_button_bindings[i] = k_control_binding_unbound;
        }
        for (i = 0; i < k_control_mouse_axis_count; i = i + 1) {
            profile->mouse_axis_bindings[i][0] = k_control_binding_unbound;
            profile->mouse_axis_bindings[i][1] = k_control_binding_unbound;
        }
        for (gp = 0; gp < k_control_gamepad_count; gp = gp + 1) {
            for (i = 0; i < k_control_gamepad_button_count; i = i + 1) {
                profile->gamepad_button_bindings[gp][i] = k_control_binding_unbound;
            }
            profile->gamepad_action_buttons[gp][0] = -1;
            profile->gamepad_action_buttons[gp][1] = -1;
            for (i = 0; i < k_control_gamepad_axis_count; i = i + 1) {
                profile->gamepad_axis_bindings[gp][i][0] = k_control_binding_unbound;
                profile->gamepad_axis_bindings[gp][i][1] = k_control_binding_unbound;
            }
            for (i = 0; i < k_control_gamepad_pov_count; i = i + 1) {
                for (j = 0; j < k_control_gamepad_pov_direction_count; j = j + 1) {
                    profile->gamepad_pov_bindings[gp][i][j] = k_control_binding_unbound;
                }
            }
        }
    }

    control_profile_reset_digital_bindings(profile);
    control_profile_reset_analog_bindings(profile);

    profile->forward_rate = 1.0f;
    profile->strafe_rate = 1.0f;
    profile->look_x_rate = 0.188495576f; // 0x3e4104fc
    profile->look_y_rate = 0.188495576f;
    profile->mouse_forward_scale = 128.0f;
    profile->mouse_strafe_scale = 128.0f;
    profile->mouse_look_x_sensitivity = 3;
    profile->mouse_look_y_sensitivity = 3;
    profile->look_inverted = 0;

    player_profile_set_default_video_options(profile, (uint8_t)merge_existing);

    if (no_simd_matrix_multiply_flag == 0 && 1000 < unknown_00722bac && 0x80 < unknown_00722ba8) {
        profile->unknown_b7d = 1;
        profile->unknown_b7f = 2;
    } else {
        profile->unknown_b7d = 0;
        profile->unknown_b7f = 1;
    }
    profile->master_volume = 10;
    profile->unknown_c81 = 1;
    profile->effects_volume = 10;
    profile->music_volume = 6;
    profile->unknown_b7b = 0;
    profile->unknown_c80 = 3;
    profile->unknown_b7c = 0;
    profile->unknown_c82 = 1;
    profile->unknown_c83 = 0;
    profile->unknown_c84 = 0;
    profile->unknown_c85 = 0;
    profile->unknown_c86 = 1;
    profile->unknown_c87 = 1;
    profile->unknown_c88 = 0;
    profile->unknown_c89 = 0;
    profile->unknown_c8a = 0;

    profile->server_name[0] = 'H'; profile->server_name[1] = 'a';
    profile->server_name[2] = 'l'; profile->server_name[3] = 'o'; profile->server_name[4] = 0;
    profile->server_password[0] = 0;
    profile->gamepad_axis_scale_x = 0.75f;
    profile->gamepad_axis_scale_y = 0.75f;
    profile->unknown_ebe = 0;
    profile->unknown_ebf = 3;
    profile->unknown_fc2[0] = 0;
    profile->unknown_fc0 = 1;
    profile->server_port = 0x8fe;
    profile->client_port = 0x8ff;

    profile->gamepad_rate_a[0] = 3; profile->gamepad_rate_b[0] = 3;
    profile->gamepad_rate_a[1] = 3; profile->gamepad_rate_b[1] = 3;
    profile->gamepad_rate_a[2] = 3; profile->gamepad_rate_b[2] = 3;
    profile->gamepad_rate_a[3] = 3; profile->gamepad_rate_b[3] = 3;

    profile->last_campaign_level = 0;
    if (local_player_index == 0) {
        profile->button_set = 0;
        profile->joystick_set = 0;
    } else if (local_player_index == 1) {
        profile->look_inverted = 1;
        profile->button_set = 0;
        profile->joystick_set = 0;
    }

    if (merge_existing != 0) {
        uint8_t have_existing = 0;

        if (saved_player_profile_slots_handle == -1) {
            capacity = 1;
            handles[0] = -1;
            saved_game_enumerate_by_type(&capacity, _saved_game_type_player_profile, handles, 0);
            handle = handles[0];
            if (capacity >= 1 && handle != -1 && player_profile_get(handle, &existing) != 0) {
                have_existing = 1;
            }
        } else {
            existing = saved_player_profile_slots[0].profile;
            have_existing = 1;
        }

        if (have_existing) {
            memcpy((uint8_t *)profile + 0x12c, (uint8_t *)&existing + 0x12c, 0x93c);
            memcpy((uint8_t *)profile + 0xb78, (uint8_t *)&existing + 0xb78, 0x108);
            memcpy((uint8_t *)profile + 0xa68, (uint8_t *)&existing + 0xa68, 0x110);
            memcpy((uint8_t *)profile + 0xc80, (uint8_t *)&existing + 0xc80, 0x10b);
            memcpy((uint8_t *)profile + 0x1108, (uint8_t *)&existing + 0x1108, 0x880);
        }
    }

    if (cache_file_index != -1) {
        control_profile_fill_default_gamepad_slots(profile);
    }
}

#if 0
Original Ghidra decompilation (0x53a1c0):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

void player_profile_initialize(undefined4 *param_1,int param_2,undefined4 param_3)

{
  char cVar1;
  undefined4 *puVar2;
  undefined1 *puVar3;
  int iVar4;
  undefined4 *puVar5;
  undefined4 *puVar6;
  undefined4 *local_2018;
  undefined4 *local_2014;
  int local_2010;
  undefined4 *local_200c;
  undefined4 local_2008 [75];
  undefined4 local_1edc [591];
  undefined4 local_15a0 [68];
  undefined4 local_1490 [66];
  undefined4 local_1388;
  undefined4 local_f00 [957];
  undefined4 uStack_c;

  uStack_c = 0x53a1d0;
  puVar2 = param_1;
  for (iVar4 = 0x7ff; iVar4 != 0; iVar4 = iVar4 + -1) {
    *puVar2 = 0;
    puVar2 = puVar2 + 1;
  }
  *(undefined1 *)param_1 = 9;
  *(undefined2 *)((int)param_1 + 0x11a) = 0xffff;
  *(undefined1 *)(param_1 + 0x4c) = 0;
  *(undefined1 *)((int)param_1 + 0x12e) = 3;
  *(undefined1 *)((int)param_1 + 0x12f) = 0;
  *(undefined1 *)((int)param_1 + 0x131) = 0;
  *(ushort *)(param_1 + 0x47) = *(ushort *)(param_1 + 0x47) | (ushort)(byte)param_2 << 8 | 1;
  *(undefined1 *)((int)param_1 + 0x133) = 0;
  puVar2 = param_1 + 0x4d;
  for (iVar4 = 0x36; iVar4 != 0; iVar4 = iVar4 + -1) {
    *puVar2 = 0x7fff7fff;
    puVar2 = puVar2 + 1;
  }
  *(undefined2 *)puVar2 = 0x7fff;
  *(undefined4 *)((int)param_1 + 0x20e) = 0x7fff7fff;
  *(undefined4 *)((int)param_1 + 0x212) = 0x7fff7fff;
  *(undefined4 *)((int)param_1 + 0x216) = 0x7fff7fff;
  *(undefined4 *)((int)param_1 + 0x21a) = 0x7fff7fff;
  puVar2 = param_1 + 0x88;
  iVar4 = 3;
  do {
    *(undefined2 *)((int)puVar2 + -2) = 0x7fff;
    *(undefined2 *)puVar2 = 0x7fff;
    puVar2 = puVar2 + 1;
    iVar4 = iVar4 + -1;
  } while (iVar4 != 0);
  local_2014 = (undefined4 *)((int)param_1 + 0x53a);
  local_2018 = (undefined4 *)((int)param_1 + 0x32a);
  puVar2 = param_1 + 0xcf;
  puVar5 = (undefined4 *)((int)param_1 + 0x22a);
  local_2010 = 4;
  do {
    *puVar5 = 0x7fff7fff;
    puVar5[1] = 0x7fff7fff;
    puVar5[2] = 0x7fff7fff;
    puVar5[3] = 0x7fff7fff;
    puVar5[4] = 0x7fff7fff;
    puVar5[5] = 0x7fff7fff;
    puVar5[6] = 0x7fff7fff;
    puVar5[7] = 0x7fff7fff;
    puVar5[8] = 0x7fff7fff;
    puVar5[9] = 0x7fff7fff;
    puVar5[10] = 0x7fff7fff;
    puVar5[0xb] = 0x7fff7fff;
    puVar5[0xc] = 0x7fff7fff;
    puVar5[0xd] = 0x7fff7fff;
    puVar5[0xe] = 0x7fff7fff;
    puVar5[0xf] = 0x7fff7fff;
    *local_2018 = 0xffffffff;
    iVar4 = 0x20;
    do {
      *(undefined2 *)((int)puVar2 + -2) = 0x7fff;
      *(undefined2 *)puVar2 = 0x7fff;
      puVar2 = puVar2 + 1;
      iVar4 = iVar4 + -1;
    } while (iVar4 != 0);
    puVar6 = local_2014;
    for (iVar4 = 0x40; iVar4 != 0; iVar4 = iVar4 + -1) {
      *puVar6 = 0x7fff7fff;
      puVar6 = puVar6 + 1;
    }
    local_2018 = local_2018 + 1;
    local_2014 = local_2014 + 0x40;
    puVar5 = puVar5 + 0x10;
    local_2010 = local_2010 + -1;
  } while (local_2010 != 0);
  control_profile_reset_digital_bindings();
  control_profile_reset_analog_bindings();
  param_1[0x24f] = 0x3f800000;
  param_1[0x250] = 0x3f800000;
  param_1[0x251] = 0x3e4104fc;
  param_1[0x252] = 0x3e4104fc;
  param_1[0x253] = 0x43000000;
  param_1[0x254] = 0x43000000;
  *(undefined1 *)(param_1 + 0x255) = 3;
  *(undefined1 *)((int)param_1 + 0x955) = 3;
  *(undefined1 *)((int)param_1 + 0x12f) = 0;
  player_profile_set_default_video_options(param_1,param_3);
  if (((DAT_007196f4 == 0) && (1000 < DAT_00722bac)) && (0x80 < DAT_00722ba8)) {
    *(undefined1 *)((int)param_1 + 0xb7d) = 1;
    *(undefined1 *)((int)param_1 + 0xb7f) = 2;
  }
  else {
    *(undefined1 *)((int)param_1 + 0xb7d) = 0;
    *(undefined1 *)((int)param_1 + 0xb7f) = 1;
  }
  *(undefined1 *)(param_1 + 0x2de) = 10;
  local_200c = param_1 + 800;
  *(undefined1 *)((int)param_1 + 0xb79) = 10;
  *(undefined1 *)((int)param_1 + 0xb7a) = 6;
  *(undefined1 *)((int)param_1 + 0xb7b) = 0;
  *(undefined1 *)(param_1 + 0x2df) = 0;
  *(undefined1 *)((int)param_1 + 0xb7e) = 0;
  *(undefined1 *)((int)param_1 + 0xc81) = 1;
  *(undefined1 *)local_200c = 3;
  *(undefined1 *)((int)param_1 + 0xc82) = 1;
  *(undefined1 *)((int)param_1 + 0xc83) = 0;
  *(undefined1 *)(param_1 + 0x321) = 0;
  *(undefined1 *)((int)param_1 + 0xc85) = 0;
  *(undefined1 *)((int)param_1 + 0xc86) = 1;
  *(undefined1 *)((int)param_1 + 0xc87) = 1;
  *(undefined1 *)(param_1 + 0x322) = 0;
  *(undefined1 *)((int)param_1 + 0xc89) = 0;
  *(undefined1 *)((int)param_1 + 0xc8a) = 0;
  FUN_00625b7a(L"Halo");
  _wcscpy((wchar_t *)(param_1 + 0x363),L"Halo");
  FUN_00625b7a(&DAT_00660c34);
  _wcscpy((wchar_t *)(param_1 + 0x3ab),L"");
  param_1[600] = 0x3f400000;
  param_1[0x259] = 0x3f400000;
  *(undefined1 *)((int)param_1 + 0xebe) = 0;
  *(undefined1 *)((int)param_1 + 0xebf) = 3;
  *(undefined2 *)((int)param_1 + 0xfc2) = 0;
  *(undefined1 *)(param_1 + 0x3f0) = 1;
  *(undefined2 *)((int)param_1 + 0x1002) = 0x8fe;
  *(undefined2 *)(param_1 + 0x401) = 0x8ff;
  puVar3 = (undefined1 *)((int)param_1 + 0x95a);
  iVar4 = 4;
  do {
    puVar3[-4] = 3;
    *puVar3 = 3;
    puVar3 = puVar3 + 1;
    iVar4 = iVar4 + -1;
  } while (iVar4 != 0);
  *(undefined2 *)(param_1 + 0x4a) = 0;
  if (param_2 == 0) {
LAB_0053a509:
    *(undefined1 *)(param_1 + 0x4b) = 0;
    *(undefined1 *)((int)param_1 + 0x12d) = 0;
  }
  else if (param_2 == 1) {
    *(undefined1 *)((int)param_1 + 0x12f) = 1;
    goto LAB_0053a509;
  }
  if ((char)param_3 != '\0') {
    if (DAT_00714dd4 == -1) {
      local_2010 = 1;
      local_2014 = (undefined4 *)0xffffffff;
      saved_game_enumerate_by_type(0,&local_2014,0);
      if ((((short)local_2010 < 1) || (local_2014 == (undefined4 *)0xffffffff)) ||
         (cVar1 = player_profile_get(local_2014), cVar1 == '\0')) goto LAB_0053a5f3;
    }
    else {
      puVar2 = &DAT_00712dd8;
      puVar5 = local_2008;
      for (iVar4 = 0x7ff; iVar4 != 0; iVar4 = iVar4 + -1) {
        *puVar5 = *puVar2;
        puVar2 = puVar2 + 1;
        puVar5 = puVar5 + 1;
      }
    }
    puVar2 = local_1edc;
    puVar5 = param_1 + 0x4b;
    for (iVar4 = 0x24f; iVar4 != 0; iVar4 = iVar4 + -1) {
      *puVar5 = *puVar2;
      puVar2 = puVar2 + 1;
      puVar5 = puVar5 + 1;
    }
    puVar2 = local_1490;
    puVar5 = param_1 + 0x2de;
    for (iVar4 = 0x42; iVar4 != 0; iVar4 = iVar4 + -1) {
      *puVar5 = *puVar2;
      puVar2 = puVar2 + 1;
      puVar5 = puVar5 + 1;
    }
    puVar2 = local_15a0;
    puVar5 = param_1 + 0x29a;
    for (iVar4 = 0x44; iVar4 != 0; iVar4 = iVar4 + -1) {
      *puVar5 = *puVar2;
      puVar2 = puVar2 + 1;
      puVar5 = puVar5 + 1;
    }
    puVar2 = &local_1388;
    puVar5 = local_200c;
    for (iVar4 = 0x42; iVar4 != 0; iVar4 = iVar4 + -1) {
      *puVar5 = *puVar2;
      puVar2 = puVar2 + 1;
      puVar5 = puVar5 + 1;
    }
    *(undefined2 *)puVar5 = *(undefined2 *)puVar2;
    *(undefined1 *)((int)puVar5 + 2) = *(undefined1 *)((int)puVar2 + 2);
    puVar2 = local_f00;
    puVar5 = param_1 + 0x442;
    for (iVar4 = 0x220; iVar4 != 0; iVar4 = iVar4 + -1) {
      *puVar5 = *puVar2;
      puVar2 = puVar2 + 1;
      puVar5 = puVar5 + 1;
    }
  }
LAB_0053a5f3:
  if (DAT_006ac494 != -1) {
    FUN_0053b7f0(param_1);
  }
  return;
}
#endif
