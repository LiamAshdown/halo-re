#pragma once

/* Include after the engine type headers (types/*.h carry no include guards). */

namespace halo::game::engine1 {

/**
 * Game engine lifetime: new game set-up, end-game sequence stages and per-frame effects. Stateless service
 * class: every function is a static member and the state it acts on lives in the engine globals.
 */
class Lifecycle {
public:
    static void dispose(void);
    static uint8_t attach_players_to_new_bsp(void);
    static void begin_end_game_sequence(void);
    static void end_game_sequence_stage1(void);
    static void end_game_sequence_stage2(void);
    static void end_game_sequence_stage3(void);
    static uint8_t get_teams_enabled(void);
    static void initialize_for_new_game(void);
    static uint8_t is_inactive(void);
    static void maybe_render_post_game(void);
    static int32_t multiplayer_ui_state_id(void);
};

}
