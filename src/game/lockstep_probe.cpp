/**
 * @file src/game/lockstep_probe.cpp
 * Determinism probe for lockstep co-op (-lsprobe [jitter]). It loads b30, replaces the local player's per-tick action
 * with a scripted one, and prints:
 *   LSP tick hash objects      every 30 ticks, a hash of the game RNG and every object's position and velocity
 *   LSP-LEAK tick where        the simulation state changed outside a tick (probe_checkpoint)
 *   LSPMAP offset kind name    once, the data arrays and pools carved out of game state
 *   LSPC tick hash...          ticks 1200-1500, the whole game-state blob hashed in 16 KB chunks
 * Two runs that print different hashes for the same tick have diverged; "jitter" stalls random frames so the runs
 * differ in how many ticks each frame advances.
 */

#include "halo/game/api.hpp"
#include "halo/game/lockstep.hpp"
#include "halo/math/globals.hpp"
#include "halo/objects/vars.hpp"
#include "halo/objects/record_access.hpp"
#include "halo/saved_games/api.hpp"
#include "halo/shell/api.hpp"
#include "halo/core/link.hpp"
#include "halo/platform/time.hpp"
#include "halo/main/api.hpp"
#include "halo/ai/api.hpp"
#include "halo/hs/api.hpp"
#include "halo/game/records.hpp"
#include "halo/memory/api.hpp"
#include "memory.h"
#include "interface.h"
#include "main.h"

#include "halo/core/libm.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static auto &object_data = halo::link::ref<data_array *>(halo::objects::vars().object_data);

