#include "halo/networking/net1_address.hpp"
#include "halo/networking/net1_bandwidth.hpp"
#include "halo/networking/net1_banlist.hpp"
#include "halo/networking/net1_channel.hpp"
#include "halo/networking/net1_server.hpp"
#include "halo/networking/net1_client.hpp"
#include "halo/networking/net1_runtime.hpp"
#include "halo/networking/net1_decode.hpp"
#include "halo/networking/net1_session.hpp"
#include "halo/networking/net1_timer.hpp"
#include "halo/networking/api.hpp"
#include "halo/core/link.hpp"
#include "halo/networking/vars.hpp"

static auto &network_server = halo::link::ref<network_server_globals *>(halo::networking::vars().network_server);
static auto &network_client = halo::link::ref<network_client_globals *>(halo::networking::vars().network_client);
static auto &network_game_mode = halo::link::ref<int16_t>(halo::networking::vars().network_game_mode);
static auto &network_host_handoff_requested = halo::link::ref<uint8_t>(halo::networking::vars().network_host_handoff_requested);
static auto &network_server_host_valid = halo::link::ref<uint8_t>(halo::networking::vars().network_server_host_valid);
static auto &network_join_error_code = halo::link::ref<int16_t>(halo::networking::vars().network_join_error_code);
static auto &network_join_error_reason = halo::link::ref<uint8_t>(halo::networking::vars().network_join_error_reason);
static auto &network_disconnect_timeout_flag = halo::link::ref<uint8_t>(halo::networking::vars().network_disconnect_timeout_flag);
static auto &network_client_vehicle_ack_enabled = halo::link::ref<uint8_t>(halo::networking::vars().network_client_vehicle_ack_enabled);
static auto &network_game_socket_port = halo::link::ref<uint32_t>(halo::networking::vars().network_game_socket_port);

