#include "halo/networking/net1_server.hpp"
#include <string.h>
#include <stdint.h>
#include <wchar.h>
#include "units.h"
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/effects/api.hpp"
#include "halo/cseries/api.hpp"
#include "halo/main/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/interface/api.hpp"

extern "C" {
extern uint8_t network_incoming_message_scratch[0x510];
typedef struct network_item_stream {
    bit_stream stream;
    uint32_t bit_count;
} network_item_stream;

extern uint8_t network_message_scratch[0x7ff8];
extern network_server_globals *network_server;
extern uint16_t network_challenge_packet_block[];
extern network_client_globals *network_client;
extern player_profile player_profile_cache[16];
extern void player_update_queue_create(player_update_queue *queue);
extern data_array *update_client_queues;
extern data_array *update_server_queues;
extern void game_engine_player_new_life(uint32_t player_datum);
extern int32_t game_engine_player_profile_cache_find(void);
extern data_packet_group network_game_messages_group;
extern data_array *player_data;
extern game_engine_definition *current_game_engine;
extern int32_t game_engine_notify_object_value_event(int32_t team);
extern void game_engine_capture_player_profile(int32_t value);
extern void game_engine_send_unit_weapon_loadout(void *machine, int32_t team, int32_t machine_id);
typedef void (*network_join_complete_callback)(int32_t unused, int32_t machine_id);
extern int32_t network_console_connection_id;
extern uint8_t network_session_active2;
extern void *network_session_host_object;
extern int32_t network_session_host_state;
extern void gcd_disconnect_all(int32_t connection_id);
extern void gcd_shutdown(void);
extern void qr2_shutdown(void *object);
extern network_server_globals network_server_storage;
extern int32_t network_scenario_round_counter_a;
extern uint8_t network_scenario_round_counter_b;
extern int16_t pending_difficulty;
extern void update_server_push_player_tick_history(void);
extern void game_engine_tick(void);
extern uint8_t game_engine_team_is_leading(uint32_t requested_team);
extern char variant_defaults_source[];
typedef struct rcon_request_decode {
    char password[20];
    char command[64];
} rcon_request_decode;
extern char sv_rcon_password_value[9];
extern void *global_white_argb;
extern void chimera__console_out(ColorARGB *color, char *format, ...);
extern int16_t network_join_error_code;
extern uint8_t network_host_handoff_requested;
extern void chat_close(void);
extern int32_t network_server_status_last_print_ms;
extern int32_t network_pending_connection_count;
extern network_pending_connection network_pending_connections[30];
extern void *message_delta_definition_table;
extern int16_t network_game_mode;
extern uint8_t network_server_host_valid;
extern uint32_t split_screen_quit_prompt_string;
extern uint32_t network_join_error_reason;
extern uint32_t network_local_address;
extern char network_build_string[];
}

namespace halo::networking {

/**
 * out/phase4/networking_functions.md: "Small dispatcher used while draining a
 * channel's bitstream, routing each unit to either the queued-message processor or the
 * incoming-packet decoder." network_game_process_incoming_message is already named and in
 * this batch.
 *
 * @address 0x4e18b0
 */
char ServerView::dispatch_bitstream_unit(uint32_t unit, bit_stream *stream, network_machine *machine)
{
    network_server_globals *server = self;
    uint16_t buffer[0x800];

    if (unit == 1) {
        return halo::networking::network_client_drain_queued_updates(server, machine, stream);
    }
    if (unit == 0) {
        uint16_t *record = halo::networking::network_message_read_sized_buffer(buffer, 0xfff, stream);

        if (record != 0) {
            return (char)halo::networking::network_game_process_incoming_message(*record >> 4, machine, record, server);
        }
    }
    return 0;
}

/**
 * out/phase4/networking_functions.md: "Drains a shared bitstream buffer one bit at a
 * time, dispatching each bit through network_channel_dispatch_bitstream_unit as part of connecting a new machine."
 * *param_2 (machine->channel) and channel->incoming (channel+0xc) match
 * network_channel::endpoint/incoming... wait, channel+0xc is actually ::incoming per
 * types/networking.h; channel+0x8/+0xc/+0x10 on the *circular_buffer* itself match
 * read_cursor/write_cursor/capacity. network_channel_incoming_read_item is functions.md's
 * "network_channel_incoming_read_item" (already named there, conf 0.4, not renamed here since
 *
 * @address 0x4e1290
 */
char ServerView::drain_bitstream(network_machine *machine)
{
    network_server_globals *server = self;
    char result = 1;

    for (;;) {
        network_channel *channel = *(network_channel **)machine;
        circular_buffer *incoming = channel->incoming;
        int32_t available;
        int32_t bit_offset = 0;
        int32_t bit_count = 0;
        uint32_t sender[6];
        network_item_stream s;

        if (incoming == 0) {
            return 1;
        }
        available = incoming->write_cursor - incoming->read_cursor;
        if (available < 0) {
            available += incoming->capacity;
        }
        if (available == 0) {
            return 1;
        }
        result = (char)halo::networking::network_channel_incoming_read_item(channel, network_incoming_message_scratch, &bit_offset,
                                                          &bit_count, (s_network_address *)sender, 0x80000);
        if (result == 0) {
            return 0;
        }
        s.stream.unknown_00 = 1;
        s.stream.data = network_incoming_message_scratch;
        s.stream.first_bit = (uint32_t)bit_offset;
        s.stream.byte_cursor = (uint32_t)bit_offset >> 3;
        s.stream.bit_cursor = (uint32_t)bit_offset & 7;
        s.stream.last_bit = (uint32_t)(bit_count + bit_offset - 1);
        s.bit_count = (uint32_t)bit_count;
        if (result == 1) {
            do {
                uint32_t position = s.stream.byte_cursor * 8 + s.stream.bit_cursor;
                uint32_t next;
                uint8_t item_flag;

                if (s.stream.first_bit - s.stream.byte_cursor * 8 - s.stream.bit_cursor + (uint32_t)bit_count < 8) {
                    break;
                }
                if (position < s.stream.first_bit || position > s.stream.last_bit) {
                    result = 0;
                    break;
                }
                item_flag = (uint8_t)((s.stream.data[s.stream.byte_cursor] >> s.stream.bit_cursor) & 1);
                next = position + 1;
                if ((next >= s.stream.first_bit && next <= s.stream.last_bit) || next == s.stream.last_bit + 1) {
                    s.stream.byte_cursor = next >> 3;
                    s.stream.bit_cursor = next & 7;
                }
                result = halo::networking::network_channel_dispatch_bitstream_unit(server, item_flag, &s.stream, machine);
            } while (result == 1);
        }
        if (result == 0) {
            return 0;
        }
    }
}

/**
 * network_server_check_machine_timeout (0x4e0f80 `mov eax,esi` / 0x4e102b
 * `lea eax,[esp+0x20]`), both immediately before the call.
 * For every one of the 16 machine slots with a connected (0..15) machine_id, requires at least
 * one valid player-table entry whose machine_index matches it. Returns 0 as soon as a
 * connected machine has no matching player; returns 1 once all 16 slots have been checked.
 *
 * @address 0x4e04f0
 */
uint32_t ServerView::all_machines_have_player()
{
    network_server_globals *server = self;
    int32_t machine_index;

    for (machine_index = 0; machine_index < 16; machine_index = machine_index + 1) {
        int16_t machine_id;

        machine_id = server->machines[machine_index].machine_id;
        if (machine_id >= 0 && machine_id < 16) {
            char found;
            network_player_entry *entry;
            int32_t i;

            found = 0;
            entry = server->session.players;
            for (i = 0; i < 16; i = i + 1) {
                char valid;

                valid = halo::networking::network_player_entry_validate(entry);
                if (valid != 0 && entry->machine_index == (int8_t)machine_id) {
                    found = 1;
                }
                entry = entry + 1;
            }
            if (!found) {
                return 0;
            }
        }
    }
    return 1;
}

/**
 * network_server_check_machine_timeout (0x4e0f80 `mov eax,esi` / 0x4e102b
 * `lea eax,[esp+0x20]`), both immediately before the call.
 * Returns 1 if the game is in team mode and either team 0 or team 1 currently has zero valid,
 * active players; returns 0 if both teams are populated, or if the game is not in team mode.
 *
 * @address 0x4e0480
 */
uint32_t ServerView::any_team_empty()
{
    network_server_globals *server = self;
    int16_t counts[2];
    network_player_entry *entry;
    int32_t i;

    if (server->session.variant.teams == 0) {
        return 0;
    }
    counts[0] = 0;
    counts[1] = 0;
    entry = server->session.players;
    for (i = 0; i < 16; i = i + 1) {
        char valid;
        int8_t team;

        valid = halo::networking::network_player_entry_validate(entry);
        if (valid != 0) {
            team = entry->team_index;
            if (team >= 0 && team < 2) {
                counts[(int32_t)team] = counts[(int32_t)team] + 1;
            }
        }
        entry = entry + 1;
    }
    for (i = 0; i < 2; i = i + 1) {
        if (counts[i] == 0) {
            return 1;
        }
    }
    return 0;
}

/**
 * out/phase4/networking_functions.md: "Posts a type-0x21 game-engine event and, if
 * accepted, broadcasts an associated update packet to the whole session -- used whenever the
 * connected-player set changes." 0x00871de0 is types/networking.h's shared encode scratch
 * buffer, already named network_message_scratch by
 * src/networking/network_server_check_machine_timeout.c (same address, "shared with
 * network_game_broadcast_team_object_updates.c").
 *
 * @address 0x4e1bf0
 */
uint32_t ServerView::broadcast_player_set_changed(uint8_t *param_1)
{
    int32_t encoded_bits;
    void *record;

    halo::networking::message_delta_parameters_protocol_send_update();
    record = param_1 + 8;
    encoded_bits = halo::networking::message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 0x21, 0, &record, 0, 1, 0);
    if (0 < encoded_bits) {
        halo::networking::network_session_broadcast_to_all(network_server, 1, network_message_scratch, 1, 0, 1, 3);
    }
    return 0 < encoded_bits;
}

