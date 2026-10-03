/**
 * @file src/memory/memory_c_api.cpp
 * The two memory functions the vendored C code in src/gamespy calls (declared in src/gamespy/gt2.h): C-linkage
 * forwards to halo::memory. Nothing else in the engine uses them.
 */

#include "halo/memory/api.hpp"

extern "C" void datum_index_invalidate(void *index)
{
    halo::memory::datum_index_invalidate(static_cast<datum_index *>(index));
}

extern "C" void crc32_update(uint32_t *crc, const void *data, int32_t length)
{
    halo::memory::crc32_update(crc, data, length);
}