namespace halo::networking {

Globals &globals()
{
    static Globals instance{::network_server, ::network_client, ::network_game_mode, ::network_host_handoff_requested, ::network_server_host_valid, ::network_join_error_code, ::network_join_error_reason, ::network_disconnect_timeout_flag, ::network_client_vehicle_ack_enabled, ::network_game_socket_port};
    return instance;
}

/**
 * C entry point for halo::networking::AddressText::parse_port; forwards to the C++ implementation unchanged.
 *
 * @address 0x4dc560
 */
char network_address_parse_port(char *address_string, int32_t *port_out)
{
    return halo::networking::AddressText::parse_port(address_string, port_out);
}

/**
 * C entry point for halo::networking::AddressText::string_is_valid; forwards to the C++ implementation unchanged.
 *
 * @address 0x4dc730
 */
char network_address_string_is_valid(char *address_string)
{
    return halo::networking::AddressText::string_is_valid(address_string);
}

/**
 * C entry point for halo::networking::AddressText::string_normalize; forwards to the C++ implementation unchanged.
 *
 * @address 0x4dc5e0
 */
char network_address_string_normalize(char *address_string, char *out_buffer, uint8_t *out_is_any)
{
    return halo::networking::AddressText::string_normalize(address_string, out_buffer, out_is_any);
}

/**
 * C entry point for halo::networking::AddressView::to_string; forwards to the C++ implementation unchanged.
 *
 * @address 0x440570
 */
char * network_address_to_string(s_network_address *addr)
{
    return halo::networking::AddressView(addr).to_string();
}

/**
 * C entry point for halo::networking::BandwidthMonitor::direction_name_to_index; forwards to the C++ implementation unchanged.
 *
 * @address 0x4d8a50
 */
int32_t network_bandwidth_direction_name_to_index(const char *name)
{
    return halo::networking::BandwidthMonitor::direction_name_to_index(name);
}

/**
 * C entry point for halo::networking::BandwidthMonitor::accumulate_received; forwards to the C++ implementation unchanged.
 *
 * @address 0x4d7a50
 */
void network_bandwidth_graph_accumulate_received(int32_t byte_count, int32_t packet_count)
{
    halo::networking::BandwidthMonitor::accumulate_received(byte_count, packet_count);
}

/**
 * C entry point for halo::networking::BandwidthMonitor::accumulate_sent; forwards to the C++ implementation unchanged.
 *
 * @address 0x4d79d0
 */
void network_bandwidth_graph_accumulate_sent(int32_t byte_count, int32_t packet_count)
{
    halo::networking::BandwidthMonitor::accumulate_sent(byte_count, packet_count);
}

/**
 * C entry point for halo::networking::BandwidthMonitor::reset; forwards to the C++ implementation unchanged.
 *
 * @address 0x4d7980
 */
uint32_t network_bandwidth_graph_reset(void)
{
    return halo::networking::BandwidthMonitor::reset();
}

/**
 * C entry point for halo::networking::BandwidthMonitor::set_units_command; forwards to the C++ implementation unchanged.
 *
 * @address 0x4d7d90
 */
uint32_t network_bandwidth_graph_set_units_command(const char *units_name, const char *direction_name)
{
    return halo::networking::BandwidthMonitor::set_units_command(units_name, direction_name);
}

/**
 * C entry point for halo::networking::BandwidthMonitor::update_; forwards to the C++ implementation unchanged.
 *
 * @address 0x4d7ad0
 */
void network_bandwidth_graph_update(void)
{
    halo::networking::BandwidthMonitor::update_();
}

/**
 * C entry point for halo::networking::BandwidthMonitor::unit_name_to_index; forwards to the C++ implementation unchanged.
 *
 * @address 0x4d8a20
 */
int32_t network_bandwidth_unit_name_to_index(const char *name)
{
    return halo::networking::BandwidthMonitor::unit_name_to_index(name);
}

/**
 * C entry point for halo::networking::BandwidthGraphView::find_peak_sample; forwards to the C++ implementation unchanged.
 *
 * @address 0x4d8140
 */
int32_t network_bandwidth_graph_find_peak_sample(int32_t *out_peak_countdown, network_bandwidth_graph *graph)
{
    return halo::networking::BandwidthGraphView(graph).find_peak_sample(out_peak_countdown);
}

/**
 * C entry point for halo::networking::BandwidthGraphView::instance_history_reset; forwards to the C++ implementation unchanged.
 *
 * @address 0x4d8080
 */
void network_bandwidth_graph_instance_history_reset(network_bandwidth_graph *graph)
{
    halo::networking::BandwidthGraphView(graph).instance_history_reset();
}

/**
 * C entry point for halo::networking::BandwidthGraphView::instance_init; forwards to the C++ implementation unchanged.
 *
 * @address 0x4d7de0
 */
void network_bandwidth_graph_instance_init(network_bandwidth_graph *graph, int32_t units_index, int32_t direction_index)
{
    halo::networking::BandwidthGraphView(graph).instance_init(units_index, direction_index);
}

/**
 * C entry point for halo::networking::BandwidthGraphView::instance_update_layout; forwards to the C++ implementation unchanged.
 *
 * @address 0x4d7e20
 */
void network_bandwidth_graph_instance_update_layout(network_bandwidth_graph *graph, uint8_t force_refresh)
{
    halo::networking::BandwidthGraphView(graph).instance_update_layout(force_refresh);
}

/**
 * C entry point for halo::networking::BandwidthGraphView::new_sample; forwards to the C++ implementation unchanged.
 *
 * @address 0x4d8430
 */
void network_bandwidth_graph_new_sample(network_bandwidth_graph *graph)
{
    halo::networking::BandwidthGraphView(graph).new_sample();
}

/**
 * C entry point for halo::networking::BandwidthGraphView::tick; forwards to the C++ implementation unchanged.
 *
 * @address 0x4d84d0
 */
void network_bandwidth_graph_tick(network_bandwidth_graph *graph)
{
    halo::networking::BandwidthGraphView(graph).tick();
}

/**
 * C entry point for halo::networking::BandwidthGraphView::update_columns; forwards to the C++ implementation unchanged.
 *
 * @address 0x4d81c0
 */
void network_bandwidth_graph_update_columns(int32_t new_sample, network_bandwidth_graph *graph)
{
    halo::networking::BandwidthGraphView(graph).update_columns(new_sample);
}

/**
 * C entry point for halo::networking::BandwidthGraphView::rate_compute; forwards to the C++ implementation unchanged.
 *
 * @address 0x4d8540
 */
void network_bandwidth_rate_compute(network_bandwidth_graph *graph)
{
    halo::networking::BandwidthGraphView(graph).rate_compute();
}

/**
 * C entry point for halo::networking::BandwidthGraphView::overlay_draw; forwards to the C++ implementation unchanged.
 *
 * @address 0x4d8620
 */
void network_stats_overlay_draw(network_bandwidth_graph *graph)
{
    halo::networking::BandwidthGraphView(graph).overlay_draw();
}

/**
 * C entry point for halo::networking::Banlist::add_ban; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e35c0
 */
uint8_t network_banlist_add_ban(int32_t identity_lookup_key, int32_t duration_override_seconds, network_player_entry *target_player)
{
    return halo::networking::Banlist::add_ban(identity_lookup_key, duration_override_seconds, target_player);
}

/**
 * C entry point for halo::networking::Banlist::load; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e3160
 */
void network_banlist_load(void)
{
    halo::networking::Banlist::load();
}

/**
 * C entry point for halo::networking::Banlist::print; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e34e0
 */
void network_banlist_print(void)
{
    halo::networking::Banlist::print();
}

/**
 * C entry point for halo::networking::Banlist::autoban_player; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e36e0
 */
uint8_t network_session_autoban_player(datum_index player_handle)
{
    return halo::networking::Banlist::autoban_player(player_handle);
}

/**
 * C entry point for halo::networking::Banlist::save; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e3380
 */
void network_banlist_save(void)
{
    halo::networking::Banlist::save();
}

/**
 * C entry point for halo::networking::ChannelFactory::clear_buffer_pair_pool; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e3ed0
 */
void network_buffer_pair_pool_clear(void)
{
    halo::networking::ChannelFactory::clear_buffer_pair_pool();
}

/**
 * C entry point for halo::networking::ChannelFactory::create_list; forwards to the C++ implementation unchanged.
 *
 * @address 0x441960
 */
network_channel_list * network_channel_list_new(int16_t requested_capacity)
{
    return halo::networking::ChannelFactory::create_list(requested_capacity);
}

/**
 * C entry point for halo::networking::ChannelFactory::create_channel; forwards to the C++ implementation unchanged.
 *
 * @address 0x4dc9b0
 */
network_channel * network_channel_new(uint32_t flags)
{
    return halo::networking::ChannelFactory::create_channel(flags);
}

/**
 * C entry point for halo::networking::ChannelFactory::create_child; forwards to the C++ implementation unchanged.
 *
 * @address 0x4dd430
 */
network_channel * network_channel_new_child(network_receive_queue *endpoint)
{
    return halo::networking::ChannelFactory::create_child(endpoint);
}

/**
 * C entry point for halo::networking::ChannelFactory::close_all; forwards to the C++ implementation unchanged.
 *
 * @address 0x441480
 */
void network_channels_close(void)
{
    halo::networking::ChannelFactory::close_all();
}

/**
 * C entry point for halo::networking::ChannelFactory::open_all; forwards to the C++ implementation unchanged.
 *
 * @address 0x441300
 */
void network_channels_open(void)
{
    halo::networking::ChannelFactory::open_all();
}

/**
 * C entry point for halo::networking::ChannelFactory::close_all_handles; forwards to the C++ implementation unchanged.
 *
 * @address 0x441bb0
 */
void network_handle_registry_close_all(void)
{
    halo::networking::ChannelFactory::close_all_handles();
}

/**
 * C entry point for halo::networking::ChannelFactory::allocate_mutex_slot; forwards to the C++ implementation unchanged.
 *
 * @address 0x440420
 */
network_mutex_record * network_mutex_slot_allocate(void)
{
    return halo::networking::ChannelFactory::allocate_mutex_slot();
}

/**
 * C entry point for halo::networking::ChannelFactory::create_receive_queue; forwards to the C++ implementation unchanged.
 *
 * @address 0x441bf0
 */
network_receive_queue * network_receive_queue_new(void)
{
    return halo::networking::ChannelFactory::create_receive_queue();
}

/**
 * C entry point for halo::networking::ChannelFactory::create_thread; forwards to the C++ implementation unchanged.
 *
 * @address 0x440460
 */
int32_t network_thread_create(uint8_t flags, void *start_address, void *parameter, network_thread_record **out_handle)
{
    return halo::networking::ChannelFactory::create_thread(flags, start_address, parameter, out_handle);
}

/**
 * C entry point for halo::networking::ReceiveQueueView::get_remote_address; forwards to the C++ implementation unchanged.
 *
 * @address 0x441ce0
 */
int16_t network_channel_get_remote_address(s_network_address *address, network_receive_queue *queue)
{
    return halo::networking::ReceiveQueueView(queue).get_remote_address(address);
}

/**
 * C entry point for halo::networking::ReceiveQueueView::start; forwards to the C++ implementation unchanged.
 *
 * @address 0x442170
 */
uint32_t network_listen_start(network_receive_queue *queue)
{
    return halo::networking::ReceiveQueueView(queue).start();
}

/**
 * C entry point for halo::networking::ReceiveQueueView::close_socket; forwards to the C++ implementation unchanged.
 *
 * @address 0x442040
 */
void network_receive_queue_close_socket(network_receive_queue *queue)
{
    halo::networking::ReceiveQueueView(queue).close_socket();
}

/**
 * C entry point for halo::networking::ReceiveQueueView::release; forwards to the C++ implementation unchanged.
 *
 * @address 0x441c80
 */
void network_receive_queue_free(network_receive_queue *queue)
{
    halo::networking::ReceiveQueueView(queue).release();
}

/**
 * C entry point for halo::networking::ChannelCallbacks::on_connected; forwards to the C++ implementation unchanged.
 *
 * @address 0x441e00
 */
void network_channel_connected_callback(void *connection, int32_t result, const uint8_t *message, int32_t length)
{
    halo::networking::ChannelCallbacks::on_connected(connection, result, message, length);
}

/**
 * C entry point for halo::networking::ChannelCallbacks::on_receive_dump; forwards to the C++ implementation unchanged.
 *
 * @address 0x441020
 */
void network_channel_gap_441020(void *socket, void *connection, uint32_t ip, uint16_t port, int32_t reset, const void *message, int32_t length, int32_t reliable, int32_t resend)
{
    halo::networking::ChannelCallbacks::on_receive_dump(socket, connection, ip, port, reset, message, length, reliable, resend);
}

/**
 * C entry point for halo::networking::ChannelCallbacks::on_send_dump; forwards to the C++ implementation unchanged.
 *
 * @address 0x441040
 */
void network_channel_gap_441040(void *socket, void *connection, uint32_t ip, uint16_t port, int32_t reset, const void *message, int32_t length)
{
    halo::networking::ChannelCallbacks::on_send_dump(socket, connection, ip, port, reset, message, length);
}

/**
 * C entry point for halo::networking::ChannelCallbacks::on_socket_error; forwards to the C++ implementation unchanged.
 *
 * @address 0x441060
 */
void network_channel_gap_441060(void *socket)
{
    halo::networking::ChannelCallbacks::on_socket_error(socket);
}

/**
 * C entry point for halo::networking::ChannelCallbacks::on_game_socket_unrecognized; forwards to the C++ implementation unchanged.
 *
 * @address 0x4410b0
 */
int32_t network_channel_gap_4410b0(void *socket, uint32_t ip, uint16_t port, const uint8_t *message, uint32_t length)
{
    return halo::networking::ChannelCallbacks::on_game_socket_unrecognized(socket, ip, port, message, length);
}

/**
 * C entry point for halo::networking::ChannelCallbacks::on_query_socket_unrecognized; forwards to the C++ implementation unchanged.
 *
 * @address 0x441200
 */
int32_t network_channel_gap_441200(void *socket, uint32_t ip, uint16_t port, const uint8_t *message, uint32_t length)
{
    return halo::networking::ChannelCallbacks::on_query_socket_unrecognized(socket, ip, port, message, length);
}

/**
 * C entry point for halo::networking::ChannelCallbacks::on_connection_error; forwards to the C++ implementation unchanged.
 *
 * @address 0x441f30
 */
void network_channel_gap_441f30(void *connection)
{
    halo::networking::ChannelCallbacks::on_connection_error(connection);
}

/**
 * C entry point for halo::networking::ChannelCallbacks::on_server_browser_list; forwards to the C++ implementation unchanged.
 *
 * @address 0x4ba660
 */
void network_channel_gap_4ba660(void *sb, uint32_t reason, void *server, void *instance)
{
    halo::networking::ChannelCallbacks::on_server_browser_list(sb, reason, server, instance);
}

/**
 * C entry point for halo::networking::ChannelView::destroy; forwards to the C++ implementation unchanged.
 *
 * @address 0x4dcae0
 */
void network_channel_delete(network_channel *channel)
{
    halo::networking::ChannelView(channel).destroy();
}

/**
 * C entry point for halo::networking::ChannelView::incoming_read_item; forwards to the C++ implementation unchanged.
 *
 * @address 0x4dcf10
 */
int32_t network_channel_incoming_read_item(network_channel *channel, uint8_t *destination, int32_t *out_bit_offset, int32_t *out_remaining_bits, s_network_address *out_address, int32_t max_item_bits)
{
    return halo::networking::ChannelView(channel).incoming_read_item(destination, out_bit_offset, out_remaining_bits, out_address, max_item_bits);
}

/**
 * C entry point for halo::networking::ChannelView::listen_service; forwards to the C++ implementation unchanged.
 *
 * @address 0x4dd4e0
 */
char network_channel_listen_service(network_channel *channel, network_channel **out_new_child)
{
    return halo::networking::ChannelView(channel).listen_service(out_new_child);
}

/**
 * C entry point for halo::networking::ChannelView::queue_message; forwards to the C++ implementation unchanged.
 *
 * @address 0x4dce40
 */
char network_channel_queue_message(network_channel *channel, uint32_t header_value, uint32_t body_value, int32_t header_bit_count, char immediate, char flush_after, int32_t body_bit_count)
{
    return halo::networking::ChannelView(channel).queue_message(header_value, body_value, header_bit_count, immediate, flush_after, body_bit_count);
}

/**
 * C entry point for halo::networking::ChannelView::record_timestamp; forwards to the C++ implementation unchanged.
 *
 * @address 0x4dd930
 */
void network_channel_record_timestamp(network_channel *channel)
{
    halo::networking::ChannelView(channel).record_timestamp();
}

/**
 * C entry point for halo::networking::ChannelView::reliable_pool_ensure_capacity; forwards to the C++ implementation unchanged.
 *
 * @address 0x4dcc30
 */
int32_t network_channel_reliable_pool_ensure_capacity(network_channel *channel, int32_t body_capacity_needed, int32_t header_capacity_needed)
{
    return halo::networking::ChannelView(channel).reliable_pool_ensure_capacity(body_capacity_needed, header_capacity_needed);
}

/**
 * C entry point for halo::networking::ChannelView::reliable_pool_store; forwards to the C++ implementation unchanged.
 *
 * @address 0x4dcdb0
 */
void network_channel_reliable_pool_store(network_channel *channel, uint8_t *body_data, uint8_t *header_data, int32_t priority, uint32_t header_bits, uint32_t body_bits)
{
    halo::networking::ChannelView(channel).reliable_pool_store(body_data, header_data, priority, header_bits, body_bits);
}

/**
 * C entry point for halo::networking::ChannelView::remote_address_or_default; forwards to the C++ implementation unchanged.
 *
 * @address 0x4dd390
 */
void network_channel_remote_address_or_default(network_channel *channel, network_resolved_address *out_address)
{
    halo::networking::ChannelView(channel).remote_address_or_default(out_address);
}

/**
 * C entry point for halo::networking::ChannelView::remove_child; forwards to the C++ implementation unchanged.
 *
 * @address 0x4dd090
 */
int32_t network_channel_remove_child(network_channel *parent, network_channel *child)
{
    return halo::networking::ChannelView(parent).remove_child(child);
}

/**
 * C entry point for halo::networking::ChannelView::scan_retransmit_timeouts; forwards to the C++ implementation unchanged.
 *
 * @address 0x4dd9d0
 */
void network_channel_scan_retransmit_timeouts(network_channel *channel)
{
    halo::networking::ChannelView(channel).scan_retransmit_timeouts();
}

/**
 * C entry point for halo::networking::ChannelView::service; forwards to the C++ implementation unchanged.
 *
 * @address 0x4dd110
 */
char network_channel_service(network_channel *channel, int32_t timeout_ms, network_channel **out_new_child)
{
    return halo::networking::ChannelView(channel).service(timeout_ms, out_new_child);
}

/**
 * C entry point for halo::networking::ChannelView::service_close_if_disconnected; forwards to the C++ implementation unchanged.
 *
 * @address 0x4dd3f0
 */
int32_t network_channel_service_close_if_disconnected(network_channel *channel)
{
    return halo::networking::ChannelView(channel).service_close_if_disconnected();
}

/**
 * C entry point for halo::networking::ChannelView::service_light; forwards to the C++ implementation unchanged.
 *
 * @address 0x4dd240
 */
char network_channel_service_light(network_channel *channel, int32_t timeout_ms, network_channel **out_new_child)
{
    return halo::networking::ChannelView(channel).service_light(timeout_ms, out_new_child);
}

/**
 * C entry point for halo::networking::ChannelView::service_retransmit_only; forwards to the C++ implementation unchanged.
 *
 * @address 0x4dd330
 */
int32_t network_channel_service_retransmit_only(network_channel *channel)
{
    return halo::networking::ChannelView(channel).service_retransmit_only();
}

/**
 * C entry point for halo::networking::ChannelView::short_disconnect_timeout; forwards to the C++ implementation unchanged.
 *
 * @address 0x4ddd20
 */
int32_t network_channel_short_disconnect_timeout(void)
{
    return halo::networking::ChannelView::short_disconnect_timeout();
}

/**
 * C entry point for halo::networking::ChannelView::transmit; forwards to the C++ implementation unchanged.
 *
 * @address 0x4dd730
 */
char network_channel_transmit(network_channel *channel)
{
    return halo::networking::ChannelView(channel).transmit();
}

/**
 * C entry point for halo::networking::ChannelKeys::close; forwards to the C++ implementation unchanged.
 *
 * @address 0x4de8c0
 */
int32_t network_channel_key_close(network_player_entry *entry, datum_index requested_handle)
{
    return halo::networking::ChannelKeys::close(entry, requested_handle);
}

/**
 * C entry point for halo::networking::ChannelKeys::open; forwards to the C++ implementation unchanged.
 *
 * @address 0x4de870
 */
int32_t network_channel_key_open(network_player_entry *entry)
{
    return halo::networking::ChannelKeys::open(entry);
}

/**
 * C entry point for halo::networking::ChannelKeys::resolve_target; forwards to the C++ implementation unchanged.
 *
 * @address 0x4ddcc0
 */
uint8_t network_channel_key_resolve_target(network_player_entry *entry)
{
    return halo::networking::ChannelKeys::resolve_target(entry);
}

/**
 * C entry point for halo::networking::ChannelKeys::send_state; forwards to the C++ implementation unchanged.
 *
 * @address 0x4de950
 */
int32_t network_channel_key_send_state(network_client_globals *client, int32_t **entry)
{
    return halo::networking::ChannelKeys::send_state(client, entry);
}

/**
 * C entry point for halo::networking::ChannelListView::add; forwards to the C++ implementation unchanged.
 *
 * @address 0x441a40
 */
int32_t network_channel_list_add(network_receive_queue *entry, network_channel_list *list)
{
    return halo::networking::ChannelListView(list).add(entry);
}

/**
 * C entry point for halo::networking::ChannelListView::mark_readable; forwards to the C++ implementation unchanged.
 *
 * @address 0x4419d0
 */
int32_t network_channel_list_mark_readable(network_channel_list *list)
{
    return halo::networking::ChannelListView(list).mark_readable();
}

/**
 * C entry point for halo::networking::ChannelListView::remove; forwards to the C++ implementation unchanged.
 *
 * @address 0x441b00
 */
int32_t network_channel_list_remove(network_receive_queue *entry, network_channel_list *list)
{
    return halo::networking::ChannelListView(list).remove(entry);
}

/**
 * C entry point for halo::networking::ChannelStreamView::flush; forwards to the C++ implementation unchanged.
 *
 * @address 0x4ddb60
 */
char network_channel_stream_flush(network_channel_stream *stream, network_channel *channel, char mode)
{
    return halo::networking::ChannelStreamView(stream).flush(channel, mode);
}

/**
 * C entry point for halo::networking::ChannelStreamView::init; forwards to the C++ implementation unchanged.
 *
 * @address 0x4dd980
 */
void network_channel_stream_init(network_channel_stream *stream)
{
    halo::networking::ChannelStreamView(stream).init();
}

/**
 * C entry point for halo::networking::ListenerCallbacks::accept_pending_connection; forwards to the C++ implementation unchanged.
 *
 * @address 0x4421b0
 */
network_receive_queue * network_listen_accept_pending_connection(void)
{
    return halo::networking::ListenerCallbacks::accept_pending_connection();
}

/**
 * C entry point for halo::networking::ListenerCallbacks::connection_request_handler; forwards to the C++ implementation unchanged.
 *
 * @address 0x442090
 */
void network_listen_connection_request_handler(int32_t listen_handle, int32_t reply_socket, uint32_t remote_address, uint32_t remote_port_raw, int32_t transport_handle, uint32_t *payload, uint32_t payload_length)
{
    halo::networking::ListenerCallbacks::connection_request_handler(listen_handle, reply_socket, remote_address, remote_port_raw, transport_handle, payload, payload_length);
}

/**
 * C entry point for halo::networking::ListenerCallbacks::reject_pending_connection; forwards to the C++ implementation unchanged.
 *
 * @address 0x442250
 */
uint32_t network_listen_reject_pending_connection(int32_t reject_code)
{
    return halo::networking::ListenerCallbacks::reject_pending_connection(reject_code);
}

/**
 * C entry point for halo::networking::ReceiveQueueView::attempt_connect; forwards to the C++ implementation unchanged.
 *
 * @address 0x441f60
 */
int16_t network_channel_attempt_connect(s_network_address *address, network_receive_queue *queue, int32_t unused_param_1, uint8_t use_query_socket)
{
    return halo::networking::ReceiveQueueView(queue).attempt_connect(address, unused_param_1, use_query_socket);
}

/**
 * C entry point for halo::networking::ListenerCallbacks::reject_pending_connection_callback; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e1410
 */
int32_t network_session_reject_pending_connection_callback(void *unused, int32_t reject_code)
{
    return halo::networking::ListenerCallbacks::reject_pending_connection_callback(unused, reject_code);
}

/**
 * C entry point for halo::networking::ChannelCallbacks::on_receive; forwards to the C++ implementation unchanged.
 *
 * @address 0x441ed0
 */
void network_channel_receive_callback(void *handle, uint8_t *data, int32_t length)
{
    halo::networking::ChannelCallbacks::on_receive(handle, data, length);
}

/**
 * C entry point for halo::networking::ServerView::dispatch_bitstream_unit; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e18b0
 */
char network_channel_dispatch_bitstream_unit(network_server_globals *server, uint32_t unit, bit_stream *stream, network_machine *machine)
{
    return halo::networking::ServerView(server).dispatch_bitstream_unit(unit, stream, machine);
}

/**
 * C entry point for halo::networking::ServerView::drain_bitstream; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e1290
 */
char network_channel_drain_bitstream(network_server_globals *server, network_machine *machine)
{
    return halo::networking::ServerView(server).drain_bitstream(machine);
}

/**
 * C entry point for halo::networking::ServerView::all_machines_have_player; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e04f0
 */
uint32_t network_game_all_machines_have_player(network_server_globals *server)
{
    return halo::networking::ServerView(server).all_machines_have_player();
}

/**
 * C entry point for halo::networking::ServerView::any_team_empty; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e0480
 */
uint32_t network_game_any_team_empty(network_server_globals *server)
{
    return halo::networking::ServerView(server).any_team_empty();
}

/**
 * C entry point for halo::networking::ServerView::broadcast_player_set_changed; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e1bf0
 */
uint32_t network_game_broadcast_player_set_changed(uint8_t *param_1)
{
    return halo::networking::ServerView::broadcast_player_set_changed(param_1);
}

/**
 * C entry point for halo::networking::ServerView::broadcast_state_snapshot; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e1b50
 */
uint32_t network_game_broadcast_state_snapshot(const uint32_t *record, network_server_globals *server)
{
    return halo::networking::ServerView(server).broadcast_state_snapshot(record);
}

/**
 * C entry point for halo::networking::ServerView::handle_client_join; forwards to the C++ implementation unchanged.
 *
 * @address 0x4dfc90
 */
void network_game_server_handle_client_join(int32_t *object_count_passthrough, network_server_globals *server, network_machine *machine, uint8_t bl_passthrough)
{
    halo::networking::ServerView(server).handle_client_join(object_count_passthrough, machine, bl_passthrough);
}

/**
 * C entry point for halo::networking::ServerView::handle_info_request; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e2700
 */
uint32_t network_game_server_handle_info_request(network_server_globals *server, network_machine *machine, network_message_record *record, int32_t length)
{
    return halo::networking::ServerView(server).handle_info_request(machine, record, length);
}

/**
 * C entry point for halo::networking::ServerView::handoff_object_ownership; forwards to the C++ implementation unchanged.
 *
 * @address 0x4dfa10
 */
void network_game_server_handoff_object_ownership(int32_t *object_count_passthrough, network_server_globals *server, network_machine *machine)
{
    halo::networking::ServerView(server).handoff_object_ownership(object_count_passthrough, machine);
}

/**
 * C entry point for halo::networking::ServerView::host_create; forwards to the C++ implementation unchanged.
 *
 * @address 0x4ddd40
 */
int32_t network_game_server_host_create(void)
{
    return halo::networking::ServerView::host_create();
}

/**
 * C entry point for halo::networking::ServerView::host_dispose; forwards to the C++ implementation unchanged.
 *
 * @address 0x4deda0
 */
void network_game_server_host_dispose(network_server_globals *host)
{
    halo::networking::ServerView(host).host_dispose();
}

/**
 * C entry point for halo::networking::ServerView::host_new; forwards to the C++ implementation unchanged.
 *
 * @address 0x4dec40
 */
void * network_game_server_host_new(void)
{
    return halo::networking::ServerView::host_new();
}

/**
 * C entry point for halo::networking::ServerView::per_frame_tick; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e03c0
 */
void network_game_server_per_frame_tick(int16_t update_count, network_server_globals *server)
{
    halo::networking::ServerView(server).per_frame_tick(update_count);
}

/**
 * C entry point for halo::networking::ServerView::send_message_to_all_machines_ingame; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e4ef0
 */
void network_game_server_send_message_to_all_machines_ingame(uint8_t *context)
{
    halo::networking::ServerView::send_message_to_all_machines_ingame(context);
}

/**
 * C entry point for halo::networking::ServerView::session_finalize_and_add_player; forwards to the C++ implementation unchanged.
 *
 * @address 0x4df840
 */
uint32_t network_game_session_finalize_and_add_player(network_player_entry *entry, network_server_globals *server, network_machine *machine)
{
    return halo::networking::ServerView(server).session_finalize_and_add_player(entry, machine);
}

/**
 * C entry point for halo::networking::ServerView::session_reset_defaults; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e1820
 */
int32_t network_game_session_reset_defaults(network_server_globals *server)
{
    return halo::networking::ServerView(server).session_reset_defaults();
}

/**
 * C entry point for halo::networking::ServerView::clear_flag_by_id; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e0b90
 */
uint32_t network_machine_clear_flag_by_id(network_server_globals *server, int32_t machine_id)
{
    return halo::networking::ServerView(server).clear_flag_by_id(machine_id);
}

/**
 * C entry point for halo::networking::ServerView::find_by_id; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e0810
 */
network_machine * network_machine_find_by_id(network_server_globals *server, int32_t machine_id)
{
    return halo::networking::ServerView(server).find_by_id(machine_id);
}

/**
 * C entry point for halo::networking::ServerView::advance_connect_state; forwards to the C++ implementation unchanged.
 *
 * @address 0x4df290
 */
void network_server_advance_connect_state(network_server_globals *server)
{
    halo::networking::ServerView(server).advance_connect_state();
}

/**
 * C entry point for halo::networking::ServerView::any_machine_awaiting_flag; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e14e0
 */
uint8_t network_server_any_machine_awaiting_flag(network_server_globals *server)
{
    return halo::networking::ServerView(server).any_machine_awaiting_flag();
}

/**
 * C entry point for halo::networking::ServerView::count_connected_machines; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e1880
 */
int32_t network_server_count_connected_machines(network_server_globals *server)
{
    return halo::networking::ServerView(server).count_connected_machines();
}

/**
 * C entry point for halo::networking::ServerView::count_machines_and_resolve_address; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e0d30
 */
uint8_t network_server_count_machines_and_resolve_address(network_server_globals *server, network_channel *channel)
{
    return halo::networking::ServerView(server).count_machines_and_resolve_address(channel);
}

/**
 * C entry point for halo::networking::ServerView::handle_rcon_request; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e4f00
 */
void network_server_handle_rcon_request(network_player_entry *client, void *message)
{
    halo::networking::ServerView::handle_rcon_request(client, message);
}

/**
 * C entry point for halo::networking::ServerView::notify_or_resend_challenge; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e0af0
 */
uint8_t network_server_notify_or_resend_challenge(int16_t reason, network_machine *machine, network_server_globals *server)
{
    return halo::networking::ServerView(server).notify_or_resend_challenge(reason, machine);
}

/**
 * C entry point for halo::networking::ServerView::password_get; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e0930
 */
void network_server_password_get(network_server_globals *server, wchar_t *dest)
{
    halo::networking::ServerView(server).password_get(dest);
}

/**
 * C entry point for halo::networking::ServerView::password_is_set; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e08e0
 */
int32_t network_server_password_is_set(network_server_globals *server)
{
    return halo::networking::ServerView(server).password_is_set();
}

/**
 * C entry point for halo::networking::ServerView::password_set; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e0910
 */
void network_server_password_set(const wchar_t *source, network_server_globals *server)
{
    halo::networking::ServerView(server).password_set(source);
}

/**
 * C entry point for halo::networking::ServerView::status_periodic_print; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e1520
 */
uint32_t network_server_status_periodic_print(network_server_globals *server)
{
    return halo::networking::ServerView(server).status_periodic_print();
}

/**
 * C entry point for halo::networking::ServerView::validate_join_request; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e0850
 */
int32_t network_server_validate_join_request(network_server_globals *server)
{
    return halo::networking::ServerView(server).validate_join_request();
}

/**
 * C entry point for halo::networking::ServerView::broadcast_to_all; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e19c0
 */
char network_session_broadcast_to_all(network_server_globals *server, int32_t param_1, void *data, int32_t param_3, int32_t param_4, char force, int32_t param_6)
{
    return halo::networking::ServerView(server).broadcast_to_all(param_1, data, param_3, param_4, force, param_6);
}

/**
 * C entry point for halo::networking::ServerMessageHandlers::client_map_data; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e2790
 */
uint32_t network_game_client_handle_map_data(network_server_globals *server, network_message_record *record, int32_t length)
{
    return halo::networking::ServerMessageHandlers(server).client_map_data(record, length);
}

/**
 * C entry point for halo::networking::ServerMessageHandlers::client_retry_schedule; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e2870
 */
uint32_t network_game_client_handle_retry_schedule(network_server_globals *server, network_machine *machine, network_message_record *record, int32_t length)
{
    return halo::networking::ServerMessageHandlers(server).client_retry_schedule(machine, record, length);
}

/**
 * C entry point for halo::networking::ServerMessageHandlers::client_settings_relay; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e2810
 */
uint32_t network_game_client_handle_settings_relay(network_server_globals *server, network_message_record *record, int32_t length)
{
    return halo::networking::ServerMessageHandlers(server).client_settings_relay(record, length);
}

/**
 * C entry point for halo::networking::ServerMessageHandlers::build_version; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e2630
 */
uint32_t network_game_message_handle_build_version(network_server_globals *server, network_machine *machine, network_message_record *record, int32_t length)
{
    return halo::networking::ServerMessageHandlers(server).build_version(machine, record, length);
}

/**
 * C entry point for halo::networking::ServerMessageHandlers::handshake_forward; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e25e0
 */
uint32_t network_game_message_handle_handshake_forward(network_server_globals *server, network_message_record *record, int32_t length)
{
    return halo::networking::ServerMessageHandlers(server).handshake_forward(record, length);
}

/**
 * C entry point for halo::networking::ServerMessageHandlers::join_finalize_ack_role2; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e2930
 */
uint32_t network_game_message_handle_join_finalize_ack_role2(network_server_globals *server, network_machine *machine, network_message_record *record, int32_t length)
{
    return halo::networking::ServerMessageHandlers(server).join_finalize_ack_role2(machine, record, length);
}

/**
 * C entry point for halo::networking::ServerMessageHandlers::keepalive; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e2110
 */
uint32_t network_game_message_handle_keepalive(network_channel **channel, int32_t *record)
{
    return halo::networking::ServerMessageHandlers::keepalive(channel, record);
}

/**
 * C entry point for halo::networking::ServerMessageHandlers::ping_timestamp; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e20b0
 */
uint32_t network_game_message_handle_ping_timestamp(int32_t **message, network_server_globals *server)
{
    return halo::networking::ServerMessageHandlers(server).ping_timestamp(message);
}

/**
 * C entry point for halo::networking::ServerMessageHandlers::player_count_broadcast; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e2530
 */
uint32_t network_game_message_handle_player_count_broadcast(network_server_globals *server, network_message_record *record, int32_t length)
{
    return halo::networking::ServerMessageHandlers(server).player_count_broadcast(record, length);
}

/**
 * C entry point for halo::networking::ServerMessageHandlers::player_entry_update; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e2580
 */
uint32_t network_game_message_handle_player_entry_update(network_server_globals *server, network_message_record *record, int32_t length)
{
    return halo::networking::ServerMessageHandlers(server).player_entry_update(record, length);
}

/**
 * C entry point for halo::networking::ServerMessageHandlers::retry_schedule; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e26a0
 */
uint32_t network_game_message_handle_retry_schedule(network_server_globals *server, network_machine *machine, network_message_record *record, int32_t length)
{
    return halo::networking::ServerMessageHandlers(server).retry_schedule(machine, record, length);
}

/**
 * C entry point for halo::networking::ServerMessageHandlers::settings_relay; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e24d0
 */
uint32_t network_game_message_handle_settings_relay(network_server_globals *server, network_message_record *record, int32_t length)
{
    return halo::networking::ServerMessageHandlers(server).settings_relay(record, length);
}

/**
 * C entry point for halo::networking::ServerMessageHandlers::settings_relay_role2; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e28d0
 */
uint32_t network_game_message_handle_settings_relay_role2(network_server_globals *server, network_message_record *record, int32_t length)
{
    return halo::networking::ServerMessageHandlers(server).settings_relay_role2(record, length);
}

/**
 * C entry point for halo::networking::HostServerView::round_reset; forwards to the C++ implementation unchanged.
 *
 * @address 0x4df640
 */
void network_host_round_reset(network_server_globals *host)
{
    halo::networking::HostServerView(host).round_reset();
}

/**
 * C entry point for halo::networking::HostServerView::send_scenario_announcement; forwards to the C++ implementation unchanged.
 *
 * @address 0x4df1c0
 */
int32_t network_host_send_scenario_announcement(network_server_globals *host)
{
    return halo::networking::HostServerView(host).send_scenario_announcement();
}

/**
 * C entry point for halo::networking::HostServerView::shutdown_or_defer; forwards to the C++ implementation unchanged.
 *
 * @address 0x4ddd90
 */
int32_t network_host_shutdown_or_defer(void)
{
    return halo::networking::HostServerView::shutdown_or_defer();
}

/**
 * C entry point for halo::networking::HostServerView::update_tick; forwards to the C++ implementation unchanged.
 *
 * @address 0x4def80
 */
char network_host_update_tick(network_server_globals *host)
{
    return halo::networking::HostServerView(host).update_tick();
}

/**
 * C entry point for halo::networking::MachineView::reset_state; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e0ab0
 */
uint8_t network_join_request_reset_state(network_machine *machine, const char *response)
{
    return halo::networking::MachineView(machine).reset_state(response);
}

/**
 * C entry point for halo::networking::MachineView::check_build_version; forwards to the C++ implementation unchanged.
 *
 * @address 0x4dff20
 */
void network_machine_check_build_version(const char *remote_version, network_machine *machine)
{
    halo::networking::MachineView(machine).check_build_version(remote_version);
}

/**
 * C entry point for halo::networking::MachineView::reset; forwards to the C++ implementation unchanged.
 *
 * @address 0x4df690
 */
int32_t network_machine_reset(network_machine *machine)
{
    return halo::networking::MachineView(machine).reset();
}

/**
 * C entry point for halo::networking::MachineView::timer_start; forwards to the C++ implementation unchanged.
 *
 * @address 0x4df090
 */
void network_machine_timer_start(network_machine *machine, int32_t duration_ms)
{
    halo::networking::MachineView(machine).timer_start(duration_ms);
}

/**
 * C entry point for halo::networking::ServerView::handle_join_confirm; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e2400
 */
char network_game_server_handle_join_confirm(network_machine *machine, network_server_globals *server, network_message_record *buffer, int32_t length)
{
    return halo::networking::ServerView(server).handle_join_confirm(machine, buffer, length);
}

/**
 * C entry point for halo::networking::ServerView::handle_join_password; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e21d0
 */
char network_game_server_handle_join_password(network_machine *machine, network_server_globals *server, network_message_record *buffer, int32_t length)
{
    return halo::networking::ServerView(server).handle_join_password(machine, buffer, length);
}

/**
 * C entry point for halo::networking::ServerView::load_scenario; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e0720
 */
char network_game_server_load_scenario(void)
{
    return halo::networking::ServerView::load_scenario();
}

/**
 * C entry point for halo::networking::ServerView::record_last_sender; forwards to the C++ implementation unchanged.
 *
 * @address 0x4df900
 */
uint32_t network_object_record_last_sender(int32_t player_index, int32_t quit_tick, network_server_globals *server)
{
    return halo::networking::ServerView(server).record_last_sender(player_index, quit_tick);
}

/**
 * C entry point for halo::networking::ServerView::build_full_game_info_packet; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e0bd0
 */
char network_server_build_full_game_info_packet(network_machine *machine)
{
    return halo::networking::ServerView::build_full_game_info_packet(machine);
}

/**
 * C entry point for halo::networking::ServerView::build_game_info_packet; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e0950
 */
char network_server_build_game_info_packet(network_server_globals *server, network_machine *machine)
{
    return halo::networking::ServerView(server).build_game_info_packet(machine);
}

/**
 * C entry point for halo::networking::ServerView::check_machine_timeout; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e0ef0
 */
int32_t network_server_check_machine_timeout(network_server_globals *server, network_machine *machine)
{
    return halo::networking::ServerView(server).check_machine_timeout(machine);
}

/**
 * C entry point for halo::networking::ServerView::service_machines_tick; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e11d0
 */
char network_server_service_machines_tick(network_server_globals *server)
{
    return halo::networking::ServerView(server).service_machines_tick();
}

/**
 * C entry point for halo::networking::ServerView::broadcast_to_flagged; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e1a80
 */
char network_session_broadcast_to_flagged(int32_t body_bit_count, network_server_globals *server, int32_t status_bit, void *data, int32_t immediate, int32_t flush_after, char force, int32_t unused)
{
    return halo::networking::ServerView(server).broadcast_to_flagged(body_bit_count, status_bit, data, immediate, flush_after, force, unused);
}

/**
 * C entry point for halo::networking::ServerView::send_to_machine; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e1930
 */
uint8_t network_session_send_to_machine(int32_t machine_id, network_server_globals *server, uint32_t status_bit, void *data, uint32_t body_bit_count, uint32_t reliable, uint32_t unknown_a, char force, uint32_t priority)
{
    return halo::networking::ServerView(server).send_to_machine(machine_id, status_bit, data, body_bit_count, reliable, unknown_a, force, priority);
}

/**
 * C entry point for halo::networking::ServerMessageHandlers::client_game_settings_updated; forwards to the C++ implementation unchanged.
 *
 * @address 0x4df2e0
 */
uint32_t network_game_client_game_settings_updated(network_server_globals *host)
{
    return halo::networking::ServerMessageHandlers(host).client_game_settings_updated();
}

/**
 * C entry point for halo::networking::ServerView::heartbeat_tick; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e15a0
 */
uint8_t network_server_heartbeat_tick(network_server_globals *server)
{
    return halo::networking::ServerView(server).heartbeat_tick();
}

/**
 * C entry point for halo::networking::ServerView::resend_challenge_periodic; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e1450
 */
uint32_t network_server_resend_challenge_periodic(network_server_globals *server)
{
    return halo::networking::ServerView(server).resend_challenge_periodic();
}

/**
 * C entry point for halo::networking::HostServerView::full_state_broadcast; forwards to the C++ implementation unchanged.
 *
 * @address 0x4df510
 */
void network_host_full_state_broadcast(network_server_globals *host)
{
    halo::networking::HostServerView(host).full_state_broadcast();
}

/**
 * C entry point for halo::networking::ClientView::begin_connect; forwards to the C++ implementation unchanged.
 *
 * @address 0x4dc8d0
 */
uint32_t network_client_begin_connect(wchar_t *player_name, s_network_address *target_address)
{
    return halo::networking::ClientView::begin_connect(player_name, target_address);
}

/**
 * C entry point for halo::networking::ClientView::check_connection_quality; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e0080
 */
uint32_t network_client_check_connection_quality(int16_t machine_id, client_update_record update)
{
    return halo::networking::ClientView::check_connection_quality(machine_id, update);
}

/**
 * C entry point for halo::networking::ClientView::connect_progress_percent; forwards to the C++ implementation unchanged.
 *
 * @address 0x4d8c10
 */
int16_t network_client_connect_progress_percent(network_client_globals *client, int16_t *out_percent)
{
    return halo::networking::ClientView(client).connect_progress_percent(out_percent);
}

/**
 * C entry point for halo::networking::ClientView::drain_queued_updates; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e1f40
 */
char network_client_drain_queued_updates(network_server_globals *server, network_machine *machine, bit_stream *stream)
{
    return halo::networking::ClientView::drain_queued_updates(server, machine, stream);
}

/**
 * C entry point for halo::networking::ClientView::globals_create; forwards to the C++ implementation unchanged.
 *
 * @address 0x4dde50
 */
int32_t network_client_globals_create(void)
{
    return halo::networking::ClientView::globals_create();
}

/**
 * C entry point for halo::networking::ClientView::globals_dispose; forwards to the C++ implementation unchanged.
 *
 * @address 0x4dde70
 */
void network_client_globals_dispose(void)
{
    halo::networking::ClientView::globals_dispose();
}

/**
 * C entry point for halo::networking::ClientView::handle_server_text_message; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e5140
 */
void network_client_handle_server_text_message(void *message)
{
    halo::networking::ClientView::handle_server_text_message(message);
}

/**
 * C entry point for halo::networking::ClientView::identity_tick; forwards to the C++ implementation unchanged.
 *
 * @address 0x4db310
 */
int32_t network_client_identity_tick(network_client_globals *client)
{
    return halo::networking::ClientView(client).identity_tick();
}

/**
 * C entry point for halo::networking::ClientView::send_local_player_updates; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e77e0
 */
void network_client_send_local_player_updates(void)
{
    halo::networking::ClientView::send_local_player_updates();
}

/**
 * C entry point for halo::networking::ClientView::state_dispatch; forwards to the C++ implementation unchanged.
 *
 * @address 0x4d8bb0
 */
int8_t network_client_state_dispatch(network_client_globals *client)
{
    return halo::networking::ClientView(client).state_dispatch();
}

/**
 * C entry point for halo::networking::ClientView::timer_schedule; forwards to the C++ implementation unchanged.
 *
 * @address 0x4d9ed0
 */
void network_client_timer_schedule(int32_t delay_ms, int32_t context, network_client_globals *client)
{
    halo::networking::ClientView(client).timer_schedule(delay_ms, context);
}

/**
 * C entry point for halo::networking::ClientView::update_dispatch; forwards to the C++ implementation unchanged.
 *
 * @address 0x4dded0
 */
char network_client_update_dispatch(void)
{
    return halo::networking::ClientView::update_dispatch();
}

/**
 * C entry point for halo::networking::ClientView::client_update; forwards to the C++ implementation unchanged.
 *
 * @address 0x4daf80
 */
int8_t network_game_client_update(network_client_globals *client)
{
    return halo::networking::ClientView(client).client_update();
}

/**
 * C entry point for halo::networking::ClientView::record_message_send; forwards to the C++ implementation unchanged.
 *
 * @address 0x4da130
 */
int32_t network_game_record_message_send(network_client_globals *client, const uint32_t *source)
{
    return halo::networking::ClientView(client).record_message_send(source);
}

/**
 * C entry point for halo::networking::ClientView::join_finalize; forwards to the C++ implementation unchanged.
 *
 * @address 0x4d9e30
 */
char network_player_join_finalize(network_client_globals *client, network_player_entry *entry)
{
    return halo::networking::ClientView(client).join_finalize(entry);
}

/**
 * C entry point for halo::networking::ClientView::create; forwards to the C++ implementation unchanged.
 *
 * @address 0x4d8a80
 */
network_client_globals * network_session_create(void)
{
    return halo::networking::ClientView::create();
}

/**
 * C entry point for halo::networking::ClientView::info_packet_send; forwards to the C++ implementation unchanged.
 *
 * @address 0x4d9050
 */
char network_session_info_packet_send(const uint32_t *source, network_client_globals *client)
{
    return halo::networking::ClientView(client).info_packet_send(source);
}

/**
 * C entry point for halo::networking::ClientView::player_join_notify; forwards to the C++ implementation unchanged.
 *
 * @address 0x4d9700
 */
void network_session_player_join_notify(network_client_globals *client, const uint32_t *source)
{
    halo::networking::ClientView(client).player_join_notify(source);
}

/**
 * C entry point for halo::networking::ClientView::player_table_index_apply; forwards to the C++ implementation unchanged.
 *
 * @address 0x4d9190
 */
uint8_t network_session_player_table_index_apply(network_client_globals *client, int32_t table_index, const uint8_t *candidate)
{
    return halo::networking::ClientView(client).player_table_index_apply(table_index, candidate);
}

/**
 * C entry point for halo::networking::ClientView::staged_message_commit; forwards to the C++ implementation unchanged.
 *
 * @address 0x4da250
 */
int32_t network_staged_message_commit(network_client_globals *client, uint16_t message_value)
{
    return halo::networking::ClientView(client).staged_message_commit(message_value);
}

/**
 * C entry point for halo::networking::ConnectionView::endpoint_set; forwards to the C++ implementation unchanged.
 *
 * @address 0x4d8c50
 */
int32_t network_connection_endpoint_set(const uint32_t *source, network_client_globals *connection)
{
    return halo::networking::ConnectionView(connection).endpoint_set(source);
}

/**
 * C entry point for halo::networking::ConnectionView::initiate; forwards to the C++ implementation unchanged.
 *
 * @address 0x4d8cf0
 */
uint8_t network_connection_initiate(network_client_globals *connection, const uint32_t *target, const uint32_t *session_info, const uint32_t *connect_address)
{
    return halo::networking::ConnectionView(connection).initiate(target, session_info, connect_address);
}

/**
 * C entry point for halo::networking::ConnectionView::retransmit_if_overdue; forwards to the C++ implementation unchanged.
 *
 * @address 0x4d93b0
 */
void network_connection_retransmit_if_overdue(const uint32_t *sender_address, network_client_globals *client, uint32_t deadline_ms, int32_t remote_time)
{
    halo::networking::ConnectionView(client).retransmit_if_overdue(sender_address, deadline_ms, remote_time);
}

/**
 * C entry point for halo::networking::ConnectionView::send_keepalive; forwards to the C++ implementation unchanged.
 *
 * @address 0x4d9400
 */
void network_connection_send_keepalive(network_client_globals *client)
{
    halo::networking::ConnectionView(client).send_keepalive();
}

/**
 * C entry point for halo::networking::HostClientView::channel_service_tick; forwards to the C++ implementation unchanged.
 *
 * @address 0x4db100
 */
char network_host_channel_service_tick(network_client_globals *client)
{
    return halo::networking::HostClientView(client).channel_service_tick();
}

/**
 * C entry point for halo::networking::HostClientView::lobby_tick; forwards to the C++ implementation unchanged.
 *
 * @address 0x4daef0
 */
char network_host_lobby_tick(network_client_globals *client)
{
    return halo::networking::HostClientView(client).lobby_tick();
}

/**
 * C entry point for halo::networking::HostClientView::presence_broadcast_tick; forwards to the C++ implementation unchanged.
 *
 * @address 0x4dadb0
 */
void network_host_presence_broadcast_tick(network_client_globals *client)
{
    halo::networking::HostClientView(client).presence_broadcast_tick();
}

/**
 * C entry point for halo::networking::JoinView::connect_retry_tick; forwards to the C++ implementation unchanged.
 *
 * @address 0x4dab80
 */
int32_t network_join_connect_retry_tick(network_client_globals *client)
{
    return halo::networking::JoinView(client).connect_retry_tick();
}

/**
 * C entry point for halo::networking::JoinView::handshake_tick; forwards to the C++ implementation unchanged.
 *
 * @address 0x4daa20
 */
uint32_t network_join_handshake_tick(network_client_globals *client)
{
    return halo::networking::JoinView(client).handshake_tick();
}

/**
 * C entry point for halo::networking::JoinView::request_resolve_host; forwards to the C++ implementation unchanged.
 *
 * @address 0x4ba320
 */
uint32_t network_join_request_resolve_host(void)
{
    return halo::networking::JoinView::request_resolve_host();
}

/**
 * C entry point for halo::networking::ClientView::connection_handshake_tick; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e0590
 */
void network_client_connection_handshake_tick(int16_t state, network_server_globals *owner)
{
    halo::networking::ClientView::connection_handshake_tick(state, owner);
}

/**
 * C entry point for halo::networking::ClientView::rejoin_check; forwards to the C++ implementation unchanged.
 *
 * @address 0x4de390
 */
void network_client_rejoin_check(int8_t machine_player_index)
{
    halo::networking::ClientView::rejoin_check(machine_player_index);
}

/**
 * C entry point for halo::networking::ClientView::timer_default_or_disconnect; forwards to the C++ implementation unchanged.
 *
 * @address 0x4d9ce0
 */
void network_client_timer_default_or_disconnect(network_client_globals *client)
{
    halo::networking::ClientView(client).timer_default_or_disconnect();
}

/**
 * C entry point for halo::networking::ClientView::disconnect_notify_dropped_machines; forwards to the C++ implementation unchanged.
 *
 * @address 0x4d9340
 */
void network_disconnect_notify_dropped_machines(network_client_globals *client)
{
    halo::networking::ClientView(client).disconnect_notify_dropped_machines();
}

/**
 * C entry point for halo::networking::ClientView::client_connect_to_address; forwards to the C++ implementation unchanged.
 *
 * @address 0x4dc790
 */
uint32_t network_game_client_connect_to_address(wchar_t *player_name, char *address_string)
{
    return halo::networking::ClientView::client_connect_to_address(player_name, address_string);
}

/**
 * C entry point for halo::networking::ClientView::destroy; forwards to the C++ implementation unchanged.
 *
 * @address 0x4d8b70
 */
void network_session_destroy(network_client_globals *client)
{
    halo::networking::ClientView(client).destroy();
}

/**
 * C entry point for halo::networking::ConnectionView::send_join_request_packet; forwards to the C++ implementation unchanged.
 *
 * @address 0x4d9220
 */
int32_t network_send_join_request_packet(network_client_globals *connection)
{
    return halo::networking::ConnectionView(connection).send_join_request_packet();
}

/**
 * C entry point for halo::networking::JoinView::hostname_resolved_callback; forwards to the C++ implementation unchanged.
 *
 * @address 0x4ba270
 */
void network_join_hostname_resolved_callback(int32_t resolve_failed, uint32_t unused, uint8_t *hostent)
{
    halo::networking::JoinView::hostname_resolved_callback(resolve_failed, unused, hostent);
}

/**
 * C entry point for halo::networking::JoinView::status_text_update; forwards to the C++ implementation unchanged.
 *
 * @address 0x4db4c0
 */
void network_join_status_text_update(int32_t mode, network_client_globals *client)
{
    halo::networking::JoinView(client).status_text_update(mode);
}

/**
 * C entry point for halo::networking::ConnectionView::finalize_join; forwards to the C++ implementation unchanged.
 *
 * @address 0x4d9960
 */
int32_t network_connection_finalize_join(uint16_t *connection)
{
    return halo::networking::ConnectionView::finalize_join(connection);
}

/**
 * C entry point for halo::networking::ConnectionStats::end; forwards to the C++ implementation unchanged.
 *
 * @address 0x440d20
 */
void network_connection_stats_end(int32_t connection_id, uint16_t connection_key)
{
    halo::networking::ConnectionStats::end(connection_id, connection_key);
}

/**
 * C entry point for halo::networking::ConnectionStats::log_tick; forwards to the C++ implementation unchanged.
 *
 * @address 0x440d80
 */
void network_connection_stats_log_tick(void)
{
    halo::networking::ConnectionStats::log_tick();
}

/**
 * C entry point for halo::networking::ConnectionStats::lookup_or_add; forwards to the C++ implementation unchanged.
 *
 * @address 0x440a80
 */
int32_t network_connection_stats_lookup_or_add(int32_t connection_id, uint16_t connection_key)
{
    return halo::networking::ConnectionStats::lookup_or_add(connection_id, connection_key);
}

/**
 * C entry point for halo::networking::ConnectionStats::record_packet; forwards to the C++ implementation unchanged.
 *
 * @address 0x440b20
 */
void network_connection_stats_record_packet(void *gamespy_connection, int32_t payload_length, uint8_t is_sent, uint8_t is_reliable, uint8_t is_resend)
{
    halo::networking::ConnectionStats::record_packet(gamespy_connection, payload_length, is_sent, is_reliable, is_resend);
}

/**
 * C entry point for halo::networking::NetworkRuntime::debug_fill_canary_buffer; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e0790
 */
void network_debug_fill_canary_buffer(uint32_t *buffer)
{
    halo::networking::NetworkRuntime::debug_fill_canary_buffer(buffer);
}

/**
 * C entry point for halo::networking::NetworkRuntime::dispatch_initialize; forwards to the C++ implementation unchanged.
 *
 * @address 0x4414c0
 */
void network_dispatch_initialize(void)
{
    halo::networking::NetworkRuntime::dispatch_initialize();
}

/**
 * C entry point for halo::networking::NetworkRuntime::hostname_thread_proc; forwards to the C++ implementation unchanged.
 *
 * @address 0x441510
 */
void network_hostname_thread_proc(char *hostname_buffer)
{
    halo::networking::NetworkRuntime::hostname_thread_proc(hostname_buffer);
}

/**
 * C entry point for halo::networking::NetworkRuntime::initialize; forwards to the C++ implementation unchanged.
 *
 * @address 0x4415c0
 */
int16_t network_initialize(void)
{
    return halo::networking::NetworkRuntime::initialize();
}

/**
 * C entry point for halo::networking::NetworkRuntime::local_hostent_get; forwards to the C++ implementation unchanged.
 *
 * @address 0x441540
 */
int network_local_hostent_get(void **out_hostent)
{
    return halo::networking::NetworkRuntime::local_hostent_get(out_hostent);
}

/**
 * C entry point for halo::networking::NetworkRuntime::log_path_resolve; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e40a0
 */
char * network_log_path_resolve(char *requested_path)
{
    return halo::networking::NetworkRuntime::log_path_resolve(requested_path);
}

/**
 * C entry point for halo::networking::NetworkRuntime::name_string_is_valid_for_mode; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e4350
 */
uint8_t network_name_string_is_valid_for_mode(char *name, void *character, int32_t mode)
{
    return halo::networking::NetworkRuntime::name_string_is_valid_for_mode(name, character, mode);
}

/**
 * C entry point for halo::networking::NetworkRuntime::password_field_set; forwards to the C++ implementation unchanged.
 *
 * @address 0x4df070
 */
void network_password_field_set(network_server_globals *object, wchar_t *source)
{
    halo::networking::NetworkRuntime::password_field_set(object, source);
}

/**
 * C entry point for halo::networking::NetworkRuntime::prepare_challenge_packet; forwards to the C++ implementation unchanged.
 *
 * @address 0x4deaf0
 */
uint16_t * network_prepare_challenge_packet(int32_t message_type, void *payload)
{
    return halo::networking::NetworkRuntime::prepare_challenge_packet(message_type, payload);
}

/**
 * C entry point for halo::networking::NetworkRuntime::random_offset; forwards to the C++ implementation unchanged.
 *
 * @address 0x4403b0
 */
int32_t network_random_offset(int32_t base)
{
    return halo::networking::NetworkRuntime::random_offset(base);
}

/**
 * C entry point for halo::networking::NetworkRuntime::shutdown; forwards to the C++ implementation unchanged.
 *
 * @address 0x4416e0
 */
int32_t network_shutdown(void)
{
    return halo::networking::NetworkRuntime::shutdown();
}

/**
 * C entry point for halo::networking::NetworkRuntime::signal_quality_glyph; forwards to the C++ implementation unchanged.
 *
 * @address 0x440610
 */
uint8_t network_signal_quality_glyph(uint32_t code)
{
    return halo::networking::NetworkRuntime::signal_quality_glyph(code);
}

/**
 * C entry point for halo::networking::NetworkRuntime::update_; forwards to the C++ implementation unchanged.
 *
 * @address 0x4418d0
 */
uint32_t network_update(void)
{
    return halo::networking::NetworkRuntime::update_();
}

/**
 * C entry point for halo::networking::EventFeed::flush; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e8040
 */
void network_event_feed_flush(int32_t *queue)
{
    halo::networking::EventFeed::flush(queue);
}

/**
 * C entry point for halo::networking::EventFeed::queue_append; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e7ff0
 */
void network_event_feed_queue_append(uint8_t *queue, uint32_t *key, uint32_t *payload)
{
    halo::networking::EventFeed::queue_append(queue, key, payload);
}

/**
 * C entry point for halo::networking::IndexCache::find_or_allocate_slot; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e9c20
 */
int32_t network_index_cache_find_or_allocate_slot(uint8_t *container, int32_t key)
{
    return halo::networking::IndexCache::find_or_allocate_slot(container, key);
}

/**
 * C entry point for halo::networking::IndexCache::get; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e9d20
 */
int32_t network_index_cache_get(hash_table *table, int32_t key)
{
    return halo::networking::IndexCache::get(table, key);
}

/**
 * C entry point for halo::networking::IndexCache::insert_if_free; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e9cd0
 */
uint8_t network_index_cache_insert_if_free(uint8_t *container, int32_t slot, int32_t key)
{
    return halo::networking::IndexCache::insert_if_free(container, slot, key);
}

/**
 * C entry point for halo::networking::IndexCache::remove; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e9d40
 */
uint8_t network_index_cache_remove(uint8_t *container, int32_t key)
{
    return halo::networking::IndexCache::remove(container, key);
}

/**
 * C entry point for halo::networking::MessageBlocks::block_build; forwards to the C++ implementation unchanged.
 *
 * @address 0x440350
 */
uint16_t * network_message_block_build(uint16_t *buffer, uint32_t *source, uint8_t flags, uint32_t length)
{
    return halo::networking::MessageBlocks::block_build(buffer, source, flags, length);
}

/**
 * C entry point for halo::networking::MessageBlocks::read_sized_buffer; forwards to the C++ implementation unchanged.
 *
 * @address 0x4de420
 */
uint16_t * network_message_read_sized_buffer(uint16_t *buffer, int32_t capacity, bit_stream *stream)
{
    return halo::networking::MessageBlocks::read_sized_buffer(buffer, capacity, stream);
}

/**
 * C entry point for halo::networking::StatsSummaryLog::open; forwards to the C++ implementation unchanged.
 *
 * @address 0x440670
 */
void network_stats_summary_log_open(void)
{
    halo::networking::StatsSummaryLog::open();
}

/**
 * C entry point for halo::networking::StatsSummaryLog::write; forwards to the C++ implementation unchanged.
 *
 * @address 0x440820
 */
void network_stats_summary_log_write(void)
{
    halo::networking::StatsSummaryLog::write();
}

/**
 * C entry point for halo::networking::GameClientView::action_apply; forwards to the C++ implementation unchanged.
 *
 * @address 0x4da320
 */
void network_game_action_apply(void **context, network_client_globals *client)
{
    halo::networking::GameClientView(client).action_apply(context);
}

/**
 * C entry point for halo::networking::GameClientView::action_queue_drain; forwards to the C++ implementation unchanged.
 *
 * @address 0x4db870
 */
char network_game_action_queue_drain(network_client_globals *client, bit_stream *stream, const uint32_t *sender)
{
    return halo::networking::GameClientView(client).action_queue_drain(stream, sender);
}

/**
 * C entry point for halo::networking::GameClientView::process_incoming_messages; forwards to the C++ implementation unchanged.
 *
 * @address 0x4db180
 */
int32_t network_game_process_incoming_messages(network_client_globals *client)
{
    return halo::networking::GameClientView(client).process_incoming_messages();
}

/**
 * C entry point for halo::networking::GameClientView::settings_packet_receive; forwards to the C++ implementation unchanged.
 *
 * @address 0x4d9800
 */
int32_t network_game_settings_packet_receive(network_client_globals *client, const uint32_t *request)
{
    return halo::networking::GameClientView(client).settings_packet_receive(request);
}

/**
 * C entry point for halo::networking::GameClientView::settings_packet_send; forwards to the C++ implementation unchanged.
 *
 * @address 0x4d94c0
 */
void network_game_settings_packet_send(network_client_globals *client, const uint8_t *request)
{
    halo::networking::GameClientView(client).settings_packet_send(request);
}

/**
 * C entry point for halo::networking::GameClientView::state_update_receive; forwards to the C++ implementation unchanged.
 *
 * @address 0x4d9d20
 */
int32_t network_game_state_update_receive(network_client_globals *client, uint8_t *record)
{
    return halo::networking::GameClientView(client).state_update_receive(record);
}

/**
 * C entry point for halo::networking::GameClientView::incoming_item_dispatch; forwards to the C++ implementation unchanged.
 *
 * @address 0x4db630
 */
char network_incoming_item_dispatch(network_client_globals *client, uint32_t item_flag, bit_stream *stream, const uint32_t *sender)
{
    return halo::networking::GameClientView(client).incoming_item_dispatch(item_flag, stream, sender);
}

/**
 * C entry point for halo::networking::ClientMessageDecoder::and_discard_ingame_message; forwards to the C++ implementation unchanged.
 *
 * @address 0x4dc020
 */
int32_t network_game_client_decode_and_discard_ingame_message(network_client_globals *client, const uint8_t *buffer, int32_t length, const uint32_t *sender_address)
{
    return halo::networking::ClientMessageDecoder(client).and_discard_ingame_message(buffer, length, sender_address);
}

/**
 * C entry point for halo::networking::ClientMessageDecoder::and_discard_join_message; forwards to the C++ implementation unchanged.
 *
 * @address 0x4dbfb0
 */
int32_t network_game_client_decode_and_discard_join_message(network_client_globals *client, const uint8_t *buffer, int32_t length, const uint32_t *sender_address)
{
    return halo::networking::ClientMessageDecoder(client).and_discard_join_message(buffer, length, sender_address);
}

/**
 * C entry point for halo::networking::ClientMessageDecoder::beacon_reply; forwards to the C++ implementation unchanged.
 *
 * @address 0x4db9a0
 */
int32_t network_game_client_decode_beacon_reply(network_client_globals *client, const uint8_t *buffer, int32_t length)
{
    return halo::networking::ClientMessageDecoder(client).beacon_reply(buffer, length);
}

/**
 * C entry point for halo::networking::ClientMessageDecoder::join_accepted; forwards to the C++ implementation unchanged.
 *
 * @address 0x4dbcc0
 */
int32_t network_game_client_decode_join_accepted(network_client_globals *client, const uint8_t *buffer, int32_t length, const uint32_t *sender_address)
{
    return halo::networking::ClientMessageDecoder(client).join_accepted(buffer, length, sender_address);
}

/**
 * C entry point for halo::networking::ClientMessageDecoder::join_complete; forwards to the C++ implementation unchanged.
 *
 * @address 0x4dbdc0
 */
int32_t network_game_client_decode_join_complete(network_client_globals *client, const uint8_t *buffer, int32_t length, const uint32_t *sender_address)
{
    return halo::networking::ClientMessageDecoder(client).join_complete(buffer, length, sender_address);
}

/**
 * C entry point for halo::networking::ClientMessageDecoder::join_finalize_ack; forwards to the C++ implementation unchanged.
 *
 * @address 0x4dc120
 */
int32_t network_game_client_decode_join_finalize_ack(network_client_globals *client, const uint8_t *buffer, int32_t length, const uint32_t *sender_address)
{
    return halo::networking::ClientMessageDecoder(client).join_finalize_ack(buffer, length, sender_address);
}

/**
 * C entry point for halo::networking::ClientMessageDecoder::join_finalize_message; forwards to the C++ implementation unchanged.
 *
 * @address 0x4dc090
 */
int32_t network_game_client_decode_join_finalize_message(network_client_globals *client, const uint8_t *buffer, int32_t length, const uint32_t *sender_address)
{
    return halo::networking::ClientMessageDecoder(client).join_finalize_message(buffer, length, sender_address);
}

/**
 * C entry point for halo::networking::ClientMessageDecoder::player_config_value; forwards to the C++ implementation unchanged.
 *
 * @address 0x4dbf30
 */
int32_t network_game_client_decode_player_config_value(network_client_globals *client, const uint8_t *buffer, int32_t length, const uint32_t *sender_address)
{
    return halo::networking::ClientMessageDecoder(client).player_config_value(buffer, length, sender_address);
}

/**
 * C entry point for halo::networking::ClientMessageDecoder::player_join_chunk; forwards to the C++ implementation unchanged.
 *
 * @address 0x4dc240
 */
char network_game_client_decode_player_join_chunk(network_client_globals *client, uint8_t *param_1, int32_t param_2, int32_t *param_3)
{
    return halo::networking::ClientMessageDecoder(client).player_join_chunk(param_1, param_2, param_3);
}

/**
 * C entry point for halo::networking::ClientMessageDecoder::player_slot_chunk; forwards to the C++ implementation unchanged.
 *
 * @address 0x4dc2e0
 */
char network_game_client_decode_player_slot_chunk(network_client_globals *client, uint8_t *param_1, int32_t param_2, int32_t *param_3)
{
    return halo::networking::ClientMessageDecoder(client).player_slot_chunk(param_1, param_2, param_3);
}

/**
 * C entry point for halo::networking::ClientMessageDecoder::pong_reply; forwards to the C++ implementation unchanged.
 *
 * @address 0x4dba20
 */
int32_t network_game_client_decode_pong_reply(network_client_globals *client, const uint8_t *buffer, int32_t length, const uint32_t *sender_address)
{
    return halo::networking::ClientMessageDecoder(client).pong_reply(buffer, length, sender_address);
}

/**
 * C entry point for halo::networking::ClientMessageDecoder::state_update_chunk; forwards to the C++ implementation unchanged.
 *
 * @address 0x4dc190
 */
char network_game_client_decode_state_update_chunk(network_client_globals *client, uint8_t *param_1, int32_t param_2, int32_t *param_3)
{
    return halo::networking::ClientMessageDecoder(client).state_update_chunk(param_1, param_2, param_3);
}

/**
 * C entry point for halo::networking::ClientMessageDecoder::sync_complete; forwards to the C++ implementation unchanged.
 *
 * @address 0x4dc3a0
 */
int32_t network_game_client_decode_sync_complete(network_client_globals *client, const uint8_t *buffer, int32_t length, const uint32_t *sender_address)
{
    return halo::networking::ClientMessageDecoder(client).sync_complete(buffer, length, sender_address);
}

/**
 * C entry point for halo::networking::ClientMessageDecoder::dispatch; forwards to the C++ implementation unchanged.
 *
 * @address 0x4db6b0
 */
char network_game_message_decode_dispatch(network_client_globals *client, uint16_t *record, int32_t record_length, const uint32_t *sender)
{
    return halo::networking::ClientMessageDecoder(client).dispatch(record, record_length, sender);
}

/**
 * C entry point for halo::networking::ClientMessageDecoder::ingame_notification; forwards to the C++ implementation unchanged.
 *
 * @address 0x4dc4b0
 */
int32_t network_game_message_decode_ingame_notification(network_client_globals *client, const uint8_t *buffer, int32_t length, const uint32_t *sender_address)
{
    return halo::networking::ClientMessageDecoder(client).ingame_notification(buffer, length, sender_address);
}

/**
 * C entry point for halo::networking::ClientMessageDecoder::replicated_command; forwards to the C++ implementation unchanged.
 *
 * @address 0x4dc410
 */
int32_t network_game_message_decode_replicated_command(network_client_globals *client, uint8_t *param_1, int32_t param_2, int32_t *param_3)
{
    return halo::networking::ClientMessageDecoder(client).replicated_command(param_1, param_2, param_3);
}

/**
 * C entry point for halo::networking::ClientMessageDecoder::connect_rejected; forwards to the C++ implementation unchanged.
 *
 * @address 0x4dbd40
 */
int32_t network_game_client_decode_connect_rejected(network_client_globals *client, const uint8_t *buffer, int32_t length, const uint32_t *expected_sequence)
{
    return halo::networking::ClientMessageDecoder(client).connect_rejected(buffer, length, expected_sequence);
}

/**
 * C entry point for halo::networking::ClientMessageDecoder::settings_or_ack; forwards to the C++ implementation unchanged.
 *
 * @address 0x4dbe50
 */
int32_t network_game_client_decode_settings_or_ack(network_client_globals *client, const uint8_t *buffer, int32_t length, const int32_t *expected_sequence)
{
    return halo::networking::ClientMessageDecoder(client).settings_or_ack(buffer, length, expected_sequence);
}

/**
 * C entry point for halo::networking::ClientMessageDecoder::decode_settings_request; forwards to the C++ implementation unchanged.
 *
 * @address 0x4dbc00
 */
int32_t network_game_decode_settings_request(network_client_globals *client, const uint8_t *buffer, int32_t length, const int32_t *expected_sequence)
{
    return halo::networking::ClientMessageDecoder(client).decode_settings_request(buffer, length, expected_sequence);
}

/**
 * C entry point for halo::networking::GameRuntime::broadcast_team_object_updates; forwards to the C++ implementation unchanged.
 *
 * @address 0x4df950
 */
void network_game_broadcast_team_object_updates(int32_t *object_count, uint32_t param_1, int32_t *bytes_sent)
{
    halo::networking::GameRuntime::broadcast_team_object_updates(object_count, param_1, bytes_sent);
}

/**
 * C entry point for halo::networking::GameRuntime::client_apply_position_update; forwards to the C++ implementation unchanged.
 *
 * @address 0x4dff70
 */
void network_game_client_apply_position_update(network_machine *machine, const client_position_packet *packet, int32_t tick_count, uint32_t history_byte)
{
    halo::networking::GameRuntime::client_apply_position_update(machine, packet, tick_count, history_byte);
}

/**
 * C entry point for halo::networking::GameRuntime::client_apply_received_update; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e0280
 */
void network_game_client_apply_received_update(network_machine *machine, uint32_t server, void **message)
{
    halo::networking::GameRuntime::client_apply_received_update(machine, server, message);
}

/**
 * C entry point for halo::networking::GameRuntime::get_random_player_name; forwards to the C++ implementation unchanged.
 *
 * @address 0x4dea80
 */
wchar_t * network_game_get_random_player_name(void)
{
    return halo::networking::GameRuntime::get_random_player_name();
}

/**
 * C entry point for halo::networking::GameRuntime::is_active; forwards to the C++ implementation unchanged.
 *
 * @address 0x4ddca0
 */
int32_t network_game_is_active(void)
{
    return halo::networking::GameRuntime::is_active();
}

/**
 * C entry point for halo::networking::GameRuntime::settings_ack_send; forwards to the C++ implementation unchanged.
 *
 * @address 0x4d9f50
 */
char network_game_settings_ack_send(uint8_t *client, int16_t template_row)
{
    return halo::networking::GameRuntime::settings_ack_send(client, template_row);
}

/**
 * C entry point for halo::networking::GameRuntime::settings_broadcast_send; forwards to the C++ implementation unchanged.
 *
 * @address 0x4df0e0
 */
uint32_t network_game_settings_broadcast_send(network_server_globals *server, const network_player_entry *entry)
{
    return halo::networking::GameRuntime::settings_broadcast_send(server, entry);
}

/**
 * C entry point for halo::networking::GameRuntime::start_new_server_with_name_and_password; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e4150
 */
uint8_t network_game_start_new_server_with_name_and_password(uint32_t unused, uint16_t *name, uint16_t *password)
{
    return halo::networking::GameRuntime::start_new_server_with_name_and_password(unused, name, password);
}

/**
 * C entry point for halo::networking::GameRuntime::map_cycle_list_broadcast; forwards to the C++ implementation unchanged.
 *
 * @address 0x4deec0
 */
void network_map_cycle_list_broadcast(void)
{
    halo::networking::GameRuntime::map_cycle_list_broadcast();
}

/**
 * C entry point for halo::networking::GameRuntime::disconnect_with_error; forwards to the C++ implementation unchanged.
 *
 * @address 0x4d97e0
 */
void network_session_disconnect_with_error(int16_t error_code)
{
    halo::networking::GameRuntime::disconnect_with_error(error_code);
}

/**
 * C entry point for halo::networking::GameSessionView::generate_unique_random_name; forwards to the C++ implementation unchanged.
 *
 * @address 0x4df730
 */
void network_game_generate_unique_random_name(network_game_session *session, wchar_t *out_name)
{
    halo::networking::GameSessionView(session).generate_unique_random_name(out_name);
}

/**
 * C entry point for halo::networking::GameSessionView::scenario_load_request; forwards to the C++ implementation unchanged.
 *
 * @address 0x4de6d0
 */
char network_game_scenario_load_request(network_game_session *session)
{
    return halo::networking::GameSessionView(session).scenario_load_request();
}

/**
 * C entry point for halo::networking::GameSessionView::session_reset; forwards to the C++ implementation unchanged.
 *
 * @address 0x4de470
 */
void network_game_session_reset(network_game_session *session)
{
    halo::networking::GameSessionView(session).session_reset();
}

/**
 * C entry point for halo::networking::GameSessionView::assign_random_color; forwards to the C++ implementation unchanged.
 *
 * @address 0x4df790
 */
void network_player_assign_random_color(network_game_session *session, network_player_entry *entry)
{
    halo::networking::GameSessionView(session).assign_random_color(entry);
}

/**
 * C entry point for halo::networking::GameSessionView::add; forwards to the C++ implementation unchanged.
 *
 * @address 0x4de4e0
 */
uint32_t network_player_entry_add(network_game_session *session, network_player_entry *incoming)
{
    return halo::networking::GameSessionView(session).add(incoming);
}

/**
 * C entry point for halo::networking::GameSessionView::find; forwards to the C++ implementation unchanged.
 *
 * @address 0x4de900
 */
char network_player_entry_find(network_game_session *session, network_player_entry *key)
{
    return halo::networking::GameSessionView(session).find(key);
}

/**
 * C entry point for halo::networking::GameSessionView::remove; forwards to the C++ implementation unchanged.
 *
 * @address 0x4de640
 */
uint32_t network_player_entry_remove(network_player_entry *key, network_game_session *session)
{
    return halo::networking::GameSessionView(session).remove(key);
}

/**
 * C entry point for halo::networking::GameSessionView::update_; forwards to the C++ implementation unchanged.
 *
 * @address 0x4de5f0
 */
uint8_t network_player_entry_update(network_player_entry *incoming, network_game_session *session)
{
    return halo::networking::GameSessionView(session).update_(incoming);
}

/**
 * C entry point for halo::networking::GameSessionView::name_collision_check; forwards to the C++ implementation unchanged.
 *
 * @address 0x4df6f0
 */
uint8_t network_player_name_collision_check(network_game_session *session, uint16_t *candidate_name)
{
    return halo::networking::GameSessionView(session).name_collision_check(candidate_name);
}

/**
 * C entry point for halo::networking::SearchEntryView::entry_is_fresh; forwards to the C++ implementation unchanged.
 *
 * @address 0x4da770
 */
uint8_t network_game_search_entry_is_fresh(network_game_search_entry *entry)
{
    return halo::networking::SearchEntryView(entry).entry_is_fresh();
}

/**
 * C entry point for halo::networking::SearchEntryView::results_add_or_update; forwards to the C++ implementation unchanged.
 *
 * @address 0x4da7d0
 */
int32_t network_game_search_results_add_or_update(network_game_search_entry *results, const uint8_t *announcement)
{
    return halo::networking::SearchEntryView(results).results_add_or_update(announcement);
}

/**
 * C entry point for halo::networking::ObjectOwnership::owner_team_index_desired; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e0cf0
 */
int32_t network_object_owner_team_index_desired(object *obj)
{
    return halo::networking::ObjectOwnership::owner_team_index_desired(obj);
}

/**
 * C entry point for halo::networking::ObjectOwnership::release_ownership_claim; forwards to the C++ implementation unchanged.
 *
 * @address 0x4dfc10
 */
void network_object_release_ownership_claim(uint8_t slot_index)
{
    halo::networking::ObjectOwnership::release_ownership_claim(slot_index);
}

/**
 * C entry point for halo::networking::PlayerEntryView::validate; forwards to the C++ implementation unchanged.
 *
 * @address 0x4de9f0
 */
char network_player_entry_validate(network_player_entry *entry)
{
    return halo::networking::PlayerEntryView(entry).validate();
}

/**
 * C entry point for halo::networking::PlayerReports::update_history_log_write_v; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e7f90
 */
void network_player_update_history_log_write(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    halo::networking::PlayerReports::update_history_log_write_v(format, args);
    va_end(args);
}

/**
 * C entry point for halo::networking::HostSession::cd_key_callback; forwards to the C++ implementation unchanged.
 *
 * @address 0x5760a0
 */
void network_session_host_cd_key_callback(int32_t game_id, int32_t local_id, int32_t authenticated, const char *message, void *instance)
{
    halo::networking::HostSession::cd_key_callback(game_id, local_id, authenticated, message, instance);
}

/**
 * C entry point for halo::networking::HostSession::dispose; forwards to the C++ implementation unchanged.
 *
 * @address 0x5778f0
 */
void network_session_host_dispose(void)
{
    halo::networking::HostSession::dispose();
}

/**
 * C entry point for halo::networking::HostSession::natneg_callback; forwards to the C++ implementation unchanged.
 *
 * @address 0x578160
 */
void network_session_host_natneg_callback(int32_t cookie)
{
    halo::networking::HostSession::natneg_callback(cookie);
}

/**
 * C entry point for halo::networking::HostSession::natneg_completed; forwards to the C++ implementation unchanged.
 *
 * @address 0x578120
 */
void network_session_host_natneg_completed(int32_t result, uint32_t socket, const uint8_t *remote_address, void *user_data)
{
    halo::networking::HostSession::natneg_completed(result, socket, remote_address, user_data);
}

/**
 * C entry point for halo::networking::HostSession::qr2_add_error; forwards to the C++ implementation unchanged.
 *
 * @address 0x578100
 */
void network_session_host_qr2_add_error(int32_t error, char *message, void *user_data)
{
    halo::networking::HostSession::qr2_add_error(error, message, user_data);
}

/**
 * C entry point for halo::networking::HostSession::qr2_count; forwards to the C++ implementation unchanged.
 *
 * @address 0x5780c0
 */
int32_t network_session_host_qr2_count(int32_t key_type, void *user_data)
{
    return halo::networking::HostSession::qr2_count(key_type, user_data);
}

/**
 * C entry point for halo::networking::HostSession::qr2_team_key; forwards to the C++ implementation unchanged.
 *
 * @address 0x577f40
 */
void network_session_host_qr2_team_key(int32_t key_id, int32_t index, void *buffer, void *user_data)
{
    halo::networking::HostSession::qr2_team_key(key_id, index, buffer, user_data);
}

/**
 * C entry point for halo::networking::HostSession::reject_or_cleanup_client; forwards to the C++ implementation unchanged.
 *
 * @address 0x575ff0
 */
uint8_t network_session_host_reject_or_cleanup_client(const char *response, const char *challenge, uint32_t ip, int32_t local_id)
{
    return halo::networking::HostSession::reject_or_cleanup_client(response, challenge, ip, local_id);
}

/**
 * C entry point for halo::networking::HostSession::start; forwards to the C++ implementation unchanged.
 *
 * @address 0x577850
 */
int32_t network_session_host_start(void *user_data)
{
    return halo::networking::HostSession::start(user_data);
}

/**
 * C entry point for halo::networking::HostSession::start_info_set; forwards to the C++ implementation unchanged.
 *
 * @address 0x576100
 */
void network_session_host_start_info_set(char *host_name, char *map_name, char *variant_name, int32_t game_type)
{
    halo::networking::HostSession::start_info_set(host_name, map_name, variant_name, game_type);
}

/**
 * C entry point for halo::networking::HostSession::update_; forwards to the C++ implementation unchanged.
 *
 * @address 0x577940
 */
void network_session_host_update(void)
{
    halo::networking::HostSession::update_();
}

/**
 * C entry point for halo::networking::GameRuntime::process_incoming_message; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e1c60
 */
uint32_t network_game_process_incoming_message(int32_t length, network_machine *machine, uint16_t *record, network_server_globals *server)
{
    return halo::networking::GameRuntime::process_incoming_message(length, machine, record, server);
}

/**
 * C entry point for halo::networking::GameRuntime::start_new_server_from_profile; forwards to the C++ implementation unchanged.
 *
 * @address 0x4e40f0
 */
uint8_t network_game_start_new_server_from_profile(uint32_t param_1)
{
    return halo::networking::GameRuntime::start_new_server_from_profile(param_1);
}

/**
 * C entry point for halo::networking::PlayerReports::ping_field_update_and_report; forwards to the C++ implementation unchanged.
 *
 * @address 0x4dbaa0
 */
void network_player_ping_field_update_and_report(void *decode_context)
{
    halo::networking::PlayerReports::ping_field_update_and_report(decode_context);
}

/**
 * C entry point for halo::networking::HostSession::qr2_key_list; forwards to the C++ implementation unchanged.
 *
 * @address 0x577fb0
 */
void network_session_host_qr2_key_list(int32_t key_type, void *keybuffer, void *user_data)
{
    halo::networking::HostSession::qr2_key_list(key_type, keybuffer, user_data);
}

/**
 * C entry point for halo::networking::HostSession::dispatch_message; forwards to the C++ implementation unchanged.
 *
 * @address 0x577e40
 */
void network_session_host_dispatch_message(int32_t key_id, int32_t index, void *buffer, void *user_data)
{
    halo::networking::HostSession::dispatch_message(key_id, index, buffer, user_data);
}

/**
 * C entry point for halo::networking::HostSession::qr2_server_key; forwards to the C++ implementation unchanged.
 *
 * @address 0x5779c0
 */
void network_session_host_qr2_server_key(int32_t key_id, void *buffer, void *user_data)
{
    halo::networking::HostSession::qr2_server_key(key_id, buffer, user_data);
}

/**
 * C entry point for halo::networking::TimerView::advance; forwards to the C++ implementation unchanged.
 *
 * @address 0x4deb50
 */
void network_timer_advance(network_timer_pair *timer)
{
    halo::networking::TimerView(timer).advance();
}

/**
 * C entry point for halo::networking::TimerView::decrement_floored; forwards to the C++ implementation unchanged.
 *
 * @address 0x4debd0
 */
void network_timer_decrement_floored(network_timer_pair *timer, int32_t decrement)
{
    halo::networking::TimerView(timer).decrement_floored(decrement);
}

/**
 * C entry point for halo::networking::TimerView::increment_clamped; forwards to the C++ implementation unchanged.
 *
 * @address 0x4debb0
 */
void network_timer_increment_clamped(network_timer_pair *timer, int32_t upper_bound, int32_t increment)
{
    halo::networking::TimerView(timer).increment_clamped(upper_bound, increment);
}

/**
 * C entry point for halo::networking::TimerView::start; forwards to the C++ implementation unchanged.
 *
 * @address 0x4debf0
 */
void network_timer_start(network_timer_pair *timer, int32_t duration_ms)
{
    halo::networking::TimerView(timer).start(duration_ms);
}

}