/**
 * this module, 0x4e19c0
 * Copies an 8-dword game-state record into a scratch buffer, encodes it as message type 0x17,
 * and broadcasts the encoded block to every established machine in the session.
 * FIXED (register inputs, objdump): the original never reads ESI as an input (it overwrites or only saves it); those parameters arrive on the stack (1 stack argument(s) read).
 *
 * @address 0x4e1b50
 */
uint32_t ServerView::broadcast_state_snapshot(const uint32_t *record)
{
    network_server_globals *server = self;
    uint32_t payload[8];
    uint8_t buffer[0x600];
    int16_t capacity;
    int32_t i;
    uint16_t *encoded;

    for (i = 0; i < 8; i = i + 1) {
        payload[i] = record[i];
    }
    capacity = 0x600;
    if (halo::memory::data_packet_group_encode_packet(&network_game_messages_group, buffer, payload, &capacity, 0x17, 1) != 0) {
        encoded = halo::networking::network_message_block_build(network_challenge_packet_block, (uint32_t *)buffer, 3, (uint32_t)capacity);
        if (encoded != 0) {
            return halo::networking::network_session_broadcast_to_all(server, 0, encoded, 1, 0, 1, 3);
        }
    }
    return 0;
}

/**
 * One-time per-round bookkeeping (first call this round records whether a listen-server
 * client exists and stamps a millisecond timestamp), then scans the 16 player-table slots
 * for the one whose key matches `machine`, finalizes that player's game object (via the
 * stats-logging-aware path selection), records it into player_profile_cache, and hands the
 * result to network_game_server_handoff_object_ownership /
 * network_object_release_ownership_claim.
 *
 * @address 0x4dfc90
 */
void ServerView::handle_client_join(int32_t *object_count_passthrough, network_machine *machine, uint8_t bl_passthrough)
{
    network_server_globals *server = self;
    network_player_entry *entry;
    network_player_entry *scan;
    char valid;
    char handled;
    char ok;
    int32_t i;
    int32_t remaining;
    int32_t *field_9c4;
    uint8_t *unknown_9bc_base;
    uint32_t player_datum;
    int32_t profile_index;
    player_profile *profile;
    int32_t j;

    machine->flags |= 0x04;
    unknown_9bc_base = (uint8_t *)server;
    field_9c4 = (int32_t *)(unknown_9bc_base + 0x9c4);

    if (server->state != 1) {
        int16_t *machine_id_ptr;
        char all_processed_or_invalid;

        all_processed_or_invalid = 1;
        machine_id_ptr = (int16_t *)((uint8_t *)server + 0x3c4);
        for (i = 4; i != 0; i = i - 1) {
            if (machine_id_ptr[0] >= 0 && machine_id_ptr[0] < 16 &&
                (((uint8_t *)machine_id_ptr)[2] & 0x04) == 0) {
                all_processed_or_invalid = 0;
            }
            if (machine_id_ptr[0x30] >= 0 && machine_id_ptr[0x30] < 16 &&
                (((uint8_t *)(machine_id_ptr + 0x30))[2] & 0x04) == 0) {
                all_processed_or_invalid = 0;
            }
            if (machine_id_ptr[0x60] >= 0 && machine_id_ptr[0x60] < 16 &&
                (((uint8_t *)(machine_id_ptr + 0x60))[2] & 0x04) == 0) {
                all_processed_or_invalid = 0;
            }
            if (machine_id_ptr[0x90] >= 0 && machine_id_ptr[0x90] < 16 &&
                (((uint8_t *)(machine_id_ptr + 0x90))[2] & 0x04) == 0) {
                all_processed_or_invalid = 0;
            }
            machine_id_ptr = machine_id_ptr + 0xc0;
        }
        if (all_processed_or_invalid) {
            char has_client;

            has_client = (network_client != 0);
            server->state = 1;
            *field_9c4 = 0;
            server->session.map_loaded = has_client ? *((uint8_t *)network_client + 0xec0) : 0;

        }
        if (*field_9c4 == 0) {
            large_integer counter;

            QueryPerformanceCounter((LARGE_INTEGER *)&counter);
            *field_9c4 = (int32_t)((counter.quad_part * 1000) / halo::cseries::globals().performance_frequency);
        }
    }

    entry = server->session.players;
    remaining = 16;
    for (;;) {
        valid = halo::networking::network_player_entry_validate(entry);
        handled = 0;
        if (valid != 0) {
            scan = server->session.players;
            i = 0;
            do {
                if (scan->machine_index == entry->machine_index &&
                    scan->machine_player_index == entry->machine_player_index) {
                    if ((int16_t)entry->machine_index == machine->machine_id) {
                        if ((server->flags >> 2 & 1) == 0) {
                            ok = halo::networking::network_player_join_finalize(network_client, entry);
                        } else {
                            ok = 0;
                            if (halo::networking::network_player_entry_validate(entry) != 0 && server->state == 1) {

                                ok = halo::networking::network_channel_key_open(entry);
                                if (ok != 0) {
                                    uint32_t slot_handle = halo::networking::player_data_iterator_advance((uint8_t)entry->slot_index);
                                    datum_index queue_handle;

                                    halo::memory::datum_new_at_index_with_salt((datum_index)slot_handle, halo::game::globals().update_client_queues);
                                    queue_handle = halo::memory::datum_new_at_index_with_salt((datum_index)slot_handle, halo::game::globals().update_server_queues);
                                    halo::game::player_update_queue_create(&((update_server_queue *)halo::game::globals().update_server_queues->data)[queue_handle & 0xffff].queue);
                                }
                            }
                        }
                        if (ok != 0) {
                            machine->player_joined = 1;
                            player_datum = halo::networking::player_data_iterator_advance((uint8_t)entry->slot_index) ;
                            halo::game::game_engine_player_new_life(player_datum);
                            if (halo::game::game_engine_player_profile_cache_find((datum_index)player_datum) != -1) {
                                handled = 1;
                            } else {

                                for (j = 0; j < 16; j = j + 1) {
                                    profile = &player_profile_cache[j];
                                    if (profile->in_use == 0) {
                                        profile->in_use = 1;
                                        profile->player = (datum_index)player_datum;
                                        halo::game::globals().profile_cache_count = halo::game::globals().profile_cache_count + 1;
                                        break;
                                    }
                                }
                                handled = 1;
                            }
                        }
                    }
                    break;
                }
                i = i + 1;
                scan = scan + 1;
            } while (i < 16);
        }
        if (handled) {
            halo::networking::network_game_server_handoff_object_ownership(object_count_passthrough, server, machine);
            halo::networking::network_object_release_ownership_claim(bl_passthrough);
        }
        entry = entry + 1;
        remaining = remaining - 1;
        if (remaining == 0) {
            return;
        }
    }
}

/**
 * out/phase4/networking_functions.md: "Handles message type 0x1a, sending a full
 * server-info reply to not-yet-established machines or otherwise forwarding to network_game_server_handle_client_join"
 * -- network_game_server_handle_client_join, already written. `*unaff_EDI + 0xa98` matches
 * network_channel::connected via network_machine::channel (offset 0), and
 * `unaff_ESI + 0xa0f` matches network_server_globals::game_over, exactly as in the type-0xe and
 * type-0xf handlers.
 *
 * @address 0x4e2700
 */
