// game_engine_update_end_game_sequence  (Ghidra: game_engine_update_end_game_sequence, already
// named)
// address 0x45fdf0, size 308 bytes
// name confidence: 0.55   rewrite confidence: 0.9
// evidence: types/game.h game_engine_state (0x0087aa10), game_engine_end_game_timer (0x0087aa08,
// "7.0 s, then 5.0 s"), game_engine_post_game_fade (0x0087aa0c, "ramps 0 -> 1"),
// game_engine_dedicated_idle/_timer (0x0087aa18/0x0087aa1c), network_server (0x0071c2d4).
// The two float tests are `fcomp / test ah,0x41 / jp` idioms that reduce to `a > 0.0` and `a <= 0.0` (no NaN can occur).
// input_get_key_state takes ECX = key 0x66 (0x45feb3); FIXED 2026-09-30: the draft called it with no key.
// DAT_0071c2de, DAT_007124a0 and DAT_007124a1 are not attributed to this module; kept as raw externs with generic names.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern game_engine_definition *current_game_engine; // 0x006f1d20
extern game_engine_state game_engine_state_value;   // 0x0087aa10
extern float game_engine_end_game_timer;            // 0x0087aa08
extern float game_engine_post_game_fade;            // 0x0087aa0c
extern uint8_t game_engine_dedicated_idle;          // 0x0087aa18
extern float game_engine_dedicated_idle_timer;      // 0x0087aa1c
extern int16_t network_game_mode;                   // 0x00719720
extern uint8_t *network_server;                       // 0x0071c2d4
extern uint8_t network_host_handoff_requested;                    // UNSURE identity/owning module
extern uint8_t unknown_007124a0;                    // UNSURE identity/owning module
extern uint8_t chimera_loading_screen_cleanup_gate;                    // UNSURE identity/owning module

extern void game_engine_end_game_sequence_stage3(void); // 0x467180, not in this batch
extern void game_engine_send_end_game_notification(uint32_t reason); // blam-cc: EAX reason; // 0x4671d0, not in this batch
extern uint8_t input_get_key_state(int16_t key_index); // 0x490b50, blam-cc: ECX key_index
extern void chimera__console_out(ColorARGB *color, char *format, ...); // 0x496b50, EAX color (NULL = default)
extern void chat_close(void); // 0x4aa900
extern void network_game_client_game_settings_updated(void *session); // 0x4df2e0

// Advances the two end-of-game countdown stages (game_engine_state _ending then _ended) and the
// post-game fade, driving the dedicated-server idle message/chat shutdown and re-notifying
// connected clients of settings changes once the fade or idle timer finishes.
void game_engine_update_end_game_sequence(float delta_time)
{
    if (current_game_engine == 0) {
        return;
    }

    if (game_engine_state_value == _game_engine_state_ended) {
        // Reuses game_engine_end_game_timer (0x0087aa08) for the second, 5.0 s stage -- it is
        // re-seeded to 5.0 by whatever transitions state _ending -> _ended (not in this batch).
        game_engine_end_game_timer = game_engine_end_game_timer - delta_time;
        if (game_engine_end_game_timer > 0.0f) {
            return;
        }
        if (network_game_mode != 2) {
            return;
        }
        game_engine_end_game_sequence_stage3();
        game_engine_send_end_game_notification(3); // FIXED 2026-09-28: 0x45ff17 loads EAX = 3
        return;
    }

    if (game_engine_state_value != _game_engine_state_post_game) {
        return;
    }

    game_engine_post_game_fade = game_engine_post_game_fade + delta_time;
    if (1.0f < game_engine_post_game_fade) {
        game_engine_post_game_fade = 1.0f;
    }

    if (network_game_mode == 2) {
        uint8_t idle_timer_expired = 0;

        if (game_engine_dedicated_idle == 0) {
            if ((*((uint8_t *)network_server + 6) >> 2 & 1) != 0) {
                chimera__console_out((ColorARGB *)0, (char *)"Game Complete. Dedicated server is now idle.");
                network_host_handoff_requested = 1;
                chat_close();
            }
        } else {
            game_engine_dedicated_idle_timer = game_engine_dedicated_idle_timer - delta_time;
            if (game_engine_dedicated_idle_timer <= 0.0f) {
                idle_timer_expired = 1;
                game_engine_dedicated_idle = 0;
            }
        }

        if (unknown_007124a0 != 0 || input_get_key_state(0x66) == 1 || idle_timer_expired) {
            network_game_client_game_settings_updated(network_server);
        }
    }

    if (chimera_loading_screen_cleanup_gate != 0) {
        network_host_handoff_requested = 1;
        chat_close();
    }
}

#if 0
Original Ghidra decompilation (0x45fdf0), from tools/pack.py 0x45fdf0:

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void game_engine_update_end_game_sequence(float param_1)

{
  bool bVar1;
  char cVar2;

  if (DAT_006f1d20 == 0) {
    return;
  }
  if (DAT_0087aa10 == 2) {
    _DAT_0087aa08 = _DAT_0087aa08 - param_1;
    if (_DAT_0087aa08 < 0.0 == (_DAT_0087aa08 == 0.0)) {
      return;
    }
    if (DAT_00719720 != 2) {
      return;
    }
    FUN_00467180();
    FUN_004671d0();
    return;
  }
  if (DAT_0087aa10 != 3) {
    return;
  }
  DAT_0087aa0c = DAT_0087aa0c + param_1;
  if (1.0 < DAT_0087aa0c) {
    DAT_0087aa0c = 1.0;
  }
  if (DAT_00719720 == 2) {
    bVar1 = false;
    if (DAT_0087aa18 == '\0') {
      if ((*(byte *)((int)DAT_0071c2d4 + 6) >> 2 & 1) != 0) {
        chimera__console_out("Game Complete. Dedicated server is now idle.");
        DAT_0071c2de = 1;
        chat_close();
      }
    }
    else {
      _DAT_0087aa1c = _DAT_0087aa1c - param_1;
      if (_DAT_0087aa1c < 0.0 != (_DAT_0087aa1c == 0.0)) {
        bVar1 = true;
        DAT_0087aa18 = '\0';
      }
    }
    if (((DAT_007124a0 != '\0') || (cVar2 = FUN_00490b50(), cVar2 == '\x01')) || (bVar1)) {
      network_game_client_game_settings_updated(DAT_0071c2d4);
    }
  }
  if (DAT_007124a1 == '\0') {
    return;
  }
  DAT_0071c2de = 1;
  chat_close();
  return;
}
#endif
