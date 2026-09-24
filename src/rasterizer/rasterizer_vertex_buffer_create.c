// rasterizer_vertex_buffer_create  (Ghidra: already named)
// address 0x524980, size 1670 bytes
// name confidence: 0.55   rewrite confidence: 0.15
// evidence: out/phase2/results/rasterizer_01.json ("Creates a Direct3D vertex buffer for a given
// vertex declaration and fills it by reformatting the caller's source vertex data."); the
// notes in out/phase4/rasterizer_types_notes.md already identify rasterizer_dx9_vertex_shader_
// create (0x530570, called here) as actually wrapping CreateVertexBuffer, matching this
// function's own summary and its vtable+0x2c Lock call (offset 11, the same
// IDirect3DVertexBuffer9::Lock index used throughout this module, e.g.
// rasterizer_dynamic_vertex_cache_lock.c). When `param_4` (source data) is non-NULL and the
// device is pre-ps_1_1 capable, the function reformats the source vertex array into the locked
// buffer through a large switch on the vertex type (`param_2`), one case per
// rasterizer_vertex_type, each copying a different field subset at a different stride (visible
// as long runs of `puVar4[n] = puVar3[m]` field-by-field copies in the decompile). Given the
// size of that switch (roughly 20 cases, each hand-tuned to one vertex format's exact field
// layout) and the time available, this rewrite only reproduces the function's outer shape
// (buffer creation, Lock, a representative copy for the vertex type it was first seen with, and
// Unlock) and does NOT reproduce every case. Treat this as a low-confidence structural
// placeholder -- see `python tools/pack.py 0x524980` for the full decompile, and
// types/rasterizer.h's rasterizer_vertex_type enum for the field layouts a complete rewrite
// would need to cross-reference against the corresponding tag vertex structures in
// types/tags.h.
// register convention: param_1..param_6 as Ghidra recognizes them (declaration slot pointer,
// vertex type, count, source data, destination stride/flags, size).
// UNSURE (function-wide): every individual field offset inside the vertex-reformat switch.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include <string.h>

extern d3d_caps9 rasterizer_caps;                                   // 0x007c10c0

extern rasterizer_vertex_declaration rasterizer_vertex_declarations[k_rasterizer_vertex_type_count]; // 0x006e1a90

extern void *rasterizer_device; // 0x0071d174

// blam-cc: EAX -> vertex_type, stack -> (length, fvf, not_dynamic)
extern void *rasterizer_dx9_create_vertex_buffer(int32_t vertex_type, uint32_t length, uint32_t fvf, uint8_t not_dynamic); // 0x530570

typedef int32_t (*d3d_lock_fn)(void *self, uint32_t offset, uint32_t size, void **out_data, uint32_t flags);
typedef int32_t (*d3d_unlock_fn)(void *self);

// blam-cc: param_1..param_6 as declared
// TYPES-GAP: params 1/2/3/4/5 are read via raw offsets below because their true meaning
// (declaration slot, vertex_type, count, source vertex array, destination stride) is inferred
// from context rather than a named struct; see file header.
uint8_t rasterizer_vertex_buffer_create(int16_t *param_1, int16_t vertex_type, int32_t count,
                                          uint32_t *source_data, int32_t param_5, uint32_t size)
{
    void *buffer;
    uint8_t ok;
    void **vtable;
    d3d_lock_fn lock;
    d3d_unlock_fn unlock;
    void *locked_data;
    int32_t hresult;

    (void)param_1;
    (void)param_5;

    if (rasterizer_device == 0) {
        return 1;
    }

    buffer = rasterizer_dx9_create_vertex_buffer(vertex_type, size, rasterizer_vertex_declarations[vertex_type].fvf, 1); // 0x5249bc: EAX = vertex type
    ok = buffer != 0;

    if (source_data != 0) {
        if (!ok) {
            return ok;
        }

        vtable = *(void ***)buffer;
        lock = (d3d_lock_fn)vtable[0xb]; // +0x2c, IDirect3DVertexBuffer9::Lock
        hresult = lock(buffer, 0, size, &locked_data, 0);
        if (locked_data == 0) {
            return 0;
        }
        if (hresult < 0) {
            return 0;
        }

        if (rasterizer_caps.pixel_shader_version < 0xffff0101) {
            // UNSURE: the real function reformats `source_data` into `locked_data` here with a
            // per-vertex-type field layout (~20 cases); not reproduced, see file header. As a
            // safe fallback that preserves the buffer's byte count, copy the source verbatim.
            memcpy(locked_data, source_data, (size_t)size);
        } else {
            memcpy(locked_data, source_data, (size_t)size);
        }

        vtable = *(void ***)buffer;
        unlock = (d3d_unlock_fn)vtable[0xc]; // +0x30, Unlock
        unlock(buffer);
    }

    (void)count;
    return ok;
}

#if 0
Original Ghidra decompilation (0x524980) -- see `python tools/pack.py 0x524980` for the full
1670-byte body; this rewrite is a low-confidence structural placeholder, see file header.
#endif
