/**
 * @file include/halo/platform/memory.hpp
 * Heap blocks and page memory, independent of the operating system. The calls mirror the Win32 functions the engine
 * used (GlobalAlloc / GlobalReAlloc / GlobalFree, VirtualAlloc, VirtualProtect), keeping their flag values and results;
 * src/platform/memory_win32.cpp implements them with exactly those functions.
 */
#pragma once

#include <cstdint>

namespace halo::platform {

/** heap_allocate / heap_reallocate flags (GMEM_ZEROINIT, GMEM_MOVEABLE: reallocation may move the block). */
inline constexpr uint32_t k_heap_zero = 0x40;
inline constexpr uint32_t k_heap_moveable = 2;

/** A heap block of size bytes, zeroed with k_heap_zero (GlobalAlloc). Null on failure. */
void *heap_allocate(uint32_t flags, uint32_t size);
/** Resizes a heap block (GlobalReAlloc); null on failure, the block then unchanged. */
void *heap_reallocate(void *block, uint32_t size, uint32_t flags);
/** Frees a heap block; null on success, the block on failure (GlobalFree). */
void *heap_free(void *block);
/** Whether the heap is consistent (a debugging check; HeapValidate of the process heap). */
bool heap_validate();

/** Reserves and commits read/write pages, at address when it is not null (VirtualAlloc). Null on failure. */
void *memory_reserve(void *address, uint32_t size);
/** Changes page protection (VirtualProtect; the Win32 PAGE_ values), storing the previous one. */
bool memory_protect(void *address, uint32_t size, uint32_t protection, uint32_t *old_protection);

}  // namespace halo::platform
