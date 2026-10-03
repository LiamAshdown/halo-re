#pragma once

/* Include after the engine type headers (types/*.h carry no include guards). */

namespace halo::game::engine1 {

/**
 * Game variant selection, loading, validation and player profile capture. Stateless service class: every
 * function is a static member and the state it acts on lives in the engine globals.
 */
class Variants {
public:
    static void apply_current_custom_variant(void);
    static void apply_player_profile_entry(void *event);
    static void apply_variant(const game_variant *variant);
    static void capture_player_profile(int32_t slot, int32_t commit);
    static uint32_t ensure_variant_history_has_entry(void);
    static void free_custom_variant_cache(void);
    static uint8_t get_variant_by_name(const char *name, game_variant *out);
    static void invoke_profile_post_update_callback(uint32_t arg_ecx, uint32_t arg_edx);
    static uint32_t is_map_and_variant_valid(const char *map_path, const char *variant_name);
    static void load_from_variant(const game_variant *variant);
};

}
