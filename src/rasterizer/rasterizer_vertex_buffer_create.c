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
#include "fn_rasterizer.h"
#include <string.h>

extern d3d_caps9 rasterizer_caps;                                   // 0x007c10c0

extern rasterizer_vertex_declaration rasterizer_vertex_declarations[k_rasterizer_vertex_type_count]; // 0x006e1a90

extern void *rasterizer_device; // 0x0071d174

// blam-cc: EAX -> vertex_type, stack -> (length, fvf, not_dynamic)


typedef int32_t (__stdcall *d3d_lock_fn)(void *self, uint32_t offset, uint32_t size, void **out_data, uint32_t flags);
typedef int32_t (__stdcall *d3d_unlock_fn)(void *self);

// blam-cc: param_1..param_6 as declared
// TYPES-GAP: params 1/2/3/4/5 are read via raw offsets below because their true meaning
// (declaration slot, vertex_type, count, source vertex array, destination stride) is inferred
// from context rather than a named struct; see file header.
// REWRITTEN (first-boot track, objdump 0x524980..0x52500a): param_1 is the caller's vertex buffer record, which
//   the old version never filled (every model's hardware buffer pointer stayed garbage and the first draw crashed
//   inside Direct3D). Now: no device -> 1, record untouched. Otherwise the buffer is created (0x530570, static);
//   with no source data the result is whether that worked and the record is left alone (0x5249d2 -> 0x524fef);
//   with source data it is locked, filled, unlocked, and the record gets {type, count, 0, source, buffer}; any
//   failure (create, lock, NULL lock pointer, unlock) zeroes the record and returns 0.
//   NOT REPRODUCED (logged): on devices without ps_1_1 (0x007c118c < 0xffff0101) vertex types 12..19 are repacked
//   through the jump table at 0x0052500c into narrower layouts; this rewrite copies the source verbatim on every
//   device, which is only right for ps_1_1 and later (every device d3d9 still supports).
uint8_t rasterizer_vertex_buffer_create(rasterizer_vertex_buffer *record, int16_t vertex_type, int32_t count,
                                        uint32_t *source_data, int32_t second_stream, uint32_t size)
{
    void *buffer;
    uint8_t ok = 1;
    void *locked_data = 0;

    (void)second_stream;
    if (rasterizer_device == 0) {
        return 1;
    }
    buffer = rasterizer_dx9_create_vertex_buffer(vertex_type, size, rasterizer_vertex_declarations[vertex_type].fvf, 1);
    if (buffer == 0) {
        ok = 0;
    }
    if (source_data == 0) {
        if (ok) {
            return ok;
        }
    } else if (ok) {
        void **vtable = *(void ***)buffer;
        if (((d3d_lock_fn)vtable[0xb])(buffer, 0, size, &locked_data, 0) < 0) { // +0x2c Lock
            ok = 0;
        }
        if (locked_data == 0) {
            ok = 0;
        }
        if (ok) {
            memcpy(locked_data, source_data, (size_t)size);
            vtable = *(void ***)buffer;
            if (((d3d_unlock_fn)vtable[0xc])(buffer) < 0) { // +0x30 Unlock
                ok = 0;
            }
            record->type = vertex_type;
            record->count = count;
            *(uint32_t *)&record->unknown_08 = 0;
            record->data = (uint32_t)source_data;
            record->hardware_buffer = (uint32_t)buffer;
            if (ok) {
                return ok;
            }
        }
    }
    record->type = 0;
    record->unknown_02 = 0;
    record->count = 0;
    *(uint32_t *)&record->unknown_08 = 0;
    record->data = 0;
    record->hardware_buffer = 0;
    return ok;
}

#if 0
Original Ghidra decompilation (0x524980) -- see `python tools/pack.py 0x524980` for the full
1670-byte body; this rewrite is a low-confidence structural placeholder, see file header.
#endif
