// Phase 4 syntax gate for types/networking.h.
// networking.h also needs game.h, because network_game_session embeds a game_variant
// (session+0x104, which is where 0x4e1820 copies game_engine_pending_variant).
// The host gcc is 64-bit, so a struct holding pointers is 4 bytes larger per pointer than
// in halo.exe; those checks are gated on PTRS32 and only fire with -m32.
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

#define PTRS32 (sizeof(void *) == 4)

// sizes that contain no pointers, so they are exact on any host
typedef char chk_addr[(sizeof(s_network_address) == 0x14) ? 1 : -1];
typedef char chk_addr_fields[(__builtin_offsetof(s_network_address, size) == 0x10
                              && __builtin_offsetof(s_network_address, port) == 0x12) ? 1 : -1];
typedef char chk_conn_stats[(sizeof(network_connection_statistics) == 0x44) ? 1 : -1];
typedef char chk_conn_stats_fields[(__builtin_offsetof(network_connection_statistics, connection_id) == 0x0c
                                    && __builtin_offsetof(network_connection_statistics, interval_bytes_sent) == 0x24
                                    && __builtin_offsetof(network_connection_statistics, interval_packets_sent) == 0x3c) ? 1 : -1];
typedef char chk_summary[(sizeof(network_summary_statistics) == 0x1c) ? 1 : -1];
typedef char chk_reliable[(!PTRS32 || sizeof(network_channel_reliable_slot) == 0x20) ? 1 : -1];
typedef char chk_reliable_fields[(__builtin_offsetof(network_channel_reliable_slot, header_bits) == 0x10
                                  && __builtin_offsetof(network_channel_reliable_slot, body_bits) == 0x14) ? 1 : -1];
typedef char chk_pending[(sizeof(network_pending_connection) == 0x14) ? 1 : -1];
typedef char chk_player_entry[(sizeof(network_player_entry) == 0x20) ? 1 : -1];
typedef char chk_player_entry_fields[(__builtin_offsetof(network_player_entry, color_index) == 0x18
                                      && __builtin_offsetof(network_player_entry, machine_index) == 0x1c
                                      && __builtin_offsetof(network_player_entry, slot_index) == 0x1f) ? 1 : -1];
typedef char chk_search[(sizeof(network_game_search_entry) == 0x130) ? 1 : -1];
typedef char chk_search_fields[(__builtin_offsetof(network_game_search_entry, name) == 0x1c
                                && __builtin_offsetof(network_game_search_entry, game_engine_index) == 0x120
                                && __builtin_offsetof(network_game_search_entry, in_use) == 0x12d) ? 1 : -1];
typedef char chk_ban[(sizeof(ban_list_entry) == 0x38) ? 1 : -1];
typedef char chk_ban_fields[(__builtin_offsetof(ban_list_entry, cd_key_hash) == 0x0d
                             && __builtin_offsetof(ban_list_entry, ban_count) == 0x2e
                             && __builtin_offsetof(ban_list_entry, expiry_time) == 0x34) ? 1 : -1];
typedef char chk_vertex[(sizeof(network_graph_vertex) == 0x18) ? 1 : -1];
typedef char chk_graph[(sizeof(network_bandwidth_graph) == 0x23e0) ? 1 : -1];
typedef char chk_graph_fields[(__builtin_offsetof(network_bandwidth_graph, bits_sent) == 0xbc
                               && __builtin_offsetof(network_bandwidth_graph, pending_sample) == 0xd0
                               && __builtin_offsetof(network_bandwidth_graph, history) == 0xd8
                               && __builtin_offsetof(network_bandwidth_graph, columns) == 0x5d8
                               && __builtin_offsetof(network_bandwidth_graph, displayed_rate) == 0x23dc) ? 1 : -1];
