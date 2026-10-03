#pragma once

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "projectiles.h"
#include "units.h"
#include "game.h"
#include "networking.h"
#include "cache.h"

namespace halo::projectiles {
namespace {

/**
 * Network replication of one projectile addressed by its object handle: baselines, creation,
 * attach and detonation messages and state updates, plus the static receivers that decode
 * incoming records.
 */
class ProjectileNetwork {
public:
    explicit ProjectileNetwork(datum_index value) : handle(value) {}
    void apply_update(uint32_t *update_record);
    int32_t build_update(uint32_t unused_arg2, uint32_t unused_arg3, int32_t update_type);
    void baseline_take();
    void request_state(int16_t requested_state);
    void send_attach(datum_index parent_object_index, int16_t marker_index);
    int32_t send_creation();
    void send_detonation();
    static void attach_apply(void *incoming_record);
    static void create_from_network(void *incoming_record);
    static void detonation_message_apply(void *incoming_record);

    datum_index handle;
};

}
}
