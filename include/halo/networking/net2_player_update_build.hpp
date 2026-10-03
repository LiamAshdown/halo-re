/**
 * @file include/halo/networking/net2_player_update_build.hpp
 * Builders and ordering checks for player update packets.
 *
 * The engine type headers (types/*.h) are not include-guarded: include this header after them.
 */
#pragma once

namespace halo::networking {

/**
 * Builders and ordering checks for player update packets.
 *
 * Static behaviour facade; the original C entry points forward to these members.
 */
class PlayerUpdateBuilder {
public:
    /**
     * this module, 0x4e7f90 If plr's queued position-ack sequence number is valid (0..63) and either no ack has been sent yet or the resend interval has elapsed, encodes a rate-limited message-0x23 local-player position acknowledgement from plr's cached fields, logs it, and records the send time. Returns the encoded message size, or 0 if nothing was sent.
     *
     * @address 0x4e81e0
     */
    static int32_t build_local_player_position_update(uint8_t *out_changed, player *plr);

    /**
     * this module, 0x4e7f90 If plr's queued vehicle-ack sequence number is valid (0..63), stages a local_player_vehicle_update_ack from plr's cached fields and plr's vehicle object's transform, and, unless the previous ack is still within the resend interval, encodes and logs a rate-limited message-0x24 acknowledgement. Returns the encoded message size, or 0 if nothing was sent.
     *
     * @address 0x4e82f0
     */
    static int32_t build_local_player_vehicle_update(uint8_t *out_changed, player *plr);

    /**
     * 0x4e1930 Re-encodes and sends player_index's three cached broadcast records (message 0x25's action staging at cache+0x130, 0x27's staging at cache+0x170, and 0x28's staging at cache+0x188) as three fresh baseline (non-delta) messages to machine 1, likely to resync a newly joined client.
     *
     * @address 0x4e7d90
     */
    static void build_player_full_resync_update(uint32_t player_index);

    /**
     * 0x4e1930 Builds a message-0x25 (remote-player orientation/action) update for player_index's broadcast cache from control, rate-limiting or staging it exactly as the vehicle-update siblings do, then (depending on network_broadcast_event_feed_mode) either queues it into the event feed or encodes and broadcasts it immediately to every other connected, established machine.
     *
     * @address 0x4e7890
     */
    static void run_build_remote_player_action_update(uint32_t player_index, uint32_t network_key,
    uint8_t update_id_byte, player_action control);

    /**
     * 0x4e1930 Looks up player_index's controlled unit; if its update id is in range and the unit resolves, dispatches to build_remote_player_vehicle_update (unit has no parent -- it is itself the vehicle) or build_remote_player_vehicle_attachment_update (unit is attached to something else), then broadcasts the encoded result to every established, eligible machine. Falls back to build_remote_player_action_update when the update id is out of range or the unit cannot be resolved.
     *
     * @address 0x4e7b50
     */
    static void build_remote_player_transform_update(uint32_t player_index, player_action *control,
    int32_t network_key);

    /**
     * 0x4ec940, EAX buffer, EDX size Builds a message-0x2a (remote-player secondary/attached-vehicle transform) update for cache from control. Like build_remote_player_vehicle_update, stages a direction vector from control's yaw/pitch, then chases cache's unit (+0x34) to its parent vehicle object and stages that vehicle's own transform. When is_full is set, encodes the full record and refreshes cache's last-sent bookkeeping (offsets 0x17c..0x188); otherwise encodes a delta against the previously cached transform.
     *
     * @address 0x4e86f0
     */
    static void run_build_remote_player_vehicle_attachment_update(uint8_t *cache, uint8_t update_id,
    uint8_t flags, char is_full, player_action *control, int32_t network_key);

    /**
     * 0x4ec940, EAX buffer, EDX size Builds a message-0x29 (remote-player vehicle transform) update for cache from control (the object's current control/aim record). When is_full is set, stages a full record (control plus a direction vector computed from control's yaw/pitch, plus cache's raw position and transform fields), encodes it, and refreshes cache's last-sent bookkeeping. Otherwise stages a delta against cache's previously-sent transform fields and encodes that instead.
     *
     * @address 0x4e84d0
     */
    static void run_build_remote_player_vehicle_update(uint8_t *cache, uint8_t update_id, uint8_t flags,
    char is_full, player_action *control, int32_t network_key);

    /**
     * Applies a decoded remote-player action update (baseline: latch control_source directly onto the player's control record; delta: validate the baseline id and action-index distance, then apply), computing yaw/pitch from the resulting direction vector and pushing an action-queue entry the first time a real action index is seen.
     *
     * @address 0x4e60c0
     */
    static void handle_remote_player_action_update(remote_player_action_state *control_source,
    remote_player_update_header *header, uint8_t is_baseline);

    /**
     * this module, 0x4e5ea0 True unless new_update_id is more than 15 ahead of current_update_id (forward case) or more than 15 behind once wrapped modulo 64 (backward/wrap case), in which case the ack is logged and discarded.
     *
     * @address 0x4e69b0
     */
    static uint8_t is_local_player_update_in_order(int32_t current_update_id, int32_t new_update_id);

    /**
     * this module, 0x4e5f20 True unless new_update_id (a byte-wide wrapping sequence number) is more than 3 ahead of plr's previously recorded remote-player update id, or wrapped more than 3 behind it.
     *
     * @address 0x4e6a20
     */
    static uint8_t is_remote_player_update_in_order(player *plr, uint8_t new_update_id, int32_t update_id);

};

}  // namespace halo::networking