typedef char chk_autopatch[(!PTRS32 || sizeof(autopatch_download_slot) == 0x14) ? 1 : -1];
typedef char chk_filters[(sizeof(server_browser_filters) == 0x06) ? 1 : -1];
typedef char chk_param[(!PTRS32 || sizeof(message_delta_parameter) == 0x0c) ? 1 : -1];

// sizes and offsets that depend on the pointer width
typedef char chk_mutex[(!PTRS32 || sizeof(network_mutex_record) == 0x28) ? 1 : -1];
typedef char chk_thread[(!PTRS32 || sizeof(network_thread_record) == 0x08) ? 1 : -1];
typedef char chk_rxq[(!PTRS32 || sizeof(network_receive_queue) == 0x1c) ? 1 : -1];
typedef char chk_rxq_fields[(!PTRS32 || (__builtin_offsetof(network_receive_queue, socket_key) == 0x08
                                         && __builtin_offsetof(network_receive_queue, flags) == 0x0c
                                         && __builtin_offsetof(network_receive_queue, incoming) == 0x10)) ? 1 : -1];
typedef char chk_list[(!PTRS32 || sizeof(network_channel_list) == 0x114) ? 1 : -1];
typedef char chk_list_fields[(!PTRS32 || (__builtin_offsetof(network_channel_list, entries) == 0x104
                                          && __builtin_offsetof(network_channel_list, last_index) == 0x10c)) ? 1 : -1];
typedef char chk_stream[(!PTRS32 || sizeof(network_channel_stream) == 0x534) ? 1 : -1];
typedef char chk_stream_fields[(!PTRS32 || (__builtin_offsetof(network_channel_stream, capacity_bits) == 0x18
                                            && __builtin_offsetof(network_channel_stream, empty) == 0x1c
                                            && __builtin_offsetof(network_channel_stream, data) == 0x1d)) ? 1 : -1];
typedef char chk_channel[(!PTRS32 || sizeof(network_channel) == 0xae4) ? 1 : -1];
typedef char chk_channel_fields[(!PTRS32 || (__builtin_offsetof(network_channel, outgoing) == 0x010
                                             && __builtin_offsetof(network_channel, retransmit) == 0x544
                                             && __builtin_offsetof(network_channel, reliable_count) == 0xa78
                                             && __builtin_offsetof(network_channel, flags) == 0xa8c
                                             && __builtin_offsetof(network_channel, connected) == 0xa98
                                             && __builtin_offsetof(network_channel, listen_list) == 0xa9c
                                             && __builtin_offsetof(network_channel, children) == 0xaa0
                                             && __builtin_offsetof(network_channel, listening) == 0xae0)) ? 1 : -1];
typedef char chk_session[(!PTRS32 || sizeof(network_game_session) == 0x3b0) ? 1 : -1];
typedef char chk_session_fields[(!PTRS32 || (__builtin_offsetof(network_game_session, server_name) == 0x084
                                             && __builtin_offsetof(network_game_session, variant) == 0x104
                                             && __builtin_offsetof(network_game_session, maximum_players) == 0x19d
                                             && __builtin_offsetof(network_game_session, player_count) == 0x1a0
                                             && __builtin_offsetof(network_game_session, players) == 0x1a2)) ? 1 : -1];
typedef char chk_machine[(!PTRS32 || sizeof(network_machine) == 0x60) ? 1 : -1];
typedef char chk_machine_fields[(!PTRS32 || (__builtin_offsetof(network_machine, machine_id) == 0x0c
                                             && __builtin_offsetof(network_machine, connect_state) == 0x1c
                                             && __builtin_offsetof(network_machine, unknown_5c) == 0x5c)) ? 1 : -1];
typedef char chk_server[(!PTRS32 || sizeof(network_server_globals) == 0xa10) ? 1 : -1];
typedef char chk_server_fields[(!PTRS32 || (__builtin_offsetof(network_server_globals, session) == 0x008
                                            && __builtin_offsetof(network_server_globals, machines) == 0x3b8
                                            && __builtin_offsetof(network_server_globals, password) == 0x9fc
                                            && __builtin_offsetof(network_server_globals, game_over) == 0xa0f)) ? 1 : -1];
