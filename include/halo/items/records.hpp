/**
 * @file include/halo/items/records.hpp
 * Typed access to the type-specific blocks of item, weapon and equipment object records.
 */
#pragma once

#include "items.h"

namespace halo::items {

/** The item_data block of an item (weapon, equipment, garbage) object record. */
inline item_data *item_data_of(const void *object_record) noexcept
{
    return const_cast<item_data *>(&static_cast<const item_object *>(object_record)->item);
}

/** The weapon_data block of a weapon object record. */
inline weapon_data *weapon_data_of(const void *object_record) noexcept
{
    return const_cast<weapon_data *>(&static_cast<const weapon_object *>(object_record)->weapon);
}

/** The equipment_data block of an equipment object record. */
inline equipment_data *equipment_data_of(const void *object_record) noexcept
{
    return const_cast<equipment_data *>(&static_cast<const equipment_object *>(object_record)->equipment);
}

}  // namespace halo::items
