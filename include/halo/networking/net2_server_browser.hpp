/**
 * @file include/halo/networking/net2_server_browser.hpp
 * Server browser filters, sorting, list rows and join latch.
 *
 * The engine type headers (types/*.h) are not include-guarded: include this header after them.
 */
#pragma once

namespace halo::networking {

/**
 * Server browser filters, sorting, list rows and join latch.
 *
 * Static behaviour facade; the original C entry points forward to these members.
 */
class ServerBrowser {
public:
    /**
     * Console command: lists installed game-variant names (an optional lowercased filter substring), two per output line.
     *
     * @address 0x4e4600
     */
    static void matching_substring(uint32_t argument_count, char **arguments);

    /**
     * Original engine function `join_game_server_browser_tick`.
     *
     * @address 0x4b80f0
     */
    static int32_t server_browser_tick(network_ui_widget *browser_widget);

    /**
     * stack parameter Looks up the join-game ticker labels tag; if found, copies its string_index'th string into buffer (truncated to capacity - 1 characters, always NUL-terminated). Leaves buffer as an empty string if the tag is missing.
     *
     * @address 0x4b6160
     */
    static void ticker_string_copy(uint16_t *buffer, int32_t capacity, int32_t string_index);

    /**
     * Console command: prints every installed map name (lowercased filter substring optional), two per output line.
     *
     * @address 0x4e4470
     */
    static void map_list_matching_substring(uint32_t argument_count, char **arguments);

    /**
     * Original engine function `server_browser_column_header_update`.
     *
     * @address 0x4b7f10
     */
    static void column_header_update(network_ui_widget *header, int32_t sort_direction);

    /**
     * Packs a custom game variant's numeric/boolean options into a compact bitfield pair (low ~28 bits and a second ~10-bit field) and formats them as "%d,%d" into a shared scratch buffer.
     *
     * @address 0x576180
     */
    static char * custom_options_pack(server_browser_custom_options *options);

    /**
     * VERIFIED against disassembly 0x5764a0..0x576747 (2026-09-30): all seven jump tables (0x576748..0x5767b0), the flag bit moves, both nibble clamps, the byte/dword field offsets and the sscanf "%d,%d" call match. A difftest "process died" here is the modern CRT aborting on an invalid string pointer, not a logic difference.
     *
     * @address 0x5764a0
     */
    static void custom_options_unpack(char *text, server_browser_custom_options *out);

    /**
     * Original engine function `server_browser_filter_headers_refresh`.
     *
     * @address 0x4b7f70
     */
    static void filter_headers_refresh(network_ui_widget *row);

    /**
     * Original engine function `server_browser_filter_panel_set_mode`.
     *
     * @address 0x4b61c0
     */
    static void filter_panel_set_mode(network_ui_widget *panel, uint8_t internet_mode);

    /**
     * Original engine function `server_browser_filter_widget_clicked`.
     *
     * @address 0x4b7d80
     */
    static int32_t filter_widget_clicked(network_ui_widget *clicked);

    /**
     * Encodes 4 boolean flags plus a time-limit enumeration into a compact code tagged with type id 1 (the low 3 bits).
     *
     * @address 0x5767d0
     */
    static uint32_t gametype1_flags_pack(server_browser_gametype1_options *options);

    /**
     * Decodes the compact type-1 code produced by server_browser_gametype1_flags_pack back into booleans and a low/high value pair.
     *
     * @address 0x576890
     */
    static void gametype1_flags_unpack(uint32_t code, server_browser_gametype1_decoded *out);

    /**
     * Encodes 3 boolean flags into a compact code tagged with type id 2 (the low 3 bits).
     *
     * @address 0x576920
     */
    static uint32_t gametype2_flags_pack(uint8_t *flags);

    /**
     * Encodes two booleans and five small numeric options into a compact code tagged with type id 3 (the low 3 bits).
     *
     * @address 0x5769c0
     */
    static uint32_t gametype3_flags_pack(server_browser_gametype3_options *options);

    /**
     * Decodes the compact type-3 code produced by server_browser_gametype3_flags_pack back into its two booleans and five numeric fields.
     *
     * @address 0x576a20
     */
    static void gametype3_flags_unpack(uint32_t code, server_browser_gametype3_options *out);

    /**
     * Encodes two small (0..2) numeric fields into a compact code tagged with type id 5 (the low 3 bits); a field outside 0..2 is simply omitted from the code.
     *
     * @address 0x576960
     */
    static uint32_t gametype5_flags_pack(int32_t *values);

    /**
     * Decodes the compact type-5 code produced by server_browser_gametype5_flags_pack back into two numeric fields, clamped to 0..2 (a decoded value of 3 is dropped to 0).
     *
     * @address 0x576990
     */
    static void gametype5_flags_unpack(uint32_t code, int32_t *out);

    /**
     * Original engine function `server_browser_latch_join_target`.
     *
     * @address 0x4b6730
     */
    static void latch_join_target(void);

