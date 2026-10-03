#pragma once

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "scenario.h"
#include "objects.h"
#include "projectiles.h"
#include "game.h"
#include "sound.h"
#include "physics.h"
#include "units.h"
#include <string.h>

namespace halo::projectiles {
namespace {

/**
 * View of one projectile object addressed by its object handle. The projectile object stays in
 * the object data array; this class carries the handle and groups the simulation operations
 * (creation, per-tick update, collision response, detonation) that the projectile object type
 * row calls.
 */
class ProjectileHandle {
public:
    explicit ProjectileHandle(datum_index value) : handle(value) {}
    uint8_t construct();
    int update();
    void update_function_values();
    void compute_deceleration();
    void compute_rotation();
    uint8_t collision_test(real_point3d *target, void *out_record);
    void response(collision_result *hit, real_point3d *out_position, real_vector3d *velocity);
    void detonate(char first_collision, real remaining_tick_fraction);
    uint8_t force_detonate();
    uint8_t is_old_enough();
    void notify_object_deleted(datum_index dying_object_index);
    static real deceleration_from_range(Projectile *tag, real r0, real r1);

    datum_index handle;
};

/**
 * Constant-result stubs shared by several object type definition rows for query-style table
 * columns.
 */
class ObjectTypeStubs {
public:
    static uint8_t return_false();
    static uint8_t return_true();
};

}
}
