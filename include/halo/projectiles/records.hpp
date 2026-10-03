/**
 * @file include/halo/projectiles/records.hpp
 * Typed view of the projectile extension that follows the common object header in a projectile record.
 */
#pragma once

#include <cstddef>
#include "projectiles.h"

namespace halo::projectiles {

/** The projectile_data block of a projectile object record. */
inline projectile_data *projectile_data_of(void *object_record) noexcept
{
    return &static_cast<projectile_object *>(object_record)->projectile;
}

static_assert(offsetof(projectile_object, projectile) == k_projectile_data_offset);

}  // namespace halo::projectiles
