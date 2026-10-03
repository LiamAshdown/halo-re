/**
 * @file include/halo/objects/object_block_storage.hpp
 * Object system API: object block storage.
 * The C symbols other modules link against are the wrappers in src/objects/objects_c_api.cpp.
 */
#pragma once

#include "halo/objects/engine_types.hpp"

namespace halo::objects {

/**
 * The memory_pool backed variable-length object record: allocation, release and growth of the node and function
 * blocks behind an object.
 */
class ObjectBlockStorage {
public:
    /**
     * Allocates a datum in the array with a variable-length block of the given size.
     *
     * @address 0x004f7d50
     */
    static datum_index create(int32_t specific_index, data_array *array, int16_t size);

    /**
     * Releases the variable-length block of an object datum back to the pool.
     *
     * @address 0x004f7de0
     */
    static void release(data_array *array, datum_index handle);

    /**
     * Grows a block field of an object record by extra_size bytes; returns whether it succeeded.
     *
     * @address 0x004f7e50
     */
    static uint8_t grow(uint32_t object_index, int16_t field_offset, int16_t extra_size);
};

}  // namespace halo::objects
