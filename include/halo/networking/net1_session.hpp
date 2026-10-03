#pragma once

#include "halo/networking/net1_common.hpp"

namespace halo::networking {

/**
 * Behaviour group for the original `network_*` functions; every member is the original function body moved unchanged.
 */
class GameRuntime {
public:
    GameRuntime() = delete;

    static void broadcast_team_object_updates(int32_t *object_count, uint32_t param_1, int32_t *bytes_sent);
    static void client_apply_position_update(uint8_t *state, uint32_t *packet, void *tick_count, void *object);
    static void client_apply_received_update(network_machine *machine, uint32_t server, void **message);
    static wchar_t * get_random_player_name();
    static int32_t is_active();
    static uint32_t process_incoming_message(int32_t length, network_machine *machine, uint16_t *record, network_server_globals *server);
    static char settings_ack_send(uint8_t *client, int16_t template_row);
    static uint32_t settings_broadcast_send(uint32_t round, uint32_t *record);
    static void start_new_server_from_profile(uint32_t param_1);
    static uint8_t start_new_server_with_name_and_password(uint32_t unused, uint16_t *name, uint16_t *password);
    static void map_cycle_list_broadcast();
    static void disconnect_with_error(int16_t error_code);
};

/**
 * Behaviour group for the original `network_player_*` functions; every member is the original function body moved unchanged.
 */
class GameSessionView {
public:
    network_game_session *self;

    explicit constexpr GameSessionView(network_game_session *record) : self(record) {}

    void generate_unique_random_name(wchar_t *out_name);
    char scenario_load_request();
    void session_reset();
    void assign_random_color(network_player_entry *entry);
    uint32_t add(network_player_entry *incoming);
    char find(network_player_entry *key);
    uint32_t remove(network_player_entry *key);
    uint8_t update_(network_player_entry *incoming);
    uint8_t name_collision_check(uint16_t *candidate_name);
};

/**
 * Behaviour group for the original `network_game_search_*` functions; every member is the original function body moved unchanged.
 */
class SearchEntryView {
public:
    network_game_search_entry *self;

    explicit constexpr SearchEntryView(network_game_search_entry *record) : self(record) {}

    uint8_t entry_is_fresh();
    int32_t results_add_or_update(const uint8_t *announcement);
};

/**
 * Behaviour group for the original `network_object_*` functions; every member is the original function body moved unchanged.
 */
class ObjectOwnership {
public:
    ObjectOwnership() = delete;

    static int32_t owner_team_index_desired(object *obj);
    static void release_ownership_claim(uint8_t slot_index);
};

/**
 * Behaviour group for the original `network_player_entry_*` functions; every member is the original function body moved unchanged.
 */
class PlayerEntryView {
public:
    network_player_entry *self;

    explicit constexpr PlayerEntryView(network_player_entry *record) : self(record) {}

    char validate();
};

/**
 * Behaviour group for the original `network_player_*` functions; every member is the original function body moved unchanged.
 */
class PlayerReports {
public:
    PlayerReports() = delete;

    static void ping_field_update_and_report(void *decode_context);
    static void update_history_log_write_v(const char *format, va_list args);
};

/**
 * Behaviour group for the original `network_session_host_*` functions; every member is the original function body moved unchanged.
 */
class HostSession {
public:
    HostSession() = delete;

    static void cd_key_callback(int32_t game_id, int32_t local_id, int32_t authenticated, const char *message, void *instance);
    static void dispatch_message(int32_t key_id, int32_t index, void *buffer, void *user_data);
    static void dispose();
    static void natneg_callback(int32_t cookie);
    static void natneg_completed(int32_t result, uint32_t socket, const uint8_t *remote_address, void *user_data);
    static void qr2_add_error(int32_t error, char *message, void *user_data);
    static int32_t qr2_count(int32_t key_type, void *user_data);
    static void qr2_key_list(int32_t key_type, void *keybuffer, void *user_data);
    static void qr2_server_key(int32_t key_id, void *buffer, void *user_data);
    static void qr2_team_key(int32_t key_id, int32_t index, void *buffer, void *user_data);
    static uint8_t reject_or_cleanup_client(const char *response, const char *challenge, uint32_t ip, int32_t local_id);
    static int32_t start(void *user_data);
    static void start_info_set(char *host_name, char *map_name, char *variant_name, int32_t game_type);
    static void update_();
};

}
