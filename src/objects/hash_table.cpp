#include "halo/objects/hash_table.hpp"
#include "halo/objects/api.hpp"
#include "halo/core/link.hpp"
#include "halo/objects/vars.hpp"

static auto &object_data = halo::link::ref<data_array *>(halo::objects::vars().object_data);
static auto &object_list_header_data = halo::link::ref<data_array *>(halo::objects::vars().object_list_header_data);
static auto &object_list_reference_data = halo::link::ref<data_array *>(halo::objects::vars().object_list_reference_data);

/**
 * Clears bit 3 in the hash chain entry for the key.
 *
 * Original register convention: uint32_t key in EAX (in_EAX).
 *
 * @address 0x004ef160
 */
void halo::objects::ObjectHashFlags::clear_bit3(uint32_t key)
{
    object_header *headers = (object_header *)object_data->data;
    uint32_t node = k_datum_index_none;
    uint32_t next_node;

    if (key != k_datum_index_none) {

        node = *(uint32_t *)((uint8_t *)object_list_header_data->data + halo::datum_slot(key) * 0xc + 8);
        if (node == k_datum_index_none) {
            next_node = k_datum_index_none;
        } else {
            uint8_t *entry = (uint8_t *)object_list_reference_data->data + halo::datum_slot(node) * 0xc;
            next_node = *(uint32_t *)(entry + 8);
            node = *(uint32_t *)(entry + 4);
        }
    } else {
        next_node = k_datum_index_none;
    }

    while (node != k_datum_index_none) {
        object *obj = headers[halo::datum_slot(node)].data;
        *((uint8_t *)obj + 0x107) &= 0xf7;

        if (next_node == k_datum_index_none) {
            node = k_datum_index_none;
        } else {
            uint8_t *entry = (uint8_t *)object_list_reference_data->data + halo::datum_slot(next_node) * 0xc;
            next_node = *(uint32_t *)(entry + 8);
            node = *(uint32_t *)(entry + 4);
        }
    }
}

/**
 * Sets bit 3 in the hash chain entry for the key.
 *
 * Original register convention: uint32_t key in EAX (in_EAX).
 *
 * @address 0x004ef200
 */
void halo::objects::ObjectHashFlags::set_bit3(uint32_t key)
{
    object_header *headers = (object_header *)object_data->data;
    uint32_t node = k_datum_index_none;
    uint32_t next_node;

    if (key != k_datum_index_none) {

        node = *(uint32_t *)((uint8_t *)object_list_header_data->data + halo::datum_slot(key) * 0xc + 8);
        if (node == k_datum_index_none) {
            next_node = k_datum_index_none;
        } else {
            uint8_t *entry = (uint8_t *)object_list_reference_data->data + halo::datum_slot(node) * 0xc;
            next_node = *(uint32_t *)(entry + 8);
            node = *(uint32_t *)(entry + 4);
        }
    } else {
        next_node = k_datum_index_none;
    }

    while (node != k_datum_index_none) {
        object *obj = headers[halo::datum_slot(node)].data;
        *((uint8_t *)obj + 0x107) |= 8;

        if (next_node == k_datum_index_none) {
            node = k_datum_index_none;
        } else {
            uint8_t *entry = (uint8_t *)object_list_reference_data->data + halo::datum_slot(next_node) * 0xc;
            next_node = *(uint32_t *)(entry + 8);
            node = *(uint32_t *)(entry + 4);
        }
    }
}

/**
 * Allocates bucket_count empty buckets and prepares an empty table.
 *
 * Original register convention: ESI -> table, EAX -> bucket_count.
 *
 * @address 0x004f0470
 */
void halo::objects::HashTableView::initialize(int32_t bucket_count)
{
    hash_table *table = self;
    int32_t i;

    if (table->initialized != 0) {
        return;
    }
    table->bucket_count = bucket_count;
    table->buckets = (hash_bucket *)GlobalAlloc(0, bucket_count * 8);
    for (i = 0; i < table->bucket_count; i++) {
        table->buckets[i].count = 0;
        table->buckets[i].first = 0;
    }
    table->entry_count = 0;
    table->freelist = 0;
    table->blocks = 0;
    table->initialized = 1;
}