uint32_t ServerView::handle_info_request(network_machine *machine, uint8_t *record, int32_t length)
{
    network_server_globals *server = self;
    uint32_t body[1];
    int16_t out_type;
    uint16_t version_used;
    int16_t state = *(int16_t *)((uint8_t *)server + 4);
    uint8_t *connection;

    if ((state != 0 && state != 1) || halo::memory::data_packet_group_decode_packet((length -= 2, (int16_t *)&length), &network_game_messages_group, body, record + 2, &out_type, &version_used, 5) == 0) {
        return 0;
    }
    connection = machine != 0 ? *(uint8_t **)machine : 0;
    if ((connection != 0 && connection[0xa98] != 0) || *((uint8_t *)server + 0xa0f) == 0) {
        halo::networking::network_game_server_handle_client_join(0 , server, machine, 1);
        return 1;
    }
    return (uint8_t)halo::networking::network_server_build_full_game_info_packet(machine);
}

/**
 * For each of the 16 player-table slots, if the slot's (machine_index, machine_player_index)
 * key does not already belong to `machine`, locates the network_machine record that does own
 * it and, when that machine's channel is established and its player is a live, non-frozen
 * unit, transfers network ownership of that unit to `machine`. Invokes the completion
 * callback at current_game_engine+0x90 once all 16 slots are processed.
 *
 * @address 0x4dfa10
 */
void ServerView::handoff_object_ownership(int32_t *object_count_passthrough, network_machine *machine)
{
    network_server_globals *server = self;
    int32_t bytes_sent;
    int32_t machine_id;
    network_player_entry *entry;
    network_player_entry *scan;
    int32_t remaining;
    int32_t i;
    int8_t key_machine_index;
    network_machine *owner;
    uint32_t datum;
    int16_t player_index;
    int16_t salt;
    player *plr;
    int32_t team;
    int32_t unit;
    object_header *hdr;
    object *unit_obj;

    bytes_sent = 0;
    machine_id = (int32_t)machine->machine_id;
    halo::networking::network_game_broadcast_team_object_updates(object_count_passthrough, 0, &bytes_sent);
    halo::networking::network_game_broadcast_team_object_updates(object_count_passthrough, 0, &bytes_sent);

    entry = server->session.players;
    remaining = 16;
    for (;;) {
        int do_transfer;

        do_transfer = 0;
        owner = 0;
        if (halo::networking::network_player_entry_validate(entry) != 0) {
            key_machine_index = entry->machine_index;
            scan = server->session.players;
            i = 0;
            do {
                if (scan->machine_index == key_machine_index &&
                    scan->machine_player_index == entry->machine_player_index) {
                    if ((int16_t)key_machine_index != machine->machine_id) {
                        owner = 0;
                        for (i = 0; i <= 15; i = i + 1) {
                            if (server->machines[i].machine_id == (int16_t)key_machine_index) {
                                owner = &server->machines[i];
                                break;
                            }
                        }
                        do_transfer = 1;
                    }
                    break;
                }
                i = i + 1;
                scan = scan + 1;
            } while (i < 16);
        }

        if (do_transfer && owner != 0 && (owner->flags & 0x04) != 0) {

            datum = halo::networking::player_data_iterator_advance((uint8_t)entry->slot_index) ;
            if (datum != 0xffffffff) {
                player_index = (int16_t)datum;
                if (player_index >= 0 && player_index < halo::game::globals().player_data->maximum_count) {
                    plr = (player *)((uint8_t *)halo::game::globals().player_data->data + halo::game::globals().player_data->size * player_index);
                    if (plr->identifier != 0) {
                        salt = (int16_t)(datum >> 16);
                        if (salt == 0 || plr->identifier == salt) {
                            team = plr->team;
                            unit = plr->unit;
                            halo::game::game_engine_notify_object_value_event((uint8_t)entry->slot_index, (int32_t)datum, machine_id, (void *)(uintptr_t)team);
                            halo::networking::build_player_full_resync_update(machine_id);
                            int32_t profile_slot = halo::game::game_engine_player_profile_cache_find((datum_index)datum);
                            if (profile_slot != -1) {
                                halo::game::game_engine_capture_player_profile(profile_slot, 0);
                            }
                            if ((uint32_t)unit != 0xffffffff) {
                                hdr = &((object_header *)halo::objects::globals().object_data->data)[unit & 0xffff];
                                unit_obj = hdr->data;
                                if ((unit_obj->vitality_flags & 0x04) == 0) {
                                    halo::game::game_engine_send_unit_weapon_loadout((uint32_t)unit, (datum_index)datum, (int32_t)team, machine_id);
                                }
                            }
                        }
                    }
                }
            }
        }

        entry = entry + 1;
        remaining = remaining - 1;
        if (remaining == 0) {
            if (halo::game::globals().current_engine != 0 &&
                *(void **)((uint8_t *)halo::game::globals().current_engine + 0x90) != 0) {
                ((network_join_complete_callback)(*(void **)((uint8_t *)halo::game::globals().current_engine + 0x90)))(0, machine_id);
            }
            return;
        }
    }
}

/**
 * out/phase4/networking_functions.md: "Allocates and installs the network host globals
 * (network_server) via network_game_server_host_new and seeds its randomisation salt field from the global PRNG
 * state, mirroring the salt into the network-game globals when present." network_server
 * (0x0071c2d4) and network_client (0x0071c2d8) match types/networking.h.
 *
 * @address 0x4ddd40
 */
int32_t ServerView::host_create()
{
    network_server_globals *host;
    uint32_t salt;

    host = (network_server_globals *)halo::networking::network_game_server_host_new();
    network_server = host;
    if (host != 0) {
        halo::math::globals().effect_random_seed = halo::math::globals().effect_random_seed * 0x19660d + 0x3c6ef35f;
        salt = halo::math::globals().effect_random_seed >> 0x10;
        *(uint32_t *)((uint8_t *)host + 0x3ac) = salt;
        if (network_client != 0) {
            *(uint32_t *)((uint8_t *)network_client + 0xeb8) = salt;
        }
    }
    return host != 0;
}

/**
 * out/phase4/networking_functions.md: "Tears down the network host globals block
 * passed in param_1: disposes each of its 16 team/player sub-entries, frees its allocation,
 * zeroes the structure, and shuts down the associated transport connection." host->flags bit2
 * (+6, stats logging), machines[16] at +0x3b8 stride 0x60 with channel/machine_id/flags, and the
 * final 0x284-dword (0xa10-byte) zero of the whole network_server_globals all match
 * types/networking.h exactly.
 *
 * @address 0x4deda0
 */
void ServerView::host_dispose()
{
    network_server_globals *host = self;
    int32_t i;
    network_machine *machine;
    int32_t challenge_packet;
    int32_t message_type;
    uint32_t challenge_payload[4];

    gcd_disconnect_all(network_console_connection_id);
    if (((*(uint8_t *)((uint8_t *)host + 6) >> 2) & 1) != 0) {
        halo::networking::message_delta_parameters_protocol_dump_to_config_file();
        halo::networking::network_stats_summary_log_write();
    }
    if ((host->state == 0 || host->state == 2)) {
        message_type = (host->state == 2) ? 0x22 : 0x0b;

        challenge_packet = (int32_t)halo::networking::network_prepare_challenge_packet(message_type, challenge_payload);
        if (challenge_packet != 0) {
            halo::networking::network_session_broadcast_to_all(network_server, 0, (void *)(uintptr_t)challenge_packet,
                1, 0, 1, 3);
        }
    }
    for (i = 0; i < 16; i++) {
        machine = &host->machines[i];
        if (machine->machine_id != -1 && (machine->channel->flags & k_network_channel_dead) == 0) {

            halo::networking::network_channel_service(machine->channel, 15000, 0);
        }
    }
    if (host->listen_channel != 0) {
        halo::networking::network_channel_delete(host->listen_channel);
    }
    memset(host, 0, sizeof(*host));
    network_session_active2 = 0;
    if (network_session_host_object != 0) {
        if (network_session_host_state != 2) {
            network_session_host_state = 2;
        }
        halo::networking::network_session_host_update();
        network_console_connection_id = -1;
        gcd_shutdown();
        qr2_shutdown(network_session_host_object);
        network_session_host_object = 0;
    }
}

/**
 * types/networking.h's own comment cites this address: "network_game_server_host_new
 * (0x4dec40) zeroes 0x284 dwords of 0x00861340, which is the size of network_server_globals."
 * Every field this function sets after the zero (flags bit1, session.message_callback,
 * session.unknown_19e, and the 16-entry machines[] init matching network_machine's
 * channel/unknown_04/unknown_08/machine_id/flags/unknown_50/unknown_51/unknown_52/unknown_56/
 * unknown_5c fields, including the header's own "unaligned in the original" note on
 * unknown_52/unknown_56) matches types/networking.h exactly.
 *
 * @address 0x4dec40
 */
