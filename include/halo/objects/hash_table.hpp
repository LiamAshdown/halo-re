/**
 * @file include/halo/objects/hash_table.hpp
 * Object system API: hash table.
 * The C symbols other modules link against are the wrappers in src/objects/objects_c_api.cpp.
 */
#pragma once

#include "halo/objects/engine_types.hpp"

namespace halo::objects {

/**
 * Chained hash table of int32 keys to int32 values, backed by GlobalAlloc node blocks.
 */
class HashTableView {
public:
    explicit HashTableView(hash_table *self) : self(self) {}

    /**
     * Allocates bucket_count empty buckets and prepares an empty table.
     *
     * Original register convention: ESI -> table, EAX -> bucket_count.
     *
     * @address 0x004f0470
     */
    void initialize(int32_t bucket_count);

    /**
     * Frees the bucket array and all node blocks of the table and marks it uninitialised.
     *
     * Original register convention: EDI -> table.
     *
     * @address 0x004f04c0
     */
    void dispose();

    /**
     * Stores value under key, replacing an existing entry, or removes the entry when the value is the removal marker.
     *
     * @address 0x004f0530
     */
    void set_or_remove(int32_t key, int32_t value);

    /**
     * Returns the value stored for key, or the table's not-found value when the key is absent.
     *
     * Original register convention: table pointer in ESI, key in ECX, return value in EAX (-1 when absent or when the
     * table is uninitialized or the key is -1) -- confirmed by objdump.
     *
     * @address 0x004f05e0
     */
    int32_t get(int32_t key);

    /**
     * Allocates another block of nodes and links them into the free list.
     *
     * Original register convention: table pointer in ESI, no other arguments -- confirmed by objdump; this matches
     * the ESI table pointer live at hash_table_set_or_remove's call site (0x4f0574), which moves EAX into ESI before
     * calling.
     *
     * @address 0x004f0620
     */
    void grow_freelist();

private:
    hash_table *self;
};

/**
 * Walks the object hash chain and toggles the per-entry bit 3 flag.
 */
class ObjectHashFlags {
public:
    /**
     * Clears bit 3 in the hash chain entry for the key.
     *
     * Original register convention: uint32_t key in EAX (in_EAX).
     *
     * @address 0x004ef160
     */
    static void clear_bit3(uint32_t key);

    /**
     * Sets bit 3 in the hash chain entry for the key.
     *
     * Original register convention: uint32_t key in EAX (in_EAX).
     *
     * @address 0x004ef200
     */
    static void set_bit3(uint32_t key);
};

}  // namespace halo::objects