typedef char chk_client[(!PTRS32 || sizeof(network_client_globals) == 0xf4c) ? 1 : -1];
typedef char chk_client_fields[(!PTRS32 || (__builtin_offsetof(network_client_globals, channel) == 0xadc
                                            && __builtin_offsetof(network_client_globals, session) == 0xb14
                                            && __builtin_offsetof(network_client_globals, update_history) == 0xf48)) ? 1 : -1];
typedef char chk_hist_node[(!PTRS32 || sizeof(player_update_history_node) == 0x418) ? 1 : -1];
typedef char chk_hist_node_fields[(!PTRS32 || (__builtin_offsetof(player_update_history_node, has_vehicle) == 0x028
                                               && __builtin_offsetof(player_update_history_node, next) == 0x414)) ? 1 : -1];
typedef char chk_hist[(!PTRS32 || sizeof(player_update_history) == 0x2c) ? 1 : -1];
typedef char chk_hist_fields[(!PTRS32 || (__builtin_offsetof(player_update_history, head) == 0x04
                                          && __builtin_offsetof(player_update_history, tail) == 0x08)) ? 1 : -1];
typedef char chk_binding[(!PTRS32 || sizeof(message_delta_field_binding) == 0x10) ? 1 : -1];
typedef char chk_msgdef[(!PTRS32 || __builtin_offsetof(message_delta_definition, fields) == 0x28) ? 1 : -1];
typedef char chk_conn_endpoint[(!PTRS32 || (sizeof(network_connection_endpoint) == 0x28
                                            && __builtin_offsetof(network_connection_endpoint, ready) == 0x22
                                            && __builtin_offsetof(network_connection_endpoint, control_block) == 0x24)) ? 1 : -1];
typedef char chk_conn_attempt[(sizeof(network_connection_attempt_state) == 0x34
                               && __builtin_offsetof(network_connection_attempt_state, session_info) == 0x0e) ? 1 : -1];
typedef char chk_timer_record[(sizeof(network_client_timer_record) == 0x14
                               && __builtin_offsetof(network_client_timer_record, retrigger_ms) == 0x10) ? 1 : -1];
typedef char chk_client_more[(!PTRS32 || (__builtin_offsetof(network_client_globals, connection) == 0xab4
                                          && __builtin_offsetof(network_client_globals, connect_attempt) == 0xae0
                                          && __builtin_offsetof(network_client_globals, state) == 0xeda
                                          && __builtin_offsetof(network_client_globals, timer) == 0xee4
                                          && __builtin_offsetof(network_client_globals, server_address) == 0xef8)) ? 1 : -1];
typedef char chk_resolved_addr[(sizeof(network_resolved_address) == 0x18
                                && __builtin_offsetof(network_resolved_address, unknown_14) == 0x14) ? 1 : -1];
typedef char chk_timer_pair[(sizeof(network_timer_pair) == 0x08) ? 1 : -1];
typedef char chk_map_cycle[(sizeof(network_map_cycle_entry) == 0x08) ? 1 : -1];
typedef char chk_scenario_req[(sizeof(network_scenario_load_request) == 0x10c
                               && __builtin_offsetof(network_scenario_load_request, map_name) == 0x0c
                               && __builtin_offsetof(network_scenario_load_request, salt) == 0x08) ? 1 : -1];
typedef char chk_client_state_enum[(k_network_client_state_established == 4) ? 1 : -1];
typedef char chk_srvlist[(!PTRS32 || sizeof(server_list_globals) == 0x10) ? 1 : -1];

// enum values referenced so the enums are not dead
typedef char chk_enums[(k_network_maximum_machines == 16
                        && k_network_address_size_ipv4 == 4
                        && k_network_error_no_address == -15
                        && k_network_channel_listening == 0x01
                        && k_network_machine_pending == 0x02
                        && k_autopatch_download_ready == 4) ? 1 : -1];