void * ServerView::host_new()
{
    network_server_globals *host;
    int32_t i;
    network_machine *machine;

    memset(&network_server_storage, 0, sizeof(network_server_storage));
    host = &network_server_storage;
    network_session_active2 = 1;
    network_scenario_round_counter_a = 0;
    network_scenario_round_counter_b = 0;
    host->full_state_broadcast_pending = 0;
    host->game_over = 0;
    halo::networking::network_channels_open();
    host->listen_channel = halo::networking::network_channel_new(k_network_channel_listening);
    if (host->listen_channel != 0) {
        host->flags = host->flags | 2;
        host->state = 0;
        halo::networking::network_game_session_reset(&host->session);
        host->session.message_callback = (void *)halo::networking::network_session_reject_pending_connection_callback;
        host->session.difficulty = pending_difficulty;
        *(int32_t *)((uint8_t *)host + 0x3b0) = -1;
        for (i = 0; i < 16; i++) {
            machine = &host->machines[i];
            machine->channel = 0;
            machine->last_update_id = 0;
            machine->unknown_08 = 0;
            machine->machine_id = -1;
            machine->flags = 0;
            machine->unknown_0f = 0;
            *(uint32_t *)((uint8_t *)machine + 0x52) = 0;
            *(uint32_t *)((uint8_t *)machine + 0x56) = 0;
            machine->gcd_user_id = -1;
            machine->player_joined = 0;
            machine->players_removed_broadcast = 0;
        }
        host->update_tick = 0;
        host->first_join_ms = 0;
        host->handshake_timer.remaining_ms = 0;
        host->handshake_timer.last_tick_ms = 0;
        host->last_challenge_sent_ms = 0;
        host->unknown_9d0 = 0;
        host->scenario_announced = 0;
        host->new_server_pending = 0;
        host->join_finalize_pending = 0;
        *(int32_t *)((uint8_t *)host + 0x3b0) = *(int32_t *)((uint8_t *)host + 0x3b0) + 1;
        *(uint32_t *)&host->handshake_state = 0;
        if (halo::networking::network_game_session_reset_defaults(host) != 0) {
            halo::networking::network_session_host_start(0);
            goto done;
        }
    }
    halo::networking::network_game_server_host_dispose(host);
    host = 0;
done:
    if (host != 0 && ((*((uint8_t *)host + 6) >> 2) & 1) != 0) {
        halo::networking::message_delta_protocol_initialize();
        halo::networking::network_stats_summary_log_open();
    }
    return host;
}

/**
 * While the server is in state 1 (client-processing), drains `update_count` queued update
 * packets and, if a deferred "process this machine's queued update" request is pending
 * (unknown_9f8), locates the matching machine by its saved id and finalizes/broadcasts its
 * join. In state 2, just ticks the game engine directly.
 *
 * @address 0x4e03c0
 */
void ServerView::per_frame_tick(int16_t update_count)
{
    network_server_globals *server = self;
    if (server->state == 1) {
        if (update_count > 0) {
            uint32_t remaining;
            large_integer counter;

            remaining = (uint32_t)update_count;
            do {
                server->update_tick = server->update_tick + 1;
                halo::game::update_server_push_player_tick_history();
                QueryPerformanceCounter((LARGE_INTEGER *)&counter);
                remaining = remaining - 1;
            } while (remaining != 0);
        }
        if (server->join_finalize_pending != 0) {
            int32_t i;
            int8_t saved_machine_id;
            network_machine *machine;

            saved_machine_id = *((int8_t *)server + 0x9f4);
            i = 0;
            while (server->machines[i].machine_id != (int16_t)saved_machine_id) {
                i = i + 1;
                if (i > 0xf) {
                    server->join_finalize_pending = 0;
                    return;
                }
            }
            machine = &server->machines[i];
            if (machine != 0) {
                char ok;

                network_player_entry *entry = &server->pending_join_entry;

                ok = halo::networking::network_game_session_finalize_and_add_player(entry, server, machine);
                if (ok != 0) {
                    halo::networking::network_game_broadcast_state_snapshot((const uint32_t *)entry, server);
                }
            }
            server->join_finalize_pending = 0;
        }
    } else if (server->state == 2) {
        halo::game::game_engine_tick();
    }
}

/**
 * Clears byte +0x1c of an opaque broadcast-message context, per the function's (unresolved)
 * name.
 *
 * @address 0x4e4ef0
 */
void ServerView::send_message_to_all_machines_ingame(uint8_t *context)
{
    context[0x1c] = 0;
}

/**
 * Rejects the '%' and '|' escape characters from a candidate player name, regenerates a
 * fresh random name on any collision or reserved character, and assigns a random colour to
 * a still-unassigned slot, before handing the finished entry to network_player_entry_add.
 * Only runs when the entry's machine_index already matches the caller's machine.
 *
 * @address 0x4df840
 */
uint32_t ServerView::session_finalize_and_add_player(network_player_entry *entry, network_machine *machine)
{
    network_server_globals *server = self;
    wchar_t reserved[4];
    wchar_t *hit;

    if (machine->machine_id != (int16_t)entry->machine_index) {
        return (uint32_t)entry & 0xffffff00;
    }
    reserved[0] = L'%';
    reserved[1] = L'\0';
    reserved[2] = L'|';
    reserved[3] = L'\0';
    if (entry->team_index == -1) {
        entry->team_index = (int8_t)halo::game::game_engine_team_is_leading(0xffffffff);
    }
    if (entry->name[0] == L'\0') {
        halo::networking::network_game_generate_unique_random_name((network_game_session *)server, (wchar_t *)entry->name);
    }
    hit = wcsstr((wchar_t *)entry->name, reserved);
    if (hit != 0 || (hit = wcsstr((wchar_t *)entry->name, reserved + 2), hit != 0)) {
        halo::networking::network_game_generate_unique_random_name((network_game_session *)server, (wchar_t *)entry->name);
    }
    if (halo::networking::network_player_name_collision_check((network_game_session *)server, (uint16_t *)entry->name) == 0) {
        halo::networking::network_game_generate_unique_random_name((network_game_session *)server, (wchar_t *)entry->name);
    }
    if (entry->color_index == -1) {
        halo::networking::network_player_assign_random_color((network_game_session *)server, entry);
    }
    return halo::networking::network_player_entry_add(&server->session, entry);
}

/**
 * Resets `server`'s session to compiled-in defaults: copies the pending game variant, the
 * default server name, and clears the two fields between them, then marks the session and the
 * listen channel as initialized.
 * FIXED: every ret of the original is preceded by mov eax,1 and callers test it
 *
 * @address 0x4e1820
 */
int32_t ServerView::session_reset_defaults()
{
    network_server_globals *server = self;
    memcpy(&server->session.variant, &halo::game::globals().pending_variant, sizeof(game_variant));
    strncpy(server->session.server_name, variant_defaults_source, 0x3f);
    server->session.server_name[0x3f] = 0;
    server->session.unknown_07e = 0;
    server->session.unknown_080 = 0;
    server->flags |= 1;
    server->listen_channel->listening = 1;
    return 1;
}

/**
 * Finds the machines[] slot whose machine_id equals machine_id and clears its player_joined
 * flag, returning that slot's address (with its low byte masked off). If no slot matches the
 * original stores through a null pointer (offset 0x50 from zero); this version returns 0 instead.
 *
 * @address 0x4e0b90
 */
uint32_t ServerView::clear_flag_by_id(int32_t machine_id)
{
    network_server_globals *server = self;
    int32_t i;

    for (i = 0; i < 16; i = i + 1) {
        if (server->machines[i].machine_id == machine_id) {
            server->machines[i].player_joined = 0;
            return ((uint32_t)&server->machines[i]) & 0xffffff00;
        }
    }
    return 0;
}

/**
 * Returns the machines[] slot whose machine_id equals `machine_id`, or NULL if none matches.
 *
 * @address 0x4e0810
 */
network_machine * ServerView::find_by_id(int32_t machine_id)
{
    network_server_globals *server = self;
    int32_t i;

    for (i = 0; i < 16; i = i + 1) {
        if (server->machines[i].machine_id == machine_id) {
            return &server->machines[i];
        }
    }
    return 0;
}

/**
 * this module, 0x4e19c0
 *
 * @address 0x4df290
 */
void ServerView::advance_connect_state()
{
    network_server_globals *server = self;
    int32_t challenge_packet;
    uint32_t challenge_payload[4];

    if (*(int16_t *)((uint8_t *)server + 4) == 1) {
        *(int16_t *)((uint8_t *)server + 4) = 2;

        challenge_packet = (int32_t)halo::networking::network_prepare_challenge_packet(0x19, challenge_payload);
        if (challenge_packet != 0) {
            halo::networking::network_session_broadcast_to_all(server, 0, (void *)(uintptr_t)challenge_packet,
                1, 0, 1, 3);
        }
    }
}

/**
 * Returns 1 if no machine slot is both connected (id 0..15) and free of
 * k_network_machine_version_mismatch; returns 0 as soon as one is found.
 * FIXED (objdump): every ret sets only AL; the upper bits of EAX are left as they were
 *
 * @address 0x4e14e0
 */