namespace halo::game::lockstep {

namespace {

constexpr int32_t k_chunk_size = 0x4000;

int g_mode = -1;  // -1 unread, 0 off, 1 probe, 2 probe with frame jitter
uint32_t g_jitter_seed = 12345;
uint32_t g_last_hash;
bool g_last_valid;
int g_leaks;

int mode()
{
    if (g_mode < 0) {
        const char *value = nullptr;

        g_mode = halo::shell::command_line_check_flag("-lsprobe", &value) ? 1 : 0;
        if (g_mode != 0 && value != nullptr && strcmp(value, "jitter") == 0) {
            g_mode = 2;
        }
    }
    return g_mode;
}

uint32_t fnv(uint32_t hash, const void *bytes, size_t size)
{
    const uint8_t *p = static_cast<const uint8_t *>(bytes);

    for (size_t i = 0; i < size; i++) {
        hash = (hash ^ p[i]) * 16777619u;
    }
    return hash;
}

/** The simulation state: the game RNG and every object's identity, position and velocity. */
uint32_t state_hash_impl(int32_t *object_count)
{
    uint32_t hash = 2166136261u;
    int32_t objects = 0;
    random_seed seed = halo::math::globals().random_seed_global;

    hash = fnv(hash, &seed, sizeof(seed));
    for (int32_t i = 0; i < object_data->last_index; i++) {
        object_header *header = &static_cast<object_header *>(object_data->data)[i];

        if (header->identifier == 0 || header->data == nullptr) {
            continue;
        }
        object *obj = header->data;

        objects++;
        hash = fnv(hash, &i, sizeof(i));
        hash = fnv(hash, &header->identifier, sizeof(header->identifier));
        hash = fnv(hash, &obj->definition_tag, sizeof(obj->definition_tag));
        hash = fnv(hash, &obj->position, sizeof(obj->position));
        hash = fnv(hash, &obj->velocity, sizeof(obj->velocity));
    }
    if (object_count != nullptr) {
        *object_count = objects;
    }
    return hash;
}

/**
 * Every byte of every object block except the per-frame fields: the gather stamp (0x14) and the effect, light and
 * render-state handles (0x14c..0x173).
 */
uint32_t full_object_hash()
{
    uint32_t hash = 2166136261u;

    for (int32_t i = 0; i < object_data->last_index; i++) {
        object_header *header = &static_cast<object_header *>(object_data->data)[i];

        if (header->identifier == 0 || header->data == nullptr) {
            continue;
        }
        uint8_t *bytes = reinterpret_cast<uint8_t *>(header->data);

        hash = fnv(hash, bytes, 0x14);
        hash = fnv(hash, bytes + 0x18, 0x14c - 0x18);
        hash = fnv(hash, bytes + 0x174, header->block_size - 0x174);
    }
    return hash;
}

uint32_t g_last_full_hash;

/** A copy of every object block at the end of the last tick, to name what changed outside the tick. */
struct object_snapshot {
    int32_t index;
    int32_t size;
    int32_t offset;
};
object_snapshot g_snapshots[0x1000];
int32_t g_snapshot_count;
uint8_t g_snapshot_bytes[0x400000];

void snapshot_objects()
{
    int32_t used = 0;

    g_snapshot_count = 0;
    for (int32_t i = 0; i < object_data->last_index && g_snapshot_count < 0x1000; i++) {
        object_header *header = &static_cast<object_header *>(object_data->data)[i];

        if (header->identifier == 0 || header->data == nullptr) {
            continue;
        }
        if (used + header->block_size > static_cast<int32_t>(sizeof(g_snapshot_bytes))) {
            break;
        }
        memcpy(g_snapshot_bytes + used, header->data, header->block_size);
        g_snapshots[g_snapshot_count++] = {i, header->block_size, used};
        used += header->block_size;
    }
}

void print_object_changes(const char *where)
{
    int printed = 0;

    for (int32_t s = 0; s < g_snapshot_count && printed < 12; s++) {
        object_header *header = &static_cast<object_header *>(object_data->data)[g_snapshots[s].index];

        if (header->identifier == 0 || header->data == nullptr) {
            continue;
        }
        const uint8_t *before = g_snapshot_bytes + g_snapshots[s].offset;
        const uint8_t *after = reinterpret_cast<const uint8_t *>(header->data);
        int32_t size = header->block_size < g_snapshots[s].size ? header->block_size : g_snapshots[s].size;

        for (int32_t offset = 0; offset + 4 <= size && printed < 12; offset += 4) {
            if (offset == 0x14 || (offset >= 0x14c && offset < 0x174)) {
                continue;
            }
            uint32_t old_word;
            uint32_t new_word;

            memcpy(&old_word, before + offset, 4);
            memcpy(&new_word, after + offset, 4);
            if (old_word != new_word) {
                printf("LSP-DIFF %d %s object=%d type=%d tag=%08x off=%x %08x -> %08x\n",
                    halo::game::globals().game_time->game_time, where, g_snapshots[s].index, header->type,
                    header->data->definition_tag, offset, old_word, new_word);
                printed++;
            }
        }
    }
}

struct region {
    int32_t offset;
    int32_t size;
    const char *name;
};
region g_regions[256];
int32_t g_region_count;

/** Per named game-state block hashes ("LSPA tick index:name=hash ..."), every 30 ticks. */
void print_region_hashes(int32_t tick)
{
    static char line[0x8000];
    uint8_t *base = halo::saved_games::globals().game_state_base;
    int32_t length = snprintf(line, sizeof(line), "LSPA %d", tick);

    for (int32_t i = 0; i < g_region_count && length < static_cast<int32_t>(sizeof(line)) - 64; i++) {
        length += snprintf(line + length, sizeof(line) - length, " %d:%.20s=%08x", i, g_regions[i].name,
            fnv(2166136261u, base + g_regions[i].offset + 0x38, g_regions[i].size));
    }
    printf("%s\n", line);
}

/**
 * Which fields differ: for each 4-byte word of an element, a hash of that word over every live element
 * ("LSPF tick name off=hash ..."; only words that are not zero everywhere).
 */
void print_columns(int32_t tick, const char *name, uint8_t *const *elements, int32_t count, int32_t size)
{
    static char line[0x10000];
    int32_t length = snprintf(line, sizeof(line), "LSPF %d %s", tick, name);

    for (int32_t word = 0; word + 4 <= size && length < static_cast<int32_t>(sizeof(line)) - 32; word += 4) {
        uint32_t hash = 2166136261u;
        bool any = false;

        for (int32_t i = 0; i < count; i++) {
            uint32_t value;

            memcpy(&value, elements[i] + word, 4);
            any = any || value != 0;
            hash = fnv(hash, &value, 4);
        }
        if (any) {
            length += snprintf(line + length, sizeof(line) - length, " %x=%08x", word, hash);
        }
    }
    printf("%s\n", line);
}

void print_field_columns(int32_t tick)
{
    static uint8_t *elements[0x1000];
    int32_t count = 0;

    for (int32_t i = 0; i < object_data->last_index && count < 0x1000; i++) {
        object_header *header = &static_cast<object_header *>(object_data->data)[i];

        if (header->identifier != 0 && header->data != nullptr) {
            elements[count++] = reinterpret_cast<uint8_t *>(header->data);
        }
    }
    print_columns(tick, "object", elements, count, 0x1f4);

    for (int32_t r = 0; r < g_region_count; r++) {
        const char *name = g_regions[r].name;

        if (strcmp(name, "prop") != 0 && strcmp(name, "actor") != 0 && strcmp(name, "players") != 0) {
            continue;
        }
        data_array *array = reinterpret_cast<data_array *>(halo::saved_games::globals().game_state_base + g_regions[r].offset);

        count = 0;
        for (int32_t i = 0; i < array->last_index && count < 0x1000; i++) {
            uint8_t *element = static_cast<uint8_t *>(array->data) + i * array->size;

            if (*reinterpret_cast<int16_t *>(element) != 0) {
                elements[count++] = element;
            }
        }
        print_columns(tick, name, elements, count, array->size);
        if (strcmp(name, "prop") == 0 && tick == 360) {
            for (int32_t i = 0; i < array->last_index; i++) {
                uint8_t *element = static_cast<uint8_t *>(array->data) + i * array->size;
                uint32_t words[4];

                if (*reinterpret_cast<int16_t *>(element) == 0) {
                    continue;
                }
                memcpy(words, element + 0xf8, sizeof(words));
                printf("LSPP %d %d %08x %08x %08x %08x %08x\n", tick, i, *reinterpret_cast<uint32_t *>(element + 0x18),
                    words[0], words[1], words[2], words[3]);
            }
        }
    }
}

void print_blob_map()
{
    uint8_t *base = halo::saved_games::globals().game_state_base;
    int32_t size = halo::saved_games::globals().game_state_cursor;

    for (int32_t offset = 0; offset + 0x38 <= size; offset += 4) {
        uint8_t *p = base + offset;
        uint8_t *data;

        memcpy(&data, p + 0x34, sizeof(data));
        if (data == p + 0x38 && p[0] >= 0x20 && p[0] < 0x7f) {
            data_array *array = reinterpret_cast<data_array *>(p);

            printf("LSPMAP %x array %.31s\n", offset, reinterpret_cast<char *>(p));
            if (g_region_count < 256) {
                g_regions[g_region_count++] = {offset, array->maximum_count * array->size, array->name};
            }
            continue;
        }
        memcpy(&data, p + 0x24, sizeof(data));
        if (data == p + 0x38 && p[4] >= 0x20 && p[4] < 0x7f) {
            memory_pool *pool = reinterpret_cast<memory_pool *>(p);

            printf("LSPMAP %x pool %.31s\n", offset, reinterpret_cast<char *>(p + 4));
            if (g_region_count < 256) {
                g_regions[g_region_count++] = {offset, pool->size, pool->name};
            }
        }
    }
    printf("LSPMAP %x end\n", size);
}

void print_blob_chunks(int32_t tick)
{
    static char line[0x4000];
    uint8_t *base = halo::saved_games::globals().game_state_base;
    int32_t size = halo::saved_games::globals().game_state_cursor;
    int32_t length = snprintf(line, sizeof(line), "LSPC %d", tick);

    for (int32_t offset = 0; offset < size && length < static_cast<int32_t>(sizeof(line)) - 16; offset += k_chunk_size) {
        int32_t chunk = size - offset < k_chunk_size ? size - offset : k_chunk_size;

        length += snprintf(line + length, sizeof(line) - length, " %08x", fnv(2166136261u, base + offset, chunk));
    }
    printf("%s\n", line);
}

}  // namespace

bool probe_active()
{
    return mode() != 0;
}

void probe_frame_begin()
{
    static int frames;

    if (mode() == 0) {
        return;
    }
    if (++frames == 60 && !halo::shell::command_line_check_flag("-coop", nullptr)) {
        // a fresh start on every machine: never the browser profile's saved checkpoint
        halo::main::main_queue_map_change("levels\\b30\\b30");
        halo::main::globals().main_globals.restore_checkpoint_on_load = 0;
    }
    if (mode() != 2) {
        return;
    }
    // a private LCG: the game's own streams must not see this
    g_jitter_seed = g_jitter_seed * 1103515245u + 12345u;
    uint32_t stall_ms = (g_jitter_seed >> 16) % 60;
    uint32_t start = halo::platform::tick_milliseconds();

    while (halo::platform::tick_milliseconds() - start < stall_ms) {
    }
}

void probe_override_actions(player_action *actions)
{
    if (mode() == 0) {
        return;
    }
    probe_scripted_action(halo::game::globals().game_time->game_time, &actions[0]);
}

void probe_scripted_action(int32_t tick, player_action *action)
{
    int32_t phase = tick % 300;

    memset(action, 0, sizeof(*action));
    action->desired_yaw = static_cast<float>(halo::libm::fmod(tick * 0.01, 6.2831855));
    action->desired_pitch = 0.2f * halo::libm::sinf(static_cast<float>(tick) * 0.02f);
    action->throttle_x = phase < 150 ? 1.0f : -0.5f;
    action->throttle_y = phase < 75 ? 0.5f : 0.0f;
    action->primary_trigger = (tick % 90) < 15 ? 1.0f : 0.0f;
    action->weapon_index = -1;
    action->grenade_index = -1;
    action->zoom_level = -1;
}

void probe_checkpoint(const char *where)
{
    // in a co-op session the machines' hash exchange stands in for this, at a fraction of the cost
    if (mode() == 0 || session_active() || object_data == nullptr || object_data->data == nullptr) {
        return;
    }
    uint32_t hash = state_hash_impl(nullptr);
    uint32_t full = full_object_hash();

    if (g_last_valid && (hash != g_last_hash || full != g_last_full_hash) && g_leaks < 300) {
        g_leaks++;
        printf("LSP-LEAK %d %s seed=%08x %s\n", halo::game::globals().game_time->game_time, where,
            static_cast<uint32_t>(halo::math::globals().random_seed_global), hash != g_last_hash ? "state" : "object-bytes");
        if (full != g_last_full_hash && g_leaks < 20) {
            print_object_changes(where);
        }
    }
    if (full != g_last_full_hash) {
        snapshot_objects();
    }
    g_last_hash = hash;
    g_last_full_hash = full;
    g_last_valid = true;
}

void probe_tick_end()
{
    if (mode() == 0) {
        return;
    }
    int32_t tick = halo::game::globals().game_time->game_time;
    int32_t objects;
    uint32_t hash = state_hash_impl(&objects);

    g_last_hash = hash;
    if (!session_active()) {
        g_last_full_hash = full_object_hash();
        g_last_valid = true;
        snapshot_objects();
    }
    if (tick % 30 == 0) {
        printf("LSP %d %08x %d\n", tick, hash, objects);
    }
    if (tick == 60 && g_region_count == 0) {
        print_blob_map();
    }
    if (tick > 60 && tick <= 1500 && tick % 30 == 0) {
        print_region_hashes(tick);
    }
    if (tick >= 1200 && tick <= 1300) {
        uint32_t objects_only = 2166136261u;

        for (int32_t i = 0; i < object_data->last_index; i++) {
            object_header *header = &static_cast<object_header *>(object_data->data)[i];

            if (header->identifier != 0 && header->data != nullptr) {
                objects_only = fnv(objects_only, &header->data->position, sizeof(real_point3d) * 2);
            }
        }
        printf("LSPT %d seed=%08x objects=%08x\n", tick, static_cast<uint32_t>(halo::math::globals().random_seed_global),
            objects_only);
    }
    static int32_t dump_tick = -2;
    if (dump_tick == -2) {
        const char *value = nullptr;

        dump_tick = halo::shell::command_line_check_flag("-lsdump", &value) && value != nullptr ? atoi(value) : -1;
    }
    if (dump_tick >= 0 && ((tick >= 1260 && tick <= 1500 && tick % 30 == 0) || tick == dump_tick)) {
        for (int32_t i = 0; i < object_data->last_index; i++) {
            object_header *header = &static_cast<object_header *>(object_data->data)[i];

            if (header->identifier == 0 || header->data == nullptr) {
                continue;
            }
            object *obj = header->data;

            uint32_t bits[6];

            memcpy(bits, &obj->position, sizeof(bits));
            printf("LSPO %d %d %04x %08x %d %08x %08x %08x %08x %08x %08x\n", tick, i, static_cast<uint16_t>(header->identifier),
                obj->definition_tag, header->type, bits[0], bits[1], bits[2], bits[3], bits[4], bits[5]);
        }
    }
}

uint32_t simulation_hash(int32_t *object_count)
{
    return state_hash_impl(object_count);
}

namespace {

/** Every live element of a game-state data array, with its index. */
uint32_t data_array_hash(const data_array *array)
{
    uint32_t hash = 2166136261u;

    if (array == nullptr || array->data == nullptr) {
        return hash;
    }
    for (int32_t i = 0; i < array->last_index; i++) {
        const uint8_t *element = static_cast<const uint8_t *>(array->data) + i * array->size;

        if (*reinterpret_cast<const int16_t *>(element) != 0) {
            hash = fnv(hash, &i, sizeof(i));
            hash = fnv(hash, element, array->size);
        }
    }
    return hash;
}

const char *const k_region_names[k_region_hash_count] = {
    "actor", "prop", "encounter", "ai pursuit", "swarm", "swarm component", "hs thread", "hs globals", "players"};

}  // namespace

const char *simulation_region_name(int32_t index)
{
    return index >= 0 && index < k_region_hash_count ? k_region_names[index] : "?";
}

void simulation_region_hashes(uint32_t *out)
{
    const halo::ai::Globals &ai = halo::ai::globals();
    data_iterator iterator;
    player *entry;
    uint32_t players = 2166136261u;

    out[0] = data_array_hash(ai.actor_data);
    out[1] = data_array_hash(ai.prop_data);
    out[2] = data_array_hash(ai.encounter_data);
    out[3] = data_array_hash(ai.pursuit_data);
    out[4] = data_array_hash(ai.swarm_data);
    out[5] = data_array_hash(ai.swarm_component_data);
    out[6] = data_array_hash(halo::hs::globals().thread_data);
    out[7] = data_array_hash(halo::hs::globals().globals_data);
    // players by what the game does with them: each machine marks a different one local
    iterator.data = halo::game::globals().player_data;
    iterator.next_index = 0;
    iterator.index = k_datum_index_none;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
    while ((entry = static_cast<player *>(halo::memory::data_iterator_next(&iterator))) != 0) {
        players = fnv(players, &iterator.index, sizeof(iterator.index));
        players = fnv(players, &entry->unit, sizeof(entry->unit));
        players = fnv(players, &entry->deaths, sizeof(entry->deaths));
        players = fnv(players, &entry->team, sizeof(entry->team));
    }
    out[8] = players;
}

}  // namespace halo::game::lockstep
