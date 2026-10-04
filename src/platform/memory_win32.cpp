/**
 * @file src/platform/memory_win32.cpp
 * halo::platform memory on Windows: the same Win32 calls the engine made directly.
 */

#include "halo/platform/memory.hpp"

#include "win32.h"

namespace halo::platform {

void *heap_allocate(uint32_t flags, uint32_t size)
{
    return GlobalAlloc(flags, size);
}

void *heap_reallocate(void *block, uint32_t size, uint32_t flags)
{
    return GlobalReAlloc(block, size, flags);
}

void *heap_free(void *block)
{
    return GlobalFree(block);
}

bool heap_validate()
{
    return HeapValidate(GetProcessHeap(), 0, nullptr) != 0;
}

void *memory_reserve(void *address, uint32_t size)
{
    return VirtualAlloc(address, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
}

bool memory_protect(void *address, uint32_t size, uint32_t protection, uint32_t *old_protection)
{
    return VirtualProtect(address, size, protection, reinterpret_cast<DWORD *>(old_protection)) != 0;
}

}  // namespace halo::platform