uint8_t ServerView::any_machine_awaiting_flag()
{
    network_server_globals *server = self;
    int32_t i;

    for (i = 0; i < 16; i = i + 1) {
        int16_t id;

        id = server->machines[i].machine_id;
        if (id >= 0 && id <= 15 && (server->machines[i].flags & k_network_machine_version_mismatch) == 0) {
            return 0;
        }
    }
    return 1;
}

/**
 * Counts machine-table slots that have both a live channel and a connected (non -1) id.
 *
 * @address 0x4e1880
 */
int32_t ServerView::count_connected_machines()
{
    network_server_globals *server = self;
    int32_t count;
    int32_t i;

    count = 0;
    for (i = 0; i < 16; i = i + 1) {
        if (server->machines[i].channel != 0 && server->machines[i].machine_id != -1) {
            count = count + 1;
        }
    }
    return count;
}

/**
 * Admits a channel the listener just accepted as a new machine of the host. It fails (0) unless the session is
 * initialised, a machine slot is free and the peer resolves to a non-zero IPv4 address that is either loopback
 * or this machine's own (any peer passes while network_disconnect_timeout_flag is set). The slot is reset as an
 * established machine with no timers, no player and the next connection id, the channel joins the listener's
 * channel list, and the machine is sent its game info packet: the full one when the game is over and the channel
 * is not connected yet, the short one otherwise. Returns 1 when the channel could be listed.
 *
 * @address 0x4e0d30
 */
uint8_t ServerView::count_machines_and_resolve_address(network_channel *channel)
{
    network_server_globals *server = self;
    static int32_t next_connection_id;
    network_machine *machine;
    network_resolved_address peer = {};
    int32_t slot;
    int32_t connection_id;
    uint8_t listed;

    if ((server->flags & 1) == 0) {
        return 0;
    }
    for (slot = 0; slot < 16; slot = slot + 1) {
        if (server->machines[slot].machine_id == -1) {
            break;
        }
    }
    if (slot == 16) {
        return 0;
    }

    if (channel->endpoint != 0 && halo::networking::network_channel_get_remote_address(&peer.address, channel->endpoint) != 0) {
        peer = {};
    }
    if (peer.address.ipv4 == 0) {
        return 0;
    }
    if (halo::networking::globals().disconnect_timeout_flag == 0 && peer.address.ipv4 != 0x7f000001 && peer.address.ipv4 != network_local_address) {
        return 0;
    }

    machine = &server->machines[slot];
    machine->channel = channel;
    machine->timer_14 = 0;
    machine->timer_18 = 0;
    machine->machine_id = (int16_t)slot;
    machine->flags = k_network_machine_established;
    machine->unknown_0f = 0;
    machine->disconnect_timer_active = 0;
    machine->player_joined = 0;
    machine->players_removed_broadcast = 0;
    machine->unknown_52 = 0;
    machine->unknown_56 = 0;
    connection_id = next_connection_id;
    next_connection_id = next_connection_id + 1;
    if (connection_id == -1) {
        connection_id = next_connection_id;
        next_connection_id = next_connection_id + 1;
    }
    machine->gcd_user_id = connection_id;

    listed = halo::networking::network_channel_list_add(channel->endpoint, server->listen_channel->listen_list) == 0;
    if (machine->channel != 0 && machine->channel->connected != 0) {
        halo::networking::network_server_build_game_info_packet(server, machine);
    } else if (server->game_over != 0) {
        halo::networking::network_server_build_full_game_info_packet(machine);
    } else {
        halo::networking::network_server_build_game_info_packet(server, machine);
    }
    return listed;
}

/**
 * Server-side handler for an incoming rcon-request message: decodes it, validates the password
 * against sv_rcon_password_value, executes the command if both the server has rcon enabled and
 * the password matches, and reports the outcome back to the requesting client via
 * chimera__rcon_out plus a server console log line.
 *
 * @address 0x4e4f00
 */
void ServerView::handle_rcon_request(network_player_entry *client, void *message)
{
    int16_t machine_id = client->machine_index;
    rcon_request_decode decode;

    if (*(int32_t *)*(int32_t *)message != 0) {
        halo::networking::message_delta_decode_compound_field_staged((void **)message);
        halo::interface::chimera__console_out((ColorARGB *)global_white_argb, (char *)"Ignoring meaningless rcon_request message from client #%d", machine_id);
        return;
    }
    memset(&decode, 0, sizeof(decode));
    if (halo::networking::message_delta_decode_compound_field((void **)message, &decode) == 0) {
        halo::interface::chimera__console_out((ColorARGB *)global_white_argb, (char *)"Could not decode rcon message from client #%d", machine_id);
        return;
    }
    if (sv_rcon_password_value[0] == 0) {
        halo::networking::chimera__rcon_out((char *)"rcon command ignored (rcon is disabled)", machine_id);
        halo::interface::chimera__console_out((ColorARGB *)global_white_argb, (char *)"Ignoring rcon request from client #%d (rcon is disabled)", machine_id);
        return;
    }
    if (strcmp(sv_rcon_password_value, decode.password) != 0) {
        halo::networking::chimera__rcon_out((char *)"rcon command ignored (bad password)", machine_id);
        halo::interface::chimera__console_out((ColorARGB *)global_white_argb, (char *)"Ignoring rcon request from client #%d (bad password)", machine_id);
        return;
    }
    if (decode.command[0] == 0) {
        halo::networking::chimera__rcon_out((char *)"rcon command ignored (empty)", machine_id);
        halo::interface::chimera__console_out((ColorARGB *)global_white_argb, (char *)"Ignoring rcon request from client #%d (empty command)", machine_id);
        return;
    }
    halo::main::console_process_rcon_command(machine_id, decode.command);
    {
        halo::networking::chimera__rcon_out((char *)"rcon command finished", machine_id);
        halo::interface::chimera__console_out((ColorARGB *)global_white_argb, (char *)"Successfully executed rcon command from client #%d.", machine_id + 1);
        return;
    }
    halo::networking::chimera__rcon_out((char *)"rcon command failed", machine_id);
    halo::interface::chimera__console_out((ColorARGB *)global_white_argb, (char *)"Failure executing rcon command from client #%d.", machine_id);
}

/**
 * CX reason, EDI machine, stack server. For a machine whose
 * channel is connected (+0xa98): the chat close deadline (0x00718fa4, when unset) becomes reason + 0x2b, the host
 * hand-off flag is set, chat closes; returns 1. Otherwise a type 6 packet carrying the reason goes to the machine
 * (reliable, 3) and the machine timer restarts for 1000 ms; returns whether the send worked (0 when no packet was
 * built). The server argument was missing.
 *
 * @address 0x4e0af0
 */
uint8_t ServerView::notify_or_resend_challenge(int16_t reason, network_machine *machine)
{
    network_server_globals *server = self;
    int32_t payload = reason;
    uint16_t *packet;
    uint8_t ok = 1;

    if (machine != 0 && machine->channel != 0 && machine->channel->connected != 0) {
        if (network_join_error_code == -1) {
            network_join_error_code = (int16_t)(reason + 0x2b);
        }
        network_host_handoff_requested = 1;
        halo::interface::chat_close();
        return 1;
    }
    packet = halo::networking::network_prepare_challenge_packet(6, &payload);
    if (packet == 0 || halo::networking::network_session_send_to_machine(machine->machine_id, server, 0, packet, (uint32_t)(*packet >> 4) << 3,
                                                        1, 1, 0, 3) == 0) {
        ok = 0;
    }
    halo::networking::network_machine_timer_start(machine, 1000);
    return ok;
}

/**
 * Copies `server`'s password (up to 8 wide characters) into `dest` and NUL-terminates it.
 *
 * @address 0x4e0930
 */
void ServerView::password_get(wchar_t *dest)
{
    network_server_globals *server = self;
    wcsncpy(dest, (wchar_t *)server->password, 8);
    dest[8] = 0;
}

/**
 * True if `server`'s join password is not the empty string.
 *
 * @address 0x4e08e0
 */
int32_t ServerView::password_is_set()
{
    network_server_globals *server = self;
    return wcsncmp((wchar_t *)server->password, L"", 8) != 0;
}

/**
 * Copies up to 8 wide characters from `source` into `server`'s password field and forces a
 * NUL terminator.
 *
 * @address 0x4e0910
 */
void ServerView::password_set(const wchar_t *source)
{
    network_server_globals *server = self;
    wcsncpy((wchar_t *)server->password, source, 8);
    server->password[8] = 0;
}

/**
 * While server's stats-logging flag is set, prints the dedicated server status roughly every
 * 15000ms.
 *
 * @address 0x4e1520
 */
uint32_t ServerView::status_periodic_print()
{
    network_server_globals *server = self;
    large_integer counter;

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    if ((server->flags >> 2 & 1) != 0) {
        int32_t now_ms;

        QueryPerformanceCounter((LARGE_INTEGER *)&counter);
        now_ms = (int32_t)((counter.quad_part * 1000) / halo::cseries::globals().performance_frequency);
        if ((uint32_t)(now_ms - network_server_status_last_print_ms) > 15000) {
            halo::networking::sv_status();
            network_server_status_last_print_ms = now_ms;
        }
    }
    return 1;
}

