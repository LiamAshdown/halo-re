/**
 * @file include/halo/networking/net2_player_update_client.hpp
 * Client-side player update ingestion.
 *
 * The engine type headers (types/*.h) are not include-guarded: include this header after them.
 */
#pragma once

namespace halo::networking {

/**
 * Client-side player update ingestion.
 *
 * Static behaviour facade; the original C entry points forward to these members.
 */
class PlayerUpdateClient {
public:
    /**
     * Handles an incoming acknowledgement of the local player's own position update: if decode_context selects the baseline path and decodes successfully, finds the (single) local player, checks the new id is in order, logs it, latches the decoded control fields into the player object, and replays pending updates on top of it.
     *
     * @address 0x4e5390
     */
    static void local_player_update_from_network(message_delta_context *context);

    /**
     * stack -> update_history, unit, x, y, z, control_ptr Handles an incoming acknowledgement of the local player's own vehicle update: baseline-decodes it, resolves the local player's vehicle object via a directly-decoded handle (rather than by scanning, as the plain position-update sibling does), checks the new id is in order, logs it, latches the decoded control/position fields...
     *
     * @address 0x4e5490
     */
    static void local_player_vehicle_update_from_network(message_delta_context *context);

    /**
     * Handles an incoming remote-player action update (baseline or delta-encoded), applying the decoded 12-dword control record directly onto the target player object's control-record block (player::unknown_f0 .. unknown_11c) before dispatching to handle_remote_player_action_update.
     *
     * @address 0x4e5620
     */
    static void remote_player_action_update_from_network(message_delta_context *context);

    /**
     * Looks up (and remaps in place) decode_context's remote-player index, decodes a baseline or delta position update (player::unknown_164/168/16c), then forwards the result to player_update_client_remote_player_position_update_from_network.
     *
     * @address 0x4e5c40
     */
    static void remote_player_position_delta_from_network(message_delta_context *context);

    /**
     * Handles a decoded remote-player position update: either queues it for ordered replay against the pending action updates, or -- once the client has been out of range three times running -- snaps the player unit straight to it.
     *
     * @address 0x4e6270
     */
    static void remote_player_position_update_from_network(datum_index player_index,
    int32_t update_id, int32_t control_sequence, real x, real y, real z);

    /**
     * Client-side handler for a combined ("total") remote-player biped update: remaps the wire player index in place, refuses anything that is not a live remote player, decodes the message either stateless (zeroed staging buffer, AggregateFieldCodec::decode_compound_field) or incrementally (staging buffer seeded from the player own control record and position, then message_delta_read_changed_subfields), then...
     *
     * @address 0x4e5870
     */
    static void remote_player_total_biped_update_from_network(message_delta_context *context);

    /**
     * Client-side handler for a combined ("total") remote-player vehicle update: remaps the wire player index in place, refuses anything that is not a live remote player, decodes the message stateless or incrementally into one 0x70 staging block (action record plus vehicle body), orthonormalizes and latches the vehicle body onto the player on the baseline path, and hands the two...
     *
     * @address 0x4e5a30
     */
    static void remote_player_total_vehicle_update_from_network(message_delta_context *context);

    /**
     * Client-side handler for a stand-alone remote-player vehicle position update: looks the player up through the remap table, decodes the 0x40-byte vehicle body either stateless (and then orthonormalizes it and latches it onto the player) or as a delta against the copy already on the player, and forwards the result to player_update_client_remote_player_vehicle_update_from_network.
     *
     * @address 0x4e5d60
     */
    static void remote_player_vehicle_position_delta_from_network(message_delta_context *context);

    /**
     * Handles a decoded remote-player vehicle position/orientation update: remaps the vehicle datum, orthonormalizes the orientation basis, and then either queues the record for ordered replay or -- once the client has been out of range twice running -- writes the position and the four vectors straight onto the vehicle object.
     *
     * @address 0x4e6510
     */
    static void remote_player_vehicle_update_from_network(datum_index player_index,
    int32_t update_id, int32_t control_sequence, vehicle_update_body vehicle);

};

}  // namespace halo::networking