/**
 * Frees the bucket array and all node blocks of the table and marks it uninitialised.
 *
 * Original register convention: EDI -> table.
 *
 * @address 0x004f04c0
 */
void halo::objects::HashTableView::dispose()
{
    hash_table *table = self;
    hash_node_block *block;
    int32_t i;

    if (table->initialized != 1) {
        return;
    }
    for (i = 0; i < table->bucket_count; i++) {
        table->buckets[i].first = 0;
        table->buckets[i].count = 0;
    }
    table->freelist = 0;
    block = table->blocks;
    while (block != 0) {
        hash_node_block *next = block->next;

        GlobalFree(block->nodes);
        block->nodes = 0;
        GlobalFree(block);
        block = next;
    }
    table->blocks = 0;
    GlobalFree(table->buckets);
    table->buckets = 0;
    table->bucket_count = 0;
    table->entry_count = 0;
    table->initialized = 0;
}

/**
 * Stores value under key, replacing an existing entry, or removes the entry when the value is the removal marker.
 *
 * @address 0x004f0530
 */
void halo::objects::HashTableView::set_or_remove(int32_t key, int32_t value)
{
    hash_table *table = self;
    hash_bucket *bucket;
    hash_node *prev;
    hash_node *node;
    int32_t unsigned_key;

    if (table->initialized != 1) {
        return;
    }

    unsigned_key = (key < 0) ? -key : key;
    bucket = table->buckets + (unsigned_key % table->bucket_count);

    prev = 0;
    for (node = bucket->first; node != 0; node = node->next) {
        if (node->key == key) {
            if (value == -1) {
                if (prev == 0) {
                    bucket->first = node->next;
                } else {
                    prev->next = node->next;
                }
                node->key = -1;
                node->value = -1;
                node->next = table->freelist;
                table->freelist = node;
                bucket->count--;
                table->entry_count--;
                return;
            }
            node->value = value;
            return;
        }
        prev = node;
    }

    if (table->freelist == 0) {
        halo::objects::hash_table_grow_freelist(table);
    }
    node = table->freelist;
    table->freelist = node->next;
    node->key = key;
    node->value = value;
    node->next = bucket->first;
    bucket->first = node;
    bucket->count++;
    table->entry_count++;
}

/**
 * Returns the value stored for key, or the table's not-found value when the key is absent.
 *
 * Original register convention: table pointer in ESI, key in ECX, return value in EAX (-1 when absent or when the
 * table is uninitialized or the key is -1) -- confirmed by objdump.
 *
 * @address 0x004f05e0
 */
int32_t halo::objects::HashTableView::get(int32_t key)
{
    hash_table *table = self;
    hash_bucket *bucket;
    hash_node *node;
    int32_t unsigned_key;

    if (table->initialized != 1 || key == -1) {
        return -1;
    }

    unsigned_key = (key < 0) ? -key : key;
    bucket = table->buckets + (unsigned_key % table->bucket_count);

    for (node = bucket->first; node != 0; node = node->next) {
        if (node->key == key) {
            return node->value;
        }
    }
    return -1;
}

/**
 * Allocates another block of nodes and links them into the free list.
 *
 * Original register convention: table pointer in ESI, no other arguments -- confirmed by objdump; this matches the
 * ESI table pointer live at hash_table_set_or_remove's call site (0x4f0574), which moves EAX into ESI before calling.
 *
 * @address 0x004f0620
 */
void halo::objects::HashTableView::grow_freelist()
{
    hash_table *table = self;
    hash_node_block *block;
    hash_node *nodes;
    int32_t i;

    block = (hash_node_block *)GlobalAlloc(0, sizeof(hash_node_block));
    nodes = (hash_node *)GlobalAlloc(0, 600);

    block->nodes = nodes;
    block->next = table->blocks;
    table->blocks = block;

    for (i = 0; i < 50; i++) {
        hash_node *node = &block->nodes[i];
        node->key = -1;
        node->value = -1;
        node->next = table->freelist;
        table->freelist = node;
    }
}
