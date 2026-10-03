/**
 * @file include/halo/units/records.hpp
 * Typed views of the type-specific extensions that follow the common object header in a unit, biped or vehicle record.
 */
#pragma once

#include <cstdint>
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

namespace halo::units {

/** The unit extension of an object record that belongs to a unit, biped or vehicle (at object + 0x1f4). */
inline unit_data *unit_data_of(void *object_record)
{
    return reinterpret_cast<unit_data *>(static_cast<uint8_t *>(object_record) + k_unit_data_offset);
}

/** The biped extension of a biped object record (at object + 0x4cc). */
inline biped_data *biped_data_of(void *object_record)
{
    return reinterpret_cast<biped_data *>(static_cast<uint8_t *>(object_record) + k_unit_object_size);
}

/** The vehicle extension of a vehicle object record (at object + 0x4cc). */
inline vehicle_data *vehicle_data_of(void *object_record)
{
    return reinterpret_cast<vehicle_data *>(static_cast<uint8_t *>(object_record) + k_unit_object_size);
}

}  // namespace halo::units
