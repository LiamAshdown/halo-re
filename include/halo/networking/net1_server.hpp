#pragma once

#include "halo/networking/net1_common.hpp"

namespace halo::networking {

/**
 * Non-owning view of the server globals: machines, passwords, challenges, broadcasts and scenario handling.
 */
class ServerView {
public:
    network_server_globals *self;

    explicit constexpr ServerView(network_server_globals *record) : self(record) {}

    char dispatch_bitstream_unit(uint32_t unit, bit_stream *stream, network_machine *machine);
    char drain_bitstream(network_machine *machine);
    uint32_t all_machines_have_player();
    uint32_t any_team_empty();
    static uint32_t broadcast_player_set_changed(uint8_t *param_1);
    uint32_t broadcast_state_snapshot(const uint32_t *record);
    void handle_client_join(int32_t *object_count_passthrough, network_machine *machine, uint8_t bl_passthrough);
    uint32_t handle_info_request(network_machine *machine, uint8_t *record, int32_t length);
    char handle_join_confirm(network_machine *machine, uint8_t *buffer, int32_t length);
    char handle_join_password(network_machine *machine, uint8_t *buffer, int32_t length);
    void handoff_object_ownership(int32_t *object_count_passthrough, network_machine *machine);
    static int32_t host_create();
    void host_dispose();
    static void * host_new();
    static char load_scenario();
    void per_frame_tick(int16_t update_count);
    static void send_message_to_all_machines_ingame(uint8_t *context);
    uint32_t session_finalize_and_add_player(network_player_entry *entry, network_machine *machine);
    int32_t session_reset_defaults();
    uint32_t clear_flag_by_id(int32_t machine_id);
    network_machine * find_by_id(int32_t machine_id);
    uint32_t record_last_sender(int32_t player_index, int32_t quit_tick);
    void advance_connect_state();
    uint8_t any_machine_awaiting_flag();
    static char build_full_game_info_packet(network_machine *machine);
    char build_game_info_packet(network_machine *machine);
    int32_t check_machine_timeout(network_machine *machine);
    int32_t count_connected_machines();
    uint32_t count_machines_and_resolve_address(uint32_t eax_passthrough, s_network_address *address_out, network_receive_queue **connection);
    static void handle_rcon_request(network_player_entry *client, void *message);
    uint8_t heartbeat_tick();
    uint8_t notify_or_resend_challenge(int16_t reason, network_machine *machine);
    void password_get(wchar_t *dest);
    int32_t password_is_set();
    void password_set(const wchar_t *source);
    uint32_t resend_challenge_periodic();
    char service_machines_tick();
    uint32_t status_periodic_print();
    int32_t validate_join_request();
    char broadcast_to_all(int32_t param_1, void *data, int32_t param_3, int32_t param_4, char force, int32_t param_6);
    char broadcast_to_flagged(int32_t body_bit_count, int32_t status_bit, void *data, int32_t immediate, int32_t flush_after, char force, int32_t unused);
    uint8_t send_to_machine(int32_t machine_id, uint32_t status_bit, void *data, uint32_t body_bit_count, uint32_t reliable, uint32_t unknown_a, char force, uint32_t priority);
};

/**
 * Handlers for messages the server receives from machines, one member per message type.
 */
class ServerMessageHandlers {
public:
    network_server_globals *self;

    explicit constexpr ServerMessageHandlers(network_server_globals *record) : self(record) {}

    uint32_t client_game_settings_updated();
    uint32_t client_map_data(uint8_t *record, int32_t length);
    uint32_t client_retry_schedule(network_machine *machine, uint8_t *record, int32_t length);
    uint32_t client_settings_relay(uint8_t *record, int32_t length);
    uint32_t build_version(network_machine *machine, uint8_t *record, int32_t length);
    uint32_t handshake_forward(uint8_t *record, int32_t length);
    uint32_t join_finalize_ack_role2(network_machine *machine, uint8_t *record, int32_t length);
    static uint32_t keepalive(network_channel **channel, int32_t *record);
    uint32_t ping_timestamp(int32_t **message);
    uint32_t player_count_broadcast(uint8_t *record, int32_t length);
    uint32_t player_entry_update(uint8_t *record, int32_t length);
    uint32_t retry_schedule(network_machine *machine, uint8_t *record, int32_t length);
    uint32_t settings_relay(uint8_t *record, int32_t length);
    uint32_t settings_relay_role2(uint8_t *record, int32_t length);
};

/**
 * Host-side operations on the server globals: round reset, full state broadcast and shutdown handling.
 */
class HostServerView {
public:
    network_server_globals *self;

    explicit constexpr HostServerView(network_server_globals *record) : self(record) {}

    void full_state_broadcast();
    void round_reset();
    int32_t send_scenario_announcement();
    static int32_t shutdown_or_defer();
    char update_tick();
};

/**
 * Non-owning view of a connected machine record.
 */
class MachineView {
public:
    network_machine *self;

    explicit constexpr MachineView(network_machine *record) : self(record) {}

    uint8_t reset_state(const char *response);
    void check_build_version(const char *remote_version);
    int32_t reset();
    void timer_start(int32_t duration_ms);
};

}
