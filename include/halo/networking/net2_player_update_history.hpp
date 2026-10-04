/**
 * @file include/halo/networking/net2_player_update_history.hpp
 * Player update history ring, queue and replay.
 *
 * The engine type headers (types/*.h) are not include-guarded: include this header after them.
 */
#pragma once

namespace halo::networking {

/**
 * Player update history ring, queue and replay.
 *
 * Static behaviour facade; the original C entry points forward to these members.
 */
class PlayerUpdateHistory {
public:
    /**
     * Original engine function `player_data_iterator_advance`.
     *
     * @address 0x4d98f0
     */
    static int32_t advance(int16_t player_index);

    /**
     * this call site's decompile shows zero arguments (`cVar1 = player_unit_has_parent();`), i.e. the batch dropped whatever ECX held; unit_ext->controlling_player (unit_data +0x218) is the only player handle in scope and is what is passed here, but this is a reconstruction, not something the decompile itself shows.
     *
     * @address 0x4e6b50
     */
    static uint8_t add(datum_index unit_index, player_update_history *history,
    int32_t tick_count, player_action control, int32_t *out_update_id);

    /**
     * Frees every node of history's linked list and then the history container itself.
     *
     * @address 0x4e6b10
     */
    static void destroy(player_update_history *history);

    /**
     * Walks history's linked list looking for the node whose update_id equals target_id. If prune is set and a match is found, frees every node from the head through the match (inclusive) and makes the following node the new head (clearing tail too if the list becomes empty). Always returns the node that followed the match, or NULL if no match was found.
     *
     * @address 0x4e6f60
     */
    static player_update_history_node * find_and_prune(player_update_history *history,
    int32_t target_id, uint8_t prune);

    /**
     * Frees every node of a player_update_history's linked list and clears its head/tail.
     *
     * @address 0x4e6f20
     */
    static void free_all(player_update_history *history);

    /**
     * Copies name (an 8-bit string, e.g. a console command argument) into the shared UTF-16 filter buffer used by player_update_history_log_printf_filtered, truncating to 0x3ff characters.
     *
     * @address 0x4e5f80
     */
    static void log_set_name_filter(char *name);

    /**
     * Replays every history node from the one player_update_history_find_and_prune leaves (after discarding everything up to and including prune_target_id, when prune is set) through the end of the list, restoring each node's saved unit (and, if applicable, vehicle) state and then re-running that many ticks of biped_update/object_update on top of it -- reconciling client-side prediction with a server-acknowledged starting position.
     *
     * @address 0x4e6ff0
     */
    static int32_t play(uint8_t prune, int32_t prune_target_id,
    player_update_history *history, datum_index unit_index, float server_x, float server_y,
    float server_z, local_player_vehicle_update_ack *vehicle_ack);

    /**
     * the first two are the AL/ECX register arguments Ghidra drops at every call site Resolves the player at player_index and replays the client's update-history against its current unit and cached position, using the client's global update_history list.
     *
     * @address 0x4e6950
     */
    static void play_for_update_index(datum_index player_index);

    /**
     * Finds the local player's controlled unit (if any) and, if network_client's update-history list has a node matching target_update_id, replays history from the node after that match using its saved position as the reconciliation starting point.
     *
     * @address 0x4e7730
     */
    static void play_local_player(int32_t target_update_id);

    /**
     * Converts name to UTF-16, then finds the (first) player whose name matches it and busy-walks that player's update-history queue's read_index to its write_index before calling
     *
     * @address 0x4e5fe0
     */
    static void flush_by_name(char *name);

    /**
     * Returns how many steps ahead new_update_id is of the sequence id at the head of plr's player_update_queue (wrapping modulo 64), or -1 if the queue currently holds nothing to compare against.
     *
     * @address 0x4e6aa0
     */
    static int32_t offset_from_head(player *plr, int32_t new_update_id);

    /**
     * Looks up (and remaps in place) decode_context's remote-player index, decodes a baseline or final contents are not the whole story), then re-validates the remapped index a second time -- requiring it to name a live, non-local player -- before dispatching to handle_remote_player_action_update.
     *
     * @address 0x4e5720
     */
    static void remote_player_action_update_apply(int32_t **decode_context);

};

}  // namespace halo::networking
