#pragma once

#include "halo/networking/net1_common.hpp"

namespace halo::networking {

/**
 * Behaviour group for the original `network_connection_stats_*` functions; every member is the original function body moved unchanged.
 */
class ConnectionStats {
public:
    ConnectionStats() = delete;

    static void end(int32_t connection_id, uint16_t connection_key);
    static void log_tick();
    static int32_t lookup_or_add(int32_t connection_id, uint16_t connection_key);
    static void record_packet(void *gamespy_connection, int32_t payload_length, uint8_t is_sent, uint8_t is_reliable, uint8_t is_resend);
};

/**
 * Behaviour group for the original `network_*` functions; every member is the original function body moved unchanged.
 */
class NetworkRuntime {
public:
    NetworkRuntime() = delete;

    static void debug_fill_canary_buffer(uint32_t *buffer);
    static void dispatch_initialize();
    static void hostname_thread_proc(char *hostname_buffer);
    static int16_t initialize();
    static int local_hostent_get(void **out_hostent);
    static char * log_path_resolve(char *requested_path);
    static uint8_t name_string_is_valid_for_mode(char *name, void *character, int32_t mode);
    static void password_field_set(uint8_t *object, wchar_t *source);
    static uint16_t * prepare_challenge_packet(int32_t message_type, void *payload);
    static int32_t random_offset(int32_t base);
    static int32_t shutdown();
    static uint8_t signal_quality_glyph(uint32_t code);
    static uint32_t update_();
};

/**
 * Behaviour group for the original `network_event_feed_*` functions; every member is the original function body moved unchanged.
 */
class EventFeed {
public:
    EventFeed() = delete;

    static void flush(int32_t *queue);
    static void queue_append(uint8_t *queue, uint32_t *key, uint32_t *payload);
};

/**
 * Behaviour group for the original `network_index_cache_*` functions; every member is the original function body moved unchanged.
 */
class IndexCache {
public:
    IndexCache() = delete;

    static int32_t find_or_allocate_slot(uint8_t *container, int32_t key);
    static int32_t get(hash_table *table, int32_t key);
    static uint8_t insert_if_free(uint8_t *container, int32_t slot, int32_t key);
    static uint8_t remove(uint8_t *container, int32_t key);
};

/**
 * Behaviour group for the original `network_message_*` functions; every member is the original function body moved unchanged.
 */
class MessageBlocks {
public:
    MessageBlocks() = delete;

    static uint16_t * block_build(uint16_t *buffer, uint32_t *source, uint8_t flags, uint32_t length);
    static uint16_t * read_sized_buffer(uint16_t *buffer, int32_t capacity, bit_stream *stream);
};

/**
 * Behaviour group for the original `network_stats_summary_log_*` functions; every member is the original function body moved unchanged.
 */
class StatsSummaryLog {
public:
    StatsSummaryLog() = delete;

    static void open();
    static void write();
};

}