/**
 * Reads the build/version word from the most recently queued pending connection and checks it
 * against an accepted range, then -- if in range -- checks the session has room and is ready,
 * finally searching for a free machine slot.
 *
 * @address 0x4e0850
 */
int32_t ServerView::validate_join_request()
{
    network_server_globals *server = self;
    int32_t build;

    if (network_pending_connection_count > 0) {
        build = network_pending_connections[network_pending_connection_count - 1].first_payload_word;
    }

    if (build > 0x9663f) {
        if (build > 0x96640) {
            return 5;
        }
        if (server->session.player_count < (int16_t)(int8_t)server->session.maximum_players ) {
            int32_t i;

            if ((server->flags & 1) == 0) {
                return 7;
            }
            for (i = 0; i < 16; i = i + 1) {
                if (server->machines[i].machine_id == -1) {
                    return 0;
                }
            }
        }
        return 6;
    }
    return 4;
}

/**
 * Sends `data` through network_channel_queue_message to every machine slot whose flags bit 0x02 is set and
 * whose channel is alive (not k_network_channel_dead) and either connected or `force` is set.
 * Returns false if any qualifying send fails.
 *
 * @address 0x4e19c0
 */
char ServerView::broadcast_to_all(int32_t param_1, void *data, int32_t param_3, int32_t param_4, char force, int32_t param_6)
{
    network_server_globals *server = self;
    char ok;
    int32_t i;

    ok = 1;
    for (i = 0; i < 16; i = i + 1) {
        network_machine *machine;
        network_channel *channel;
        char connected;

        machine = &server->machines[i];
        channel = machine->channel;
        connected = (channel != 0) ? channel->connected : 0;

        if ((machine->flags & 0x02) != 0 &&
            (connected != 1 || force != 0) &&
            channel != 0 &&
            (channel->flags & 0x10) == 0) {
            uint8_t status;
            char sent;

            status = (uint8_t)(param_1 != 0);
            sent = halo::networking::network_channel_queue_message(channel, (uint32_t)(uintptr_t)data, (uint32_t)(uintptr_t)&status, 1, param_3, param_4, param_6);
            if (sent == 0) {
                ok = 0;
            }
        }
    }
    return ok;
}

/**
 * out/phase4/networking_functions.md's summary claims message type 0x1b, but
 * network_game_process_incoming_message.c's own verified dispatch switch (this batch) shows
 * `case 0x1c: network_game_client_handle_map_data(param_1);` -- type 0x1b is instead handled inline inside the
 * dispatcher itself (decode + a direct call to network_game_client_apply_position_update). The
 * dispatcher's literal call table is trusted here instead of the low-confidence (0.3) summary.
 * This function latches an incoming 32-byte game/map data block into the client's pending-state
 * fields.
 *
 * @address 0x4e2790
 */
uint32_t ServerMessageHandlers::client_map_data(uint8_t *record, int32_t length)
{
    network_server_globals *server = self;
    uint32_t body[8];
    int16_t out_type;
    uint16_t version_used;
    uint8_t *s = (uint8_t *)server;

    if (*(int16_t *)(s + 4) == 1 && halo::memory::data_packet_group_decode_packet((length -= 2, (int16_t *)&length), &network_game_messages_group, body, record + 2, &out_type, &version_used, 5) != 0 && s[0x9f8] == 0 &&
        halo::networking::network_player_entry_validate((network_player_entry *)body) != 0) {
        memcpy(s + 0x9d8, body, sizeof(body));
        s[0x9f8] = 1;
    }
    return 1;
}

/**
 * out/phase4/networking_functions.md's summary claims message type 0x1d, but
 * network_game_process_incoming_message.c's own verified dispatch switch (this batch) shows
 * `case 0x1e: network_game_client_handle_retry_schedule();` (case 0x1d instead reaches network_game_client_handle_settings_relay, the settings relay) --
 * the dispatcher's literal call table is trusted here instead of the low-confidence (0.3)
 * summary. Forwards to network_machine_timer_start; the client-side counterpart of the
 * network_game_message_handle_build_version server handler, gated on role == 1 and decode class 5.
 *
 * @address 0x4e2870
 */
uint32_t ServerMessageHandlers::client_retry_schedule(network_machine *machine, uint8_t *record, int32_t length)
{
    network_server_globals *server = self;
    uint32_t body[1];
    int16_t out_type;
    uint16_t version_used;

    if (*(int16_t *)((uint8_t *)server + 4) == 1 && halo::memory::data_packet_group_decode_packet((length -= 2, (int16_t *)&length), &network_game_messages_group, body, record + 2, &out_type, &version_used, 5) != 0) {
        halo::networking::network_machine_timer_start(machine, 0);
    }
    return 1;
}

/**
 * out/phase4/networking_functions.md's summary claims message type 0x1c, but
 * network_game_process_incoming_message.c's own verified dispatch switch (this batch) shows
 * `case 0x1d: network_game_client_handle_settings_relay();` (case 0x1c instead reaches network_game_client_handle_map_data, the map-data latch) --
 * the dispatcher's literal call table is trusted here instead of the low-confidence (0.3)
 * summary. Forwards to network_game_settings_broadcast_send, the same forward the server-side
 * network_game_message_handle_settings_relay handler makes, but gated on role == 1 (client) and decode class 5 instead of 3.
 *
 * @address 0x4e2810
 */
uint32_t ServerMessageHandlers::client_settings_relay(uint8_t *record, int32_t length)
{
    network_server_globals *server = self;
    network_player_entry body;
    int16_t out_type;
    uint16_t version_used;

    if (*(int16_t *)((uint8_t *)server + 4) == 1 && halo::memory::data_packet_group_decode_packet((length -= 2, (int16_t *)&length), &network_game_messages_group, &body, record + 2, &out_type, &version_used, 5) != 0) {
        return halo::networking::network_game_settings_broadcast_send(server, &body);
    }
    return 1;
}

/**
 * out/phase4/networking_functions.md's summary claims message types 0x14/0x25, but
 * network_game_process_incoming_message.c's own verified dispatch switch (this batch) shows
 * `case 0x15: network_game_message_handle_build_version();` -- the low-confidence (0.3) summary swapped this function's type
 * number with network_game_message_handle_retry_schedule's (which the same switch shows at `case 0x14: case 0x25:`); the
 * dispatcher's literal call table is trusted here instead. Forwards to
 * network_machine_check_build_version (network_machine_check_build_version, already written: EAX -> remote_version,
 * EDI -> machine).
 *
 * @address 0x4e2630
 */
uint32_t ServerMessageHandlers::build_version(network_machine *machine, uint8_t *record, int32_t length)
{
    network_server_globals *server = self;
    char body[0x100];
    int16_t out_type;
    uint16_t version_used;

    if (*(int16_t *)((uint8_t *)server + 4) == 0 && halo::memory::data_packet_group_decode_packet((length -= 2, (int16_t *)&length), &network_game_messages_group, body, record + 2, &out_type, &version_used, 3) != 0) {
        halo::networking::network_machine_check_build_version(body, machine);
    }
    return 1;
}

/**
 * out/phase4/networking_functions.md: "Handles message type 0x13 by decoding it and
 * forwarding to network_client_connection_handshake_tick" -- network_client_connection_handshake_tick, already written in
 * an earlier batch.
 *
 * @address 0x4e25e0
 */
uint32_t ServerMessageHandlers::handshake_forward(uint8_t *record, int32_t length)
{
    network_server_globals *server = self;
    int32_t body[1];
    int16_t out_type;
    uint16_t version_used;

    if (*(int16_t *)((uint8_t *)server + 4) == 0 && halo::memory::data_packet_group_decode_packet((length -= 2, (int16_t *)&length), &network_game_messages_group, body, record + 2, &out_type, &version_used, 3) != 0) {
        halo::networking::network_client_connection_handshake_tick((int16_t)body[0], server);
    }
    return 1;
}

/**
 * rewrite)
 * address 0x4e2930, size 86 bytes
 * name confidence: 0.35   rewrite confidence: 0.85 (REWRITTEN; was 0.3)
 * out/phase4/networking_functions.md's summary claims message type 0x23, but
 * network_game_process_incoming_message.c's own verified dispatch switch (this batch) shows
 * `case 0x24: network_game_message_handle_join_finalize_ack_role2();` ('$' is 0x24, not 0x23) -- the dispatcher's literal call table
 * is trusted here instead of the low-confidence (0.3) summary. The cleared bit
 *
 * @address 0x4e2930
 */