    /**
     * pointer in ECX (in_ECX)
     *
     * @address 0x4b69c0
     */
    static void list_row_gather(network_ui_widget *row, uint8_t flag, void *entry);

    /**
     * the bound (0x1f / 7 at the call sites) rides in EDX and is passed as `count` // foreign
     *
     * @address 0x4b67e0
     */
    static void list_row_populate(network_ui_widget *row, uint8_t flag1, uint8_t flag2,
                                        const char *server_name, wchar_t *map_name,
                                        const char *gametype_name,
                                        uint8_t flag3, int32_t count_a, int32_t count_b, int32_t ping);

    /**
     * Original engine function `server_browser_open`.
     *
     * @address 0x4b75b0
     */
    static int32_t open(network_ui_widget *root);

    /**
     * Clears the ticker, then appends one formatted row per player (name and score, or a default placeholder row when the name lookup fails), clamping the player count to 0..16.
     *
     * @address 0x4b73e0
     */
    static int32_t player_list_populate(void *entry);

    /**
     * Original engine function `server_browser_query_results_ingest`.
     *
     * @address 0x4baae0
     */
    static void query_results_ingest(server_list_globals *list);

    /**
     * Original engine function `server_browser_result_array_sort`.
     *
     * @address 0x4ba9c0
     */
    static void result_array_sort(server_list_globals *array);

    /**
     * Original engine function `server_browser_selected_variant_description_build`.
     *
     * @address 0x4b74e0
     */
    static int32_t selected_variant_description_build(void *entry);

    /**
     * Original engine function `server_browser_server_passes_filter`.
     *
     * @address 0x4b7080
     */
    static uint8_t server_passes_filter(void *entry);

    /**
     * Original engine function `server_browser_sort_comparator_select`.
     *
     * @address 0x4ba970
     */
    static server_browser_sort_comparator sort_comparator_select(void);

    /**
     * Original engine function `server_browser_total_players_compute`.
     *
     * @address 0x4baa60
     */
    static void total_players_compute(server_list_globals *array);

    /**
     * Original engine function `server_browser_ui_refresh`.
     *
     * @address 0x4b73a0
     */
    static void ui_refresh(void);

    /**
     * Original engine function `server_list_compare_by_gametype`.
     *
     * @address 0x4b6fb0
     */
    static int32_t compare_by_gametype(const void *a, const void *b);

    /**
     * Original engine function `server_list_compare_by_hostname`.
     *
     * @address 0x4b6cd0
     */
    static int32_t compare_by_hostname(const void *a, const void *b);

    /**
     * Original engine function `server_list_compare_by_mapname`.
     *
     * @address 0x4b6c20
     */
    static int32_t compare_by_mapname(void **a, void **b);

    /**
     * Original engine function `server_list_compare_by_mapname_then_hostname`.
     *
     * @address 0x4b6f20
     */
    static int32_t compare_by_mapname_then_hostname(void **a, void **b);

    /**
     * Original engine function `server_list_compare_by_ping_then_hostname`.
     *
     * @address 0x4b6da0
     */
    static int32_t compare_by_ping_then_hostname(void **a, void **b);

    /**
     * Original engine function `server_list_compare_by_players`.
     *
     * @address 0x4b6e70
     */
    static int32_t compare_by_players(const void *a, const void *b);

    /**
     * Original engine function `server_list_compare_by_string_key`.
     *
     * @address 0x4b6be0
     */
    static int32_t compare_by_string_key(void **a, void **b, const char *key);

    /**
     * Waits (with a timeout) on the shared server-browser query-result mutex, unless the mutex has never been created, and returns a pointer to the shared server_list object on success (either the wait was not needed, it completed normally, or it timed out after abandonment -- 0x80 is WAIT_ABANDONED_0), or NULL if the wait failed or genuinely timed out.
     *
     * @address 0x4ba760
     */
    static server_list_globals * mutex_try_lock(uint32_t timeout_ms);

    /**
     * Original engine function `server_list_mutex_unlock`.
     *
     * @address 0x4ba7a0
     */
    static void mutex_unlock(server_list_globals **list_slot);

    /**
     * Original engine function `server_list_reset`.
     *
     * @address 0x4b65f0
     */
    static void reset(uint8_t *entry);

    /**
     * Thread-safely reads server_list.result_count, returning 0 if the mutex could not be acquired.
     *
     * @address 0x4ba820
     */
    static uint32_t result_count_get(void);

    /**
     * Original engine function `server_list_result_reset`.
     *
     * @address 0x4ba7c0
     */
    static void result_reset(uint8_t *entry);

    /**
     * Original engine function `server_list_scroll_clamp`.
     *
     * @address 0x4b7360
     */
    static void scroll_clamp(server_list_globals *results);

    /**
     * Original engine function `server_list_scroll_page_down`.
     *
     * @address 0x4b7bb0
     */
    static void scroll_page_down(uint8_t jump_to_bottom);

    /**
     * Original engine function `server_list_scroll_page_up`.
     *
     * @address 0x4b7b20
     */
    static void scroll_page_up(uint8_t jump_to_top);

};

}  // namespace halo::networking
