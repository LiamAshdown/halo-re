#include "halo/hs/hs3_objects.hpp"
#include "crt.h"
#include "halo/memory/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/saved_games/api.hpp"

extern "C" {
extern data_array *object_list_header_data;
extern data_array *object_list_reference_data;
extern void object_list_reference_chain_delete(data_array *reference_array, datum_index chain_head);
}

namespace halo::hs::part3 {

/**
 * Returns the object_index of the first element in the list headed by `header_index` (or k_datum_index_none if
 * the list is empty or invalid), and writes the iterator state to continue from there into `*iterator_out`.
 *
 * @address 0x48b2f0
 */
int32_t ObjectLists::get_first(datum_index header_index, object_list_iterator *iterator_out) const
{
    object_list_header *header;
    datum_index first;
    object_list_reference *node;

    if (header_index != k_datum_index_none) {
        header = (object_list_header *)((uint8_t *)object_list_header_data->data +
            (header_index & halo::k_slot_mask) * 0x0c);
        first = header->first_reference;
        *iterator_out = first;
        if (first != k_datum_index_none) {
            node = (object_list_reference *)((uint8_t *)object_list_reference_data->data +
                (first & halo::k_slot_mask) * 0x0c);
            *iterator_out = node->next;
            return node->object_index;
        }
    }
    return -1;
}

/**
 * Walks `n` nodes into the reference list headed by `header_index` and returns that node's object_index, or
 * k_datum_index_none if the list is shorter than `n + 1` entries (or `header_index` itself is
 * k_datum_index_none).
 *
 * @address 0x488570
 */
int32_t ObjectLists::nth_reference(datum_index header_index, int16_t n) const
{
    object_list_header *header;
    object_list_reference *reference;
    datum_index next;
    int32_t object_index;
    int16_t remaining;

    object_index = -1;
    next = halo::k_dword_none;
    if (header_index != k_datum_index_none) {
        header = (object_list_header *)((uint8_t *)object_list_header_data->data +
            (header_index & halo::k_slot_mask) * 0x0c);
        next = header->first_reference;
        if (next == k_datum_index_none) {
            object_index = -1;
            next = halo::k_dword_none;
        } else {
            reference = (object_list_reference *)((uint8_t *)object_list_reference_data->data +
                (next & halo::k_slot_mask) * 0x0c);
            object_index = reference->object_index;
            next = reference->next;
        }
    }

    remaining = n;
    while (0 < remaining && object_index != -1) {
        if (next == halo::k_dword_none) {
            object_index = -1;
            next = halo::k_dword_none;
        } else {
            reference = (object_list_reference *)((uint8_t *)object_list_reference_data->data +
                (next & halo::k_slot_mask) * 0x0c);
            object_index = reference->object_index;
            next = reference->next;
        }
        remaining = remaining - 1;
    }
    return object_index;
}

/**
 * Pushes `object_index` onto the front of the reference list headed by `header_index`, allocating a new node
 * from object_list_reference_data. header_index's count is bumped either way, even if the allocation failed.
 *
 * @address 0x48b2a0
 */
void ObjectLists::reference_add(datum_index header_index, datum_index object_index) const
{
    object_list_header *header;
    datum_index node_index;
    object_list_reference *node;

    header = (object_list_header *)((uint8_t *)object_list_header_data->data +
        (header_index & halo::k_slot_mask) * 0x0c);
    node_index = halo::memory::datum_new(object_list_reference_data);
    if (node_index != k_datum_index_none) {
        node = (object_list_reference *)((uint8_t *)object_list_reference_data->data +
            (node_index & halo::k_slot_mask) * 0x0c);
        node->object_index = object_index;
        node->next = header->first_reference;
        header->first_reference = node_index;
    }
    header->count = header->count + 1;
}

/**
 * Deletes every node in the singly-linked reference chain starting at `chain_head`.
 *
 * @address 0x48b220
 */
void ObjectLists::reference_chain_delete(data_array *reference_array, datum_index chain_head) const
{
    object_list_reference *node;
    datum_index next;

    while (chain_head != k_datum_index_none) {
        node = (object_list_reference *)((uint8_t *)reference_array->data +
            (chain_head & halo::k_slot_mask) * 0x0c);
        next = node->next;
        halo::memory::datum_delete(reference_array, chain_head);
        chain_head = next;
    }
}

/**
 * Sweeps every object_list_header whose reference_count (holder count) has dropped to zero, deleting its
 * reference chain and then the header itself.
 *
 * @address 0x48b340
 */
void ObjectLists::dispose_empty() const
{
    datum_index header_index;
    object_list_header *header;

    header_index = halo::memory::datum_next(-1, object_list_header_data);
    while (header_index != k_datum_index_none) {
        header = (object_list_header *)((uint8_t *)object_list_header_data->data +
            (header_index & halo::k_slot_mask) * 0x0c);
        if (header->reference_count == 0) {
            object_list_reference_chain_delete(object_list_reference_data, header->first_reference);
            halo::memory::datum_delete(object_list_header_data, header_index);
        }
        header_index = halo::memory::datum_next((int16_t)header_index, object_list_header_data);
    }
}

/**
 * Creates the two global data arrays backing the object_list script type: the list headers, and the
 * singly-linked reference nodes each list's chain is built from.
 *
 * @address 0x48b250
 */
void ObjectLists::initialize() const
{
    char name[256];

    object_list_header_data = halo::saved_games::game_state_new((char *)"object list header", k_hs_object_list_header_count, 0xc );
    sprintf(name, "%s reference", "list object");
    object_list_reference_data = halo::saved_games::game_state_new(name, k_hs_object_list_reference_count, 0xc );
}

}