uint32_t ServerMessageHandlers::join_finalize_ack_role2(network_machine *machine, uint8_t *record, int32_t length)
{
    network_server_globals *server = self;
    uint32_t body[1];
    int16_t out_type;
    uint16_t version_used;

    if (*(int16_t *)((uint8_t *)server + 4) == 2 && halo::memory::data_packet_group_decode_packet((length -= 2, (int16_t *)&length), &network_game_messages_group, body, record + 2, &out_type, &version_used, 7) != 0) {
        *((uint8_t *)machine + 0xe) &= 0xfb;
    }
    return 1;
}

/**
 * Builds a timestamped acknowledgement (echoing `record`'s first dword plus the current
 * millisecond clock) as message type 3 and, when `channel` is not itself in listening mode,
 * queues it for reliable send.
 *
 * @address 0x4e2110
 */
uint32_t ServerMessageHandlers::keepalive(network_channel **channel, int32_t *record)
{
    network_channel *chan;
    large_integer counter;
    struct {
        int32_t echoed_value;
        int32_t timestamp_ms;
    } payload;
    uint16_t *packet;

    if (channel == 0) {
        return 0;
    }
    chan = *channel;
    if (chan == 0) {
        return 0;
    }
    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    payload.echoed_value = *record;
    payload.timestamp_ms = (int32_t)((counter.quad_part * 1000) / halo::cseries::globals().performance_frequency);
    packet = halo::networking::network_prepare_challenge_packet(3, &payload);
    if (packet != 0) {
        uint8_t reliable_flag = 0;
        if ((chan->flags & 1) == 0) {
            halo::networking::network_channel_reliable_pool_store(chan, (uint8_t *)packet, &reliable_flag, 1, 1, (uint32_t)(*(uint16_t *)packet >> 4) << 3);
        }
        return 1;
    }
    return 0;
}

/**
 * If the queued message's first dword is non-zero, skips it via AggregateFieldCodec::decode_compound_field_staged. Otherwise, if
 * AggregateFieldCodec::decode_compound_field reports true, resolves the local player's datum and stores the elapsed time
 * since server+0x9c0 into datum+0xdc.
 *
 * @address 0x4e20b0
 */
uint32_t ServerMessageHandlers::ping_timestamp(int32_t **message)
{
    network_server_globals *server = self;
    uint8_t *player;
    uint8_t decode_scratch[5];

    if (**message != 0) {
        halo::networking::message_delta_decode_compound_field_staged((void **)message);
        return 1;
    }
    if (halo::networking::message_delta_decode_compound_field((void **)message, decode_scratch) == 1) {
        player = (uint8_t *)halo::memory::datum_get((datum_index)decode_scratch[0], halo::game::globals().player_data);
        if (player != 0) {
            int32_t stored_time = *(int32_t *)((uint8_t *)server + 0x9c0);
            int32_t now = halo::cseries::time_query_performance_counter_ms();
            ((struct player *)player)->ping = now - stored_time;
        }
    }
    return 1;
}

/**
 * out/phase4/networking_functions.md: "Handles message type 0x11 by decoding it and
 * triggering a session-wide player-count broadcast" -- network_game_broadcast_player_set_changed
 * (network_game_broadcast_player_set_changed, this batch). Same decode shape as every sibling handler.
 *
 * @address 0x4e2530
 */
uint32_t ServerMessageHandlers::player_count_broadcast(uint8_t *record, int32_t length)
{
    network_server_globals *server = self;
    uint32_t body[1];
    int16_t out_type;
    uint16_t version_used;
    int16_t state = *(int16_t *)((uint8_t *)server + 4);

    if ((state == 0 || state == 1) && halo::memory::data_packet_group_decode_packet((length -= 2, (int16_t *)&length), &network_game_messages_group, body, record + 2, &out_type, &version_used, 3) != 0) {
        halo::networking::network_game_broadcast_player_set_changed((uint8_t *)server);
    }
    return 1;
}

/**
 * out/phase4/networking_functions.md: "Handles message type 0x12, conditionally
 * triggering a player-count broadcast after an approval check" -- the approval check is
 * network_player_entry_update (network_player_entry_update, already written), whose own signature is
 * (network_player_entry *incoming, network_game_session *session); on success this broadcasts
 * the player set change exactly like the type-0x11 handler.
 *
 * @address 0x4e2580
 */
uint32_t ServerMessageHandlers::player_entry_update(uint8_t *record, int32_t length)
{
    network_server_globals *server = self;
    uint32_t body[8];
    int16_t out_type;
    uint16_t version_used;

    if (*(int16_t *)((uint8_t *)server + 4) == 0 && halo::memory::data_packet_group_decode_packet((length -= 2, (int16_t *)&length), &network_game_messages_group, body, record + 2, &out_type, &version_used, 3) != 0 &&
        halo::networking::network_player_entry_update((network_player_entry *)body, (network_game_session *)((uint8_t *)server + 8)) != 0) {
        halo::networking::network_game_broadcast_player_set_changed((uint8_t *)server);
    }
    return 1;
}

/**
 * out/phase4/networking_functions.md's summary claims message type 0x15, but
 * network_game_process_incoming_message.c's own verified dispatch switch (this batch) shows
 * `case 0x14: case 0x25: network_game_message_handle_retry_schedule();` -- the low-confidence (0.3) summary swapped this
 * function's type number with network_game_message_handle_build_version's; the dispatcher's literal call table is trusted
 * here instead. Forwards to network_machine_timer_start, already written (blam-cc: ESI ->
 * machine, stack -> duration_ms).
 *
 * @address 0x4e26a0
 */
uint32_t ServerMessageHandlers::retry_schedule(network_machine *machine, uint8_t *record, int32_t length)
{
    network_server_globals *server = self;
    uint32_t body[1];
    int16_t out_type;
    uint16_t version_used;

    if (*(int16_t *)((uint8_t *)server + 4) == 0 && halo::memory::data_packet_group_decode_packet((length -= 2, (int16_t *)&length), &network_game_messages_group, body, record + 2, &out_type, &version_used, 3) != 0) {
        halo::networking::network_machine_timer_start(machine, 0);
    }
    return 1;
}

/**
 * out/phase4/networking_functions.md: "Server-side handler for message type 0x10
 * that decodes its payload and passes it to network_game_settings_broadcast_send" -- the already-written
 * network_game_settings_broadcast_send. Follows the exact decode-then-forward shape shared by
 * every sibling handler in this cluster (see network_game_process_incoming_message.c).
 *
 * @address 0x4e24d0
 */
uint32_t ServerMessageHandlers::settings_relay(uint8_t *record, int32_t length)
{
    network_server_globals *server = self;
    network_player_entry body;
    int16_t out_type;
    uint16_t version_used;

    if (*(int16_t *)((uint8_t *)server + 4) == 0 && halo::memory::data_packet_group_decode_packet((length -= 2, (int16_t *)&length), &network_game_messages_group, &body, record + 2, &out_type, &version_used, 3) != 0) {
        return halo::networking::network_game_settings_broadcast_send(server, &body);
    }
    return 1;
}

/**
 * out/phase4/networking_functions.md's summary claims message type 0x1e, but
 * network_game_process_incoming_message.c's own verified dispatch switch (this batch) shows
 * `case 0x23: network_game_message_handle_settings_relay_role2();` -- the dispatcher's literal call table is trusted here instead
 * of the low-confidence (0.3) summary. This is the third of three role-gated forwarders to
 * network_game_settings_broadcast_send (roles 0, 1 and 2 at message types 0x10, 0x1d and 0x23
 * respectively), gated on role == 2 and decode class 7.
 *
 * @address 0x4e28d0
 */
uint32_t ServerMessageHandlers::settings_relay_role2(uint8_t *record, int32_t length)
{
    network_server_globals *server = self;
    network_player_entry body;
    int16_t out_type;
    uint16_t version_used;

    if (*(int16_t *)((uint8_t *)server + 4) == 2 && halo::memory::data_packet_group_decode_packet((length -= 2, (int16_t *)&length), &network_game_messages_group, &body, record + 2, &out_type, &version_used, 7) != 0) {
        return halo::networking::network_game_settings_broadcast_send(server, &body);
    }
    return 1;
}

/**
 * out/phase4/networking_functions.md: "Resets the per-round update counters and
 * completion flags on the object at in_EAX, increments its round counter, and calls network_game_session_reset_defaults
 * to continue setup." Same host offsets (+0x9b8, +0x9bc-region, +0x3b0) as
 * network_game_server_host_new.c; see that file's header for the field-matching evidence and the
 * same "+0x3b0 lands in session.unknown_3a2[10]" UNSURE note.
 *
 * @address 0x4df640
 */
void HostServerView::round_reset()
{
    network_server_globals *host = self;
    host->handshake_timer.remaining_ms = 0;
    host->handshake_timer.last_tick_ms = 0;
    host->unknown_9d0 = 0;
    *(uint32_t *)&host->handshake_state = 0;
    host->update_tick = 0;
    host->first_join_ms = 0;
    host->scenario_announced = 0;
    host->new_server_pending = 0;
    host->join_finalize_pending = 0;
    *(int32_t *)((uint8_t *)host + 0x3b0) = *(int32_t *)((uint8_t *)host + 0x3b0) + 1;
    halo::networking::network_game_session_reset_defaults(host);
}

