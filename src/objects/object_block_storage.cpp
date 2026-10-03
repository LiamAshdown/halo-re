#include "halo/objects/object_block_storage.hpp"
#include <string.h>
#include "halo/memory/api.hpp"

extern "C" {
extern data_array *object_data;
extern memory_pool *object_memory_pool;
}

/**
 * Allocates a datum in the array with a variable-length block of the given size.
 *
 * @address 0x004f7d50
 */
datum_index halo::objects::ObjectBlockStorage::create(int32_t specific_index, data_array *array, int16_t size)
{
    datum_index handle = (specific_index == -1) ? halo::memory::datum_new(array) : halo::memory::datum_new_at_index_with_salt(specific_index, array);

    if (handle != k_datum_index_none) {
        object_header *header = (object_header *)array->data + (handle & 0xffff);

        if (halo::memory::block_list_allocate(object_memory_pool, size, (void **)&header->data) != 0) {
            uint8_t *data = (uint8_t *)header->data;
            int16_t i;
            header->block_size = size;
            for (i = 0; i < size; i++) {
                data[i] = 0;
            }
            return handle;
        }

        halo::memory::datum_delete(array, handle);
        handle = k_datum_index_none;
    }

    return handle;
}

/**
 * Releases the variable-length block of an object datum back to the pool.
 *
 * @address 0x004f7de0
 */
void halo::objects::ObjectBlockStorage::release(data_array *array, datum_index handle)
{
    object_header *header = (object_header *)array->data + (handle & 0xffff);
    uint8_t *payload = (uint8_t *)header->data;

    if (payload != 0) {
        memory_pool_block *block = (memory_pool_block *)(payload - 0x18);

        object_memory_pool->free_bytes += block->size;

        if (block->previous == 0) {
            object_memory_pool->first_block = block->next;
        } else {
            block->previous->next = block->next;
        }

        if (block->next == 0) {
            object_memory_pool->last_block = block->previous;
        } else {
            block->next->previous = block->previous;
        }
    }

    halo::memory::datum_delete(array, handle);
    header->data = 0;
    header->flags = 0;
}

/**
 * Grows a block field of an object record by extra_size bytes; returns whether it succeeded.
 *
 * @address 0x004f7e50
 */
uint8_t halo::objects::ObjectBlockStorage::grow(uint32_t object_index, int16_t field_offset, int16_t extra_size)
{
    object_header *header = (object_header *)object_data->data + (object_index & 0xffff);
    int32_t extra = (int32_t)extra_size;
    uint16_t old_size;
    uint8_t *data;
    object_block_reference *field;

    if ((uint8_t)halo::memory::block_list_reallocate((void **)&header->data, (int32_t)header->block_size + extra,
            object_memory_pool) == 0) {
        return 0;
    }
    old_size = (uint16_t)header->block_size;
    header->block_size = (int16_t)(old_size + extra_size);
    header = (object_header *)object_data->data + (object_index & 0xffff);
    data = (uint8_t *)header->data;
    field = (object_block_reference *)(data + field_offset);
    field->offset = (int16_t)old_size;
    field->size = extra_size;
    memset(data + (int16_t)old_size, 0, (size_t)extra);
    return 1;
}
