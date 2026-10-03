#include "halo/memory/memory.hpp"

#include "tags.h"
#include <string.h>
#include "crt.h"
#include "math.h"
#include <stdint.h>
#include "halo/memory/api.hpp"
#include "halo/core/datum.hpp"

namespace halo::memory {

/** High bit of a heap block size word: set while the block is allocated. */
inline constexpr uint32_t k_heap_block_in_use_bit = 0x80000000u;

/**
 * Carves a block of requested_size payload bytes out of the pool's free space and links it at the end
 * of the block list. On success the payload address is stored through owner so that compaction can
 * relocate the block later; the block header is stamped with its head and tail signatures.
 *
 * Returns the payload address, or 0 when the pool has too little free space.
 *
 * @address 0x4d1d60
 */
int32_t memory_pool_view::allocate(int32_t requested_size, void **owner)
{
    int32_t block_size = requested_size + 0x18;
    memory_pool_block *block;

    if ((block_size & 3) != 0) {
        block_size = (block_size | 3) + 1;
    }

    if (this->last_block == 0) {
        block = (memory_pool_block *)this->base;
    } else {
        block = (memory_pool_block *)((uint8_t *)this->last_block + this->last_block->size);
    }

    if ((uint8_t *)block + block_size <= (uint8_t *)this->base + this->size && block != 0) {
        block->head_signature = k_memory_pool_block_head_signature;
        block->size = block_size;
        block->address = owner;
        block->next = 0;
        block->previous = this->last_block;
        block->tail_signature = k_memory_pool_block_tail_signature;
        if (this->first_block == 0) {
            this->first_block = block;
        }
        if (this->last_block != 0) {
            this->last_block->next = block;
        }
        this->last_block = block;
        this->free_bytes -= block->size;
        *owner = (uint8_t *)block + 0x18;
        return 1;
    }
    return 0;
}

/**
 * Slides every live block of the pool down over the gaps left by unlinked blocks, rewriting each owner
 * pointer to the block's new payload address. The block list order is unchanged.
 *
 * @address 0x4d1eb0
 */
void memory_pool_view::compact()
{
    memory_pool_block *src = this->first_block;
    memory_pool_block *dest;
    memory_pool_block *block = 0;
    memory_pool_block *previous = 0;

    if (src == 0) {
        return;
    }

    dest = (memory_pool_block *)this->base;
    do {
        block = src;
        if (dest < src) {
            memmove(dest, src, (size_t)src->size);
            *dest->address = (uint8_t *)dest + 0x18;

            block = dest;
        }
        block->previous = previous;
        if (previous == 0) {
            this->first_block = block;
        } else {
            previous->next = block;
        }
        dest = (memory_pool_block *)((uint8_t *)block + block->size);
        src = block->next;
        previous = block;
    } while (src != 0);

    block->next = 0;
    this->last_block = block;
}

/**
 * Resizes the block that owner_cell points into. When the block cannot grow in place a new block is
 * allocated, the payload is copied across and the old block is unlinked; owner_cell is updated either
 * way.
 *
 * Returns nonzero on success.
 *
 * @address 0x4d1de0
 */
int32_t memory_pool_view::reallocate(void **owner_cell, int32_t new_size)
{
    void *old_payload = *owner_cell;
    memory_pool_block *old_block = (memory_pool_block *)((uint8_t *)old_payload - 0x18);
    int32_t block_size = new_size + 0x18;
    uint32_t boundary;

    if ((block_size & 3) != 0) {
        block_size = (block_size | 3) + 1;
    }

    boundary = old_block->next != 0 ? (uint32_t)old_block->next :
        (uint32_t)((uint8_t *)this->base + this->size);

    if ((uint32_t)((uint8_t *)old_block + block_size) <= boundary) {
        this->free_bytes += old_block->size - block_size;
        old_block->size = block_size;
        return 1;
    }

    if (this->allocate(new_size, owner_cell)) {
        void *new_payload = *owner_cell;
        memory_pool_block *new_block = (memory_pool_block *)((uint8_t *)new_payload - 0x18);
        uint32_t old_payload_size = (uint32_t)old_block->size - 0x18;

        memcpy(new_payload, old_payload, old_payload_size);
        this->unlink(&old_payload);

        new_block->address = owner_cell;
        *owner_cell = new_payload;
        return 1;
    }
    return 0;
}

/**
 * Removes the block that payload_ptr points into from the block list and returns its bytes to the
 * pool's free count. payload_ptr is the address of the owner pointer, not of the block header.
 *
 * @address 0x4d1e70
 */
void memory_pool_view::unlink(void **payload_ptr)
{
    memory_pool_block *block = (memory_pool_block *)((uint8_t *)*payload_ptr - 0x18);

    this->free_bytes += block->size;
    if (block->previous == 0) {
        this->first_block = block->next;
    } else {
        block->previous->next = block->next;
    }
    if (block->next != 0) {
        block->next->previous = block->previous;
        return;
    }
    this->last_block = block->previous;
}

/**
 * Moves next_free_slot forward to the next empty entry of the block pointer table, or to -1 when the
 * table has no empty entry left.
 *
 * @address 0x4d2140
 */
void heap_view::advance_free_slot()
{
    int32_t slot;
    heap_block **entry;

    if (this->next_free_slot == -1) {
        return;
    }
    slot = this->next_free_slot + 1;
    this->next_free_slot = -1;
    if (slot < this->maximum_blocks) {
        entry = &this->blocks[slot];
        while (*entry != 0) {
            slot = slot + 1;
            entry = entry + 1;
            if (this->maximum_blocks <= slot) {
                return;
            }
        }
        this->next_free_slot = slot;
    }
}

/**
 * Allocates size payload bytes from the heap and updates the usage statistics (bytes allocated,
 * allocation count and their peaks, largest single allocation). Returns the payload address, or 0 when
 * no block fits.
 *
 * @address 0x4d1f10
 */
void *heap_view::allocate(uint32_t size)
{
    heap_block *block = (heap_block *)this->allocate_raw(size);
    void *payload;
    uint32_t allocation_count;

    if (block == 0) {
        return 0;
    }

    payload = (uint8_t *)block + 0x10;
    block->size = block->size | k_heap_block_in_use_bit;
    allocation_count = (uint32_t)this->allocation_count + 1;
    this->bytes_allocated += (int32_t)(block->size & k_heap_block_size_mask);
    this->allocation_count = (int32_t)allocation_count;

    if (this->peak_bytes_allocated < this->bytes_allocated) {
        this->peak_bytes_allocated = this->bytes_allocated;
    }
    if ((uint32_t)this->peak_allocation_count < allocation_count) {
        this->peak_allocation_count = (int32_t)allocation_count;
    }
    if ((uint32_t)this->peak_allocation_size < (block->size & k_heap_block_size_mask)) {
        this->peak_allocation_size = (int32_t)(block->size & k_heap_block_size_mask);
    }
    return payload;
}

/**
 * Finds or makes room for a block of size payload bytes plus the 0x10 byte header, rounded up to a
 * multiple of four. Compacts the heap first when the free bytes are enough but no single gap is, then
 * splits or takes over the chosen free block and registers it in the block pointer table.
 *
 * Returns the block header address, or 0 when the request cannot be satisfied.
 *
 * @address 0x4d2180
 */
uint32_t heap_view::allocate_raw(uint32_t size)
{
    uint32_t block_size;
    int32_t free_bytes;
    int32_t found_block;
    heap_block *predecessor;
    int32_t slot;
    heap_block *block;
    uint32_t block_addr;

    if (size == 0 || k_heap_block_in_use_bit <= size || (uint32_t)this->size <= size) {
        return 0;
    }

    found_block = 0;
    predecessor = 0;
    for (block_size = size + 0x10; (block_size & 3) != 0; block_size = block_size + 1) {

    }

    free_bytes = this->size;
    if (this->first_block != 0) {
        free_bytes = this->get_free_bytes();
    }

    if ((uint32_t)free_bytes < block_size) {
        this->compact();
        free_bytes = this->get_free_bytes();
        if ((uint32_t)free_bytes < block_size) {
            found_block = this->find_free_block(block_size, (void **)&predecessor);
            if (found_block == 0) {
                return 0;
            }
        }
    }

    if (this->next_free_slot == -1) {
        this->next_free_slot = (int32_t)this->find_first_free_slot();
    }
    slot = this->next_free_slot;
    if (slot == -1) {
        return 0;
    }

    if (found_block == 0) {
        if (this->first_block == 0) {
            this->blocks[slot] = (heap_block *)this->base;
        } else {
            this->blocks[slot] = (heap_block *)((uint8_t *)this->last_block +
                (this->last_block->size & k_heap_block_size_mask));
        }
    } else {
        this->blocks[slot] = (heap_block *)found_block;
    }

    block = this->blocks[this->next_free_slot];
    block->size = block_size;
    block->slot = (int32_t)this->next_free_slot;
    block = this->blocks[this->next_free_slot];
    block_addr = (uint32_t)block;

    if (this->first_block == 0) {
        this->last_block = block;
        this->first_block = block;
        block->previous = 0;
        block->next = 0;
        this->advance_free_slot();
        return block_addr;
    }
    if (block_addr < (uint32_t)this->first_block) {
        block->previous = 0;
        block->next = this->first_block;
        this->first_block->previous = block;
        this->first_block = block;
        this->advance_free_slot();
        return block_addr;
    }
    if (block_addr <= (uint32_t)this->last_block) {
        block->previous = predecessor;
        block->next = predecessor->next;
        predecessor->next = block;
        if (block->next != 0) {
            block->next->previous = block;
        }
        this->advance_free_slot();
        return block_addr;
    }
    block->next = 0;
    block->previous = this->last_block;
    this->last_block->next = block;
    this->last_block = block;
    this->advance_free_slot();
    return block_addr;
}

/**
 * Slides every allocated block towards the start of the heap so that the free bytes form one gap at
 * the end. Does nothing while compaction_disabled is set.
 *
 * @address 0x4d2310
 */
void heap_view::compact()
{
    heap_block *src = this->first_block;
    heap_block *dest;
    uint32_t shift;
    heap_block *prev;

    if (src == 0 || this->compaction_disabled != 0) {
        return;
    }

    shift = 0;
    prev = (heap_block *)this->base;
    do {
        dest = src;
        if (0 <= (int32_t)src->size &&
            (heap_block *)((uint8_t *)src - shift) != prev &&
            0 <= (int32_t)(((uint8_t *)src - shift) - (uint8_t *)prev)) {
            dest = (heap_block *)((uint8_t *)prev + shift);
            memmove(dest, src, src->size & k_heap_block_size_mask);
            if (dest->previous != 0) {
                dest->previous->next = dest;
            }
        }
        src = dest->next;
        shift = dest->size & k_heap_block_size_mask;
        prev = dest;
    } while (src != 0);
}

/**
 * Returns the index of the first empty entry in the block pointer table, or -1 when every entry is in
 * use.
 *
 * @address 0x4d2110
 */
uint32_t heap_view::find_first_free_slot()
{
    uint32_t slot = halo::k_dword_none;

    if (this->maximum_blocks != 0) {
        heap_block **entry = &this->blocks[0];
        slot = 0;
        while (*entry != 0) {
            slot = slot + 1;
            entry = entry + 1;
            if ((uint32_t)this->maximum_blocks <= slot) {
                return halo::k_dword_none;
            }
        }
    }
    return slot;
}

/**
 * Scans the address-ordered block list for the first free gap of at least size_needed bytes. Returns
 * the gap's offset and stores the block that precedes it through out_predecessor.
 *
 * @address 0x4d2370
 */
int32_t heap_view::find_free_block(uint32_t size_needed, void **out_predecessor)
{
    heap_block *prev = this->first_block;
    int32_t result = 0;

    if (prev == 0) {
        return result;
    }
    if (size_needed <= (uint32_t)((uint8_t *)prev - this->base)) {
        return (int32_t)this->base;
    }
    if (prev->next != 0) {
        heap_block *next = prev->next;
        heap_block *cur;
        while (cur = next,
            (uint32_t)((uint8_t *)cur - ((uint8_t *)prev + (prev->size & k_heap_block_size_mask))) <
                size_needed) {
            next = cur->next;
            prev = cur;
            if (cur->next == 0) {
                return result;
            }
        }
        result = (int32_t)((prev->size & k_heap_block_size_mask) + (uint8_t *)prev);
        *out_predecessor = prev;
    }
    return result;
}

/**
 * Returns how many bytes of the heap are not currently allocated, block headers included.
 *
 * @address 0x4d20f0
 */
int32_t heap_view::get_free_bytes()
{
    int32_t free_bytes = this->size;

    if (this->first_block != 0) {
        free_bytes = ((free_bytes - (int32_t)(this->last_block->size & k_heap_block_size_mask)) +
            (int32_t)this->base) - (int32_t)this->last_block;
    }
    return free_bytes;
}

/**
 * Resizes the allocation whose payload is old_payload to new_size bytes, keeping the contents, and
 * updates the usage statistics. A null old_payload behaves like heap_allocate. Returns the new payload
 * address, or 0 on failure.
 *
 * @address 0x4d1f80
 */
void *heap_view::reallocate(void *old_payload, uint32_t new_size)
{
    heap_block *old_block = old_payload == 0 ? 0 : (heap_block *)((uint8_t *)old_payload - 0x10);
    uint32_t old_size = 0;
    heap_block *block;
    void *payload;
    int32_t bytes_allocated;
    uint32_t allocation_count;

    if (old_block != 0) {
        old_size = old_block->size & k_heap_block_size_mask;
    }

    block = (heap_block *)this->resize_block(new_size, old_block);
    if (block == 0) {
        return 0;
    }

    if (0 <= (int32_t)block->size) {

        block->size = block->size | k_heap_block_in_use_bit;
    }
    bytes_allocated = this->bytes_allocated + (int32_t)((block->size & k_heap_block_size_mask) - old_size);
    this->bytes_allocated = bytes_allocated;
    payload = (uint8_t *)block + 0x10;
    allocation_count = (uint32_t)this->allocation_count + (old_size == 0 ? 1u : 0u);
    this->allocation_count = (int32_t)allocation_count;

    if (this->peak_bytes_allocated < bytes_allocated) {
        this->peak_bytes_allocated = bytes_allocated;
    }
    if ((uint32_t)this->peak_allocation_count < allocation_count) {
        this->peak_allocation_count = (int32_t)allocation_count;
    }
    if ((uint32_t)this->peak_allocation_size < (block->size & k_heap_block_size_mask)) {
        this->peak_allocation_size = (int32_t)(block->size & k_heap_block_size_mask);
    }
    return payload;
}

/**
 * Grows or shrinks old_block (or makes a fresh block when it is null) to hold new_size payload bytes,
 * moving the contents into a new block when it cannot be resized in place. Returns the resulting block
 * header.
 *
 * @address 0x4d2020
 */
void *heap_view::resize_block(uint32_t new_size, heap_block *old_block)
{
    void *new_block;

    if (new_size == 0) {
        return 0;
    }
    if (old_block == 0) {
        return (void *)this->allocate_raw(new_size);
    }
    if (new_size <= (old_block->size & k_heap_block_size_mask) - 0x10) {
        return old_block;
    }
    new_block = (void *)this->allocate_raw(new_size);
    if (new_block != 0) {
        uint32_t old_payload_size = (old_block->size & k_heap_block_size_mask) - 0x10;
        uint8_t *old_payload = (uint8_t *)old_block + 0x10;
        uint8_t *new_payload = (uint8_t *)new_block + 0x10;

        memcpy(new_payload, old_payload, old_payload_size);
        this->unlink_block(old_block);
        return new_block;
    }
    return 0;
}

/**
 * Removes block from the address-ordered list, clears its pointer table slot and returns its bytes to
 * the free count.
 *
 * @address 0x4d20a0
 */
void heap_view::unlink_block(heap_block *block)
{
    uint32_t slot = block->slot;

    if (block->previous != 0) {
        block->previous->next = block->next;
    }
    if (block->next != 0) {
        block->next->previous = block->previous;
    }
    if (block == this->first_block) {
        this->first_block = block->next;
    }
    if (block == this->last_block) {
        this->last_block = block->previous;
    }
    this->blocks[slot] = 0;
    this->next_free_slot = -(int32_t)(this->first_block != 0) & (int32_t)slot;
}

/**
 * Initialises a cache container in the memory this points at: builds the embedded entry data_array of
 * maximum_count entries, clears the header, and stores the name, the two procedures and the block
 * geometry. The age counter starts at 1 and the entry list is empty.
 *
 * release_procedure is called for an entry before it is evicted; in_use_procedure vetoes the eviction
 * of a busy entry.
 *
 * @address 0x4d1750
 */
void cache_view::initialize(char *name, int32_t block_count, int32_t block_shift, int16_t maximum_count, void *release_procedure, void *in_use_procedure)
{
    data_array *entries = &this->entry_data;

    memset(entries, 0, sizeof(*entries));
    strncpy(entries->name, name, 0x1f);
    entries->maximum_count = maximum_count;
    entries->size = sizeof(cache_entry);
    entries->signature = k_data_array_signature;
    entries->data = (uint8_t *)this + 0x7c;
    entries->valid = 0;
    entries->valid = 1;
    halo::memory::view(entries)->delete_all();

    memset(this, 0, 0x44);
    strncpy(this->name, name, 0x1f);
    this->release_procedure = release_procedure;
    this->in_use_procedure = in_use_procedure;
    this->entries = entries;
    this->block_count = block_count;
    this->block_shift = block_shift;
    this->signature = k_cache_signature;
    this->first = halo::k_dword_none;
    this->last = halo::k_dword_none;
    this->age = 1;
}

/**
 * Finds or makes room for requested_bytes of the cache region and returns a new entry handle that
 * covers it, or k_datum_index_none when no window can be freed. The scan keeps up to 256 candidate
 * windows over the entries in offset order, prefers the window whose newest evicted entry is oldest,
 * never evicts a busy or current-age entry, and evicts the least recently used entry when the entry
 * table is full.
 *
 * The chosen window's entries are evicted and the new entry is linked after the window's predecessor.
 *
 * @address 0x4d1840
 */
datum_index cache_view::allocate_block(uint32_t requested_bytes)
{
    cache_allocation_gap gaps[256];
    cache_allocation_gap best;
    int32_t blocks_needed;
    datum_index cursor = this->first;
    datum_index gap_previous = k_datum_index_none;
    datum_index lru = k_datum_index_none;
    uint32_t lru_age = 0;
    int16_t window_start = 0;
    int16_t write = 0;
    int32_t offset = 0;
    uint8_t found = 0;
    datum_index handle;
    cache_entry *entry;

    blocks_needed = (int32_t)requested_bytes >> this->block_shift;
    if ((requested_bytes & ((1u << this->block_shift) - 1u)) != 0) {
        blocks_needed++;
    }
    best.previous_entry = k_datum_index_none;
    best.newest_age = 0;
    best.offset = 0;
    best.size = 0;

    if (this->block_count <= 0) {
        return k_datum_index_none;
    }

    do {
        int16_t next = (write == 0xff) ? 0 : (int16_t)(write + 1);
        int32_t gap;
        uint32_t entry_age;

        if (next != window_start) {
            gaps[write].previous_entry = gap_previous;
            gaps[write].newest_age = 0;
            gaps[write].offset = offset;
            gaps[write].size = 0;
            write = next;
        }

        if (cursor == k_datum_index_none) {
            entry_age = 0;
            gap = this->block_count - offset;
            offset = this->block_count;
        } else {
            entry = this->entry_at(cursor);
            if (offset != entry->offset) {
                gap = entry->offset - offset;
                entry_age = 0;
                offset = entry->offset;
            } else {
                uint8_t protected_entry = 0;

                entry_age = entry->age;
                gap = entry->size;
                if (this->in_use_procedure != 0 &&
                    (uint8_t)((int32_t (*)(datum_index))this->in_use_procedure)(cursor) != 0) {
                    protected_entry = 1;
                }
                if (entry->age == this->age) {
                    protected_entry = 1;
                } else if (!protected_entry && (lru == k_datum_index_none || entry->age < lru_age)) {
                    lru = cursor;
                    lru_age = entry->age;
                }
                offset = entry->size + entry->offset;
                gap_previous = cursor;
                cursor = entry->next;
                if (protected_entry) {
                    window_start = write;
                    continue;
                }
            }
        }

        {
            int16_t scan = window_start;

            while (scan != write) {
                cache_allocation_gap *window = &gaps[scan];

                if (entry_age > window->newest_age) {
                    window->newest_age = entry_age;
                }
                window->size += gap;
                if (window->size >= blocks_needed) {
                    if (!found || window->newest_age < best.newest_age ||
                        (window->newest_age == best.newest_age && window->size < best.size)) {
                        best = *window;
                        found = 1;
                    }
                    window_start = (window_start == 0xff) ? 0 : (int16_t)(window_start + 1);
                }
                scan = (scan == 0xff) ? 0 : (int16_t)(scan + 1);
            }
        }
    } while (offset < this->block_count);

    if (!found) {
        return k_datum_index_none;
    }

    {
        data_iterator iterator;
        cache_entry *candidate;

        iterator.data = this->entries;
        iterator.next_index = 0;
        iterator.index = k_datum_index_none;
        iterator.signature = (uint32_t)(uintptr_t)this->entries ^ k_data_iterator_signature;
        while ((candidate = (cache_entry *)halo::memory::view(&iterator)->next()) != 0) {
            if (candidate->offset < blocks_needed + best.offset && candidate->size + candidate->offset > best.offset) {
                this->evict_entry(iterator.index);
            }
        }
    }

    if (this->entries->actual_count == this->entries->maximum_count && lru != k_datum_index_none) {
        if (best.previous_entry == lru) {
            best.previous_entry = this->entry_at(lru)->previous;
        }
        this->evict_entry(lru);
    }

    handle = halo::memory::view(this->entries)->new_datum();
    if (handle == k_datum_index_none) {
        return handle;
    }
    entry = this->entry_at(handle);
    if (best.previous_entry == k_datum_index_none) {
        entry->previous = k_datum_index_none;
        if (this->first == k_datum_index_none) {
            this->last = handle;
        } else {
            this->entry_at(this->first)->previous = handle;
        }
        entry->next = this->first;
        this->first = handle;
    } else {
        cache_entry *previous = this->entry_at(best.previous_entry);

        if (previous->next == k_datum_index_none) {
            entry->previous = this->last;
            this->last = handle;
        } else {
            cache_entry *following = this->entry_at(previous->next);

            entry->previous = following->previous;
            following->previous = handle;
        }
        entry->next = previous->next;
        previous->next = handle;
    }
    entry->offset = best.offset;
    entry->size = blocks_needed;
    entry->age = this->age;
    return handle;
}

/**
 * Fills bitmap with one status byte per cache block using the cache_block_status_flags bits:
 * allocated, current age, stale and locked. The locked bit comes from in_use_procedure.
 *
 * @address 0x4d1ca0
 */
void cache_view::build_status_bitmap(uint8_t *bitmap)
{
    data_iterator iterator;
    cache_entry *entry;

    memset(bitmap, 0, (uint32_t)this->block_count);

    iterator.data = this->entries;
    iterator.next_index = 0;
    iterator.index = 0;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
    entry = (cache_entry *)halo::memory::view(&iterator)->next();
    while (entry != 0) {
        uint8_t status = _cache_block_allocated_bit;

        if (this->in_use_procedure != 0 &&
            ((int32_t (*)(datum_index))this->in_use_procedure)(halo::k_dword_none) != 0) {
            status = 9;
        }
        if ((uint32_t)entry->age == (uint32_t)this->age) {
            status = status | _cache_block_current_bit;
        }
        if ((uint32_t)entry->age + 0x1e < (uint32_t)this->age) {
            status = status | _cache_block_stale_bit;
        }

        memset(bitmap + entry->offset, status, (uint32_t)entry->size);

        entry = (cache_entry *)halo::memory::view(&iterator)->next();
    }
}

/**
 * Evicts one entry: calls release_procedure, unlinks the entry from the offset-ordered list and frees
 * its datum.
 *
 * @address 0x4d1c20
 */
void cache_view::evict_entry(datum_index handle)
{
    cache_entry *entry = (cache_entry *)((uint8_t *)this->entries->data +
        (uint32_t)(uint16_t)handle * sizeof(cache_entry));

    if (this->release_procedure != 0) {
        ((void (*)(datum_index))this->release_procedure)(handle);
    }

    if (entry->previous == halo::k_dword_none) {
        this->first = entry->next;
    } else {
        cache_entry *previous = (cache_entry *)((uint8_t *)this->entries->data +
            (uint32_t)(uint16_t)entry->previous * sizeof(cache_entry));
        previous->next = entry->next;
    }

    if (entry->next != halo::k_dword_none) {
        cache_entry *next = (cache_entry *)((uint8_t *)this->entries->data +
            (uint32_t)(uint16_t)entry->next * sizeof(cache_entry));
        next->previous = entry->previous;
        halo::memory::view(this->entries)->delete_datum(handle);
        return;
    }
    this->last = entry->previous;
    halo::memory::view(this->entries)->delete_datum(handle);
}

/**
 * Evicts every entry of the cache by iterating the entry data_array.
 *
 * @address 0x4d17f0
 */
void cache_view::flush()
{
    data_iterator iterator;

    iterator.data = this->entries;
    iterator.next_index = 0;
    iterator.index = 0;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;

    while (halo::memory::view(&iterator)->next() != 0) {
        this->evict_entry(iterator.index);
    }
}

/**
 * Returns the cache_entry that the datum handle names, without validating the handle.
 */
cache_entry *cache_view::entry_at(datum_index handle)
{
    return (cache_entry *)((uint8_t *)this->entries->data + (uint32_t)(uint16_t)handle * sizeof(cache_entry));
}

} // namespace halo::memory