/**
 * out/phase4/networking_functions.md: "Sends the one-time scenario/challenge
 * announcement packets (via message_delta_encode_message/network_session_broadcast_to_all and network_prepare_challenge_packet) the first time it is
 * called for this game, then latches a done flag." host->unknown_9f9/unknown_9b8 match
 * network_game_server_host_new.c's established offsets on network_server_globals.
 *
 * @address 0x4df1c0
 */
int32_t HostServerView::send_scenario_announcement()
{
    network_server_globals *host = self;
    int32_t result;
    void *payload;
    int32_t encode_result;
    int32_t challenge_packet;
    uint32_t challenge_payload[4];

    result = 1;
    if (host->scenario_announced == 0) {
        halo::networking::message_delta_parameters_protocol_send_update();
        payload = (uint8_t *)host + 8;
        encode_result = halo::networking::message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 0x21, 0, &payload, 0, 1, 0);
        if (encode_result > 0) {
            halo::networking::network_session_broadcast_to_all(network_server, 1, &message_delta_definition_table, 1, 0, 1, 3);
        }
        result = encode_result > 0;
        if (encode_result > 0) {

            challenge_packet = (int32_t)halo::networking::network_prepare_challenge_packet(0x0a, challenge_payload);
            if (challenge_packet != 0) {
                if (halo::networking::network_session_broadcast_to_all(network_server, 0, (void *)(uint32_t)challenge_packet, 1, 0, 1, 3) != 0) {
                    host->scenario_announced = 1;
                    result = 1;
                }
            }
        }
    }
    host->update_tick = 0;
    return result;
}

/**
 * out/phase4/networking_functions.md: "If a network host is active and not in the
 * special team-sync case, resets the host's map-load flag and history state and disposes the
 * host globals; otherwise defers to network_host_update_tick." session->unknown_3ac (the map-loaded flag)
 * matches types/networking.h's network_game_session exactly when reached through
 * network_server->session or network_client->session directly (unlike
 * network_game_server_host_create.c's host-relative offset, which lands elsewhere -- see that
 * file's UNSURE note).
 *
 * @address 0x4ddd90
 */
int32_t HostServerView::shutdown_or_defer()
{
    network_game_session *session;

    if (network_server != 0) {
        if (network_host_handoff_requested != 1 || ((network_server->flags >> 2) & 1) == 0) {

            return halo::networking::network_host_update_tick(network_server);
        }
        network_game_mode = 0;
        halo::main::main_menu_music_stop();
        if (network_server != 0) {
            session = &network_server->session;
        } else if (network_client != 0) {
            session = &network_client->session;
        } else {
            session = 0;
        }
        if (session->map_loaded != 0) {
            halo::main::chimera__load_ui_map(1);
        }
        session->map_loaded = 0;
        halo::networking::network_client_globals_dispose();
        if (network_server != 0) {
            halo::networking::network_game_server_host_dispose(network_server);
            network_server = 0;
            network_server_host_valid = 0;
        }
        *(uint16_t *)&split_screen_quit_prompt_string = 0xffff;
        network_join_error_reason = 0;
        *((uint8_t *)&split_screen_quit_prompt_string + 3) = 1;
    }
    return 1;
}

/**
 * out/phase4/networking_functions.md: "Per-tick client-side network update handler:
 * pulls the next queued packet, refreshes the map/variant cycle list periodically, and
 * dispatches processing to one of three state-specific handlers based on [state]." host->flags
 * bit1 (k_network_server_host, matches network_game_server_host_new.c) and host->unknown_004
 * (the state dispatched on 0/1/2, matching network_game_server_host_dispose.c's own 0/2 test)
 * match types/networking.h.
 *
 * @address 0x4def80
 */
char HostServerView::update_tick()
{
    network_server_globals *host = self;
    char service_result;
    char proceed;
    network_channel *new_child;
    network_resolved_address sender;
    uint32_t now_ms;

    service_result = 1;
    if ((host->flags >> 1) & 1) {
        new_child = 0;
        service_result = halo::networking::network_channel_service(host->listen_channel, 0, &new_child);
        if (service_result == 1) {
            proceed = 1;
            if (new_child != 0) {
                service_result = halo::networking::network_server_count_machines_and_resolve_address(host, new_child);
                if (service_result == 1) {
                    halo::networking::network_channel_remote_address_or_default(new_child, &sender);
                    proceed = 1;
                } else {
                    proceed = (char)halo::networking::network_channel_remove_child(host->listen_channel, new_child);
                }
            }
            service_result = 0;
            if (proceed != 0) {
                now_ms = halo::cseries::time_query_performance_counter_ms();
                if ((uint32_t)((int32_t)host->last_stamp_ms + 3000) < now_ms) {
                    halo::networking::network_map_cycle_list_broadcast();
                    host->last_stamp_ms = now_ms;
                }
                service_result = halo::networking::network_server_service_machines_tick(host);
                if (service_result == 0) {
                    return 0;
                }
                if (host->state == 0) {
                    return halo::networking::network_server_heartbeat_tick(host);
                }
                if (host->state != 1) {
                    if (host->state != 2) {
                        return 0;
                    }
                    return halo::networking::network_server_resend_challenge_periodic(host);
                }
                return halo::networking::network_server_status_periodic_print(host);
            }
        }
    }
    return service_result;
}

/**
 * EAX machine, stack response: the CD key check of a joining
 * machine with its remote ip (the local address 0x006869b0 for loopback 127.0.0.1), its challenge (+0x52) and CD
 * key local id (+0x5c). (Name kept.)
 *
 * @address 0x4e0ab0
 */
uint8_t MachineView::reset_state(const char *response)
{
    network_machine *machine = self;
    network_resolved_address address;
    uint32_t ip;

    halo::networking::network_channel_remote_address_or_default(machine != 0 ? machine->channel : 0, &address);
    ip = *(uint32_t *)&address;
    if (ip == 0x7f000001) {
        ip = network_local_address;
    }
    return halo::networking::network_session_host_reject_or_cleanup_client(response, (const char *)machine + 0x52, ip,
        *(int32_t *)((uint8_t *)machine + 0x5c));
}

/**
 * Sets k_network_machine_version_mismatch on `machine` when `remote_version` matches this
 * build's version string exactly (see the polarity UNSURE note above).
 *
 * @address 0x4dff20
 */
void MachineView::check_build_version(const char *remote_version)
{
    network_machine *machine = self;
    const uint8_t *local;
    const uint8_t *remote;
    int32_t equal;

    local = (const uint8_t *)network_build_string;
    remote = (const uint8_t *)remote_version;
    equal = 0;
    while (*local == *remote) {
        if (*local == 0) {
            equal = 1;
            break;
        }
        local = local + 1;
        remote = remote + 1;
    }
    if (equal) {
        machine->flags |= k_network_machine_version_mismatch;
    }
}

/**
 * types/networking.h cites this address directly: "network_machine (0x4dec40 init,
 * 0x4df690 reset, ...)". flags |= k_network_machine_pending, timer_14/timer_18/unknown_50, and
 * the 0xd-dword (0x34-byte) zero of connect_state[0x34] at +0x1c all match exactly.
 * The leading network_channel_remote_address_or_default call takes EAX = machine->channel and
 * ECX = &local scratch; its result is unused (verified 0x4df6a8..0x4df6b9).
 *
 * @address 0x4df690
 */
int32_t MachineView::reset()
{
    network_machine *machine = self;
    network_resolved_address sender;

    halo::networking::network_channel_remote_address_or_default(machine->channel, &sender);
    machine->flags = machine->flags | k_network_machine_pending;
    machine->disconnect_timer_active = 0;
    machine->timer_14 = 0;
    machine->timer_18 = 0;
    machine->player_joined = 0;
    memset(&machine->last_update, 0, sizeof(machine->last_update));
    return 1;
}

/**
 * out/phase4/networking_functions.md: "Starts a timer on the object at unaff_ESI:
 * records the current time, marks it active, and computes its expiry as now plus the given
 * duration." Fields +0x10/+0x14/+0x18 match network_machine's unknown_10/timer_14/timer_18
 * exactly (same object network_machine_reset.c clears).
 *
 * @address 0x4df090
 */
void MachineView::timer_start(int32_t duration_ms)
{
    network_machine *machine = self;
    large_integer counter;
    int32_t now_ms;

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    now_ms = (int32_t)((counter.quad_part * 1000) / halo::cseries::globals().performance_frequency);
    machine->timer_14 = now_ms;
    machine->disconnect_timer_active = 1;
    machine->timer_18 = now_ms + duration_ms;
}

}
