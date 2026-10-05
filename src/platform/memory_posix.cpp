/**
 * @file src/platform/memory_posix.cpp
 * halo::platform heap and page memory on POSIX (Emscripten, Linux). A reservation at a fixed address (the map memory at
 * 0x40000000) is a fixed anonymous mapping on Linux; in WebAssembly's linear memory it is claimed by moving the heap
 * break past it, which works while the heap has not grown that far (the reservation is made at startup).
 */

#include "halo/platform/memory.hpp"
#include "halo/platform/file.hpp"
#include "posix_internal.hpp"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#if !defined(__EMSCRIPTEN__)
#include <sys/mman.h>
#endif

namespace halo::platform {

void *heap_allocate(uint32_t flags, uint32_t size)
{
    void *block = (flags & k_heap_zero) != 0 ? calloc(1, size != 0 ? size : 1) : malloc(size != 0 ? size : 1);

    if (block == nullptr) {
        set_last_error(posix::k_error_not_enough_memory);
    }
    return block;
}

void *heap_reallocate(void *block, uint32_t size, uint32_t flags)
{
    (void)flags;  // ponytail: always allowed to move; GlobalReAlloc without GMEM_MOVEABLE fails instead of moving
    return realloc(block, size != 0 ? size : 1);
}

void *heap_free(void *block)
{
    free(block);
    return nullptr;
}

bool heap_validate()
{
    return true;
}

void *memory_reserve(void *address, uint32_t size)
{
    if (address == nullptr) {
        void *block = calloc(1, size);

        if (block == nullptr) {
            set_last_error(posix::k_error_not_enough_memory);
        }
        return block;
    }
#if defined(__EMSCRIPTEN__)
    {
        uintptr_t start = reinterpret_cast<uintptr_t>(address);
        uintptr_t end = start + size;
        uintptr_t brk = reinterpret_cast<uintptr_t>(sbrk(0));

        if (brk > start || sbrk(static_cast<intptr_t>(end - brk)) == reinterpret_cast<void *>(-1)) {
            set_last_error(posix::k_error_not_enough_memory);
            return nullptr;
        }
        memset(address, 0, size);
        return address;
    }
#else
    {
        void *mapped = mmap(address, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);

        if (mapped == MAP_FAILED || mapped != address) {
            set_last_error(posix::k_error_not_enough_memory);
            return nullptr;
        }
        return mapped;
    }
#endif
}

bool memory_protect(void *address, uint32_t size, uint32_t protection, uint32_t *old_protection)
{
    if (old_protection != nullptr) {
        *old_protection = 4;  // PAGE_READWRITE: everything this platform hands out
    }
#if defined(__EMSCRIPTEN__)
    (void)address;
    (void)size;
    (void)protection;
    return true;  // linear memory has no page protection
#else
    int native = protection == 0x02 ? PROT_READ : protection == 0x01 ? PROT_NONE : PROT_READ | PROT_WRITE;  // NOACCESS, READONLY

    return mprotect(address, size, native) == 0;
#endif
}

}  // namespace halo::platform