// ---------------------------------------------------------------------------
// types folded into the header by the 2026-09-20 whole-module review pass
// (they used to be per-file typedefs in src/networking/*.c)
// ---------------------------------------------------------------------------
typedef char chk_md_field_binding[(!PTRS32 || (sizeof(message_delta_field_binding) == 0x10
                                  && __builtin_offsetof(message_delta_field_binding, destination_offset) == 0x04
                                  && __builtin_offsetof(message_delta_field_binding, source_offset) == 0x08
                                  && __builtin_offsetof(message_delta_field_binding, initialized) == 0x0c)) ? 1 : -1];
typedef char chk_md_vtable[(!PTRS32 || (sizeof(message_delta_field_type_vtable) == 0x18
                            && __builtin_offsetof(message_delta_field_type_vtable, compute_size) == 0x08
                            && __builtin_offsetof(message_delta_field_type_vtable, initialize) == 0x0c
                            && __builtin_offsetof(message_delta_field_type_vtable, teardown) == 0x10
                            && __builtin_offsetof(message_delta_field_type_vtable, registered) == 0x14)) ? 1 : -1];
typedef char chk_md_field_type[(!PTRS32 || (__builtin_offsetof(message_delta_field_type, encode) == 0x50
                                && __builtin_offsetof(message_delta_field_type, decode) == 0x54
                                && __builtin_offsetof(message_delta_field_type, array_descriptor) == 0x58
                                && __builtin_offsetof(message_delta_field_type, size_bits) == 0x5c
                                && __builtin_offsetof(message_delta_field_type, reserved_bits) == 0x60
                                && __builtin_offsetof(message_delta_field_type, initialized) == 0x64)) ? 1 : -1];
typedef char chk_md_array_desc[(!PTRS32 || sizeof(message_delta_array_descriptor) == 0x0c) ? 1 : -1];
typedef char chk_md_array_list[(!PTRS32 || __builtin_offsetof(message_delta_array_field_list, fields) == 0x04) ? 1 : -1];
typedef char chk_md_scalar_desc[(sizeof(message_delta_scalar_array_descriptor) == 0x04) ? 1 : -1];
typedef char chk_md_ring[(sizeof(message_delta_sample_ring_buffer) == 0x264
                          && __builtin_offsetof(message_delta_sample_ring_buffer, entries) == 0x0c) ? 1 : -1];
typedef char chk_lerp_table[(__builtin_offsetof(vector3d_lerp_table, denominator_mode0) == 0x14) ? 1 : -1];
typedef char chk_waypoint[(__builtin_offsetof(waypoint_table, count_as_float) == 0x18
                           && __builtin_offsetof(waypoint_table, points) == 0x24) ? 1 : -1];
typedef char chk_index_cache[((!PTRS32 || sizeof(network_index_cache) == 0x2c)
                              && __builtin_offsetof(network_index_cache, table) == 0x0c
                              && __builtin_offsetof(network_index_cache, cursor) == 0x24
                              && __builtin_offsetof(network_index_cache, slots) == 0x28) ? 1 : -1];
typedef char chk_custom_options[(__builtin_offsetof(server_browser_custom_options, gametype_like) == 0x1c
                                 && __builtin_offsetof(server_browser_custom_options, unknown_40) == 0x40) ? 1 : -1];
typedef char chk_gametype1[(sizeof(server_browser_gametype1_options) == 0x08
                            && sizeof(server_browser_gametype1_decoded) == 0x08
                            && __builtin_offsetof(server_browser_gametype1_options, time_limit) == 0x04) ? 1 : -1];
typedef char chk_gametype3[(sizeof(server_browser_gametype3_options) == 0x18
                            && __builtin_offsetof(server_browser_gametype3_options, value_04) == 0x04
                            && __builtin_offsetof(server_browser_gametype3_options, value_14) == 0x14) ? 1 : -1];
