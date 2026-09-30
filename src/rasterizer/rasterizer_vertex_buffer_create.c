// rasterizer_vertex_buffer_create  (Ghidra: already named)
// address 0x524980, size 1670 bytes
// name confidence: 0.55   rewrite confidence: 0.9
// evidence: out/phase2/results/rasterizer_01.json ("Creates a Direct3D vertex buffer for a given vertex declaration and
//   fills it by reformatting the caller's source vertex data."); rasterizer_dx9_create_vertex_buffer (0x530570) wraps
//   CreateVertexBuffer; the Lock / Unlock calls are IDirect3DVertexBuffer9 vtable +0x2c / +0x30.
// REWRITTEN (first-boot track, objdump 0x524980..0x525008) and completed 2026-09-30: param_1 is the caller's vertex buffer
//   record. No device -> 1, record untouched. Otherwise the buffer is created (0x530570, static); with no source data the
//   result is whether that worked and the record is left alone (0x5249d2 -> 0x524fef); with source data it is locked,
//   filled, unlocked, and the record gets {type, count, 0, source, buffer}; any failure (create, lock, NULL lock pointer)
//   zeroes the record and returns 0. Unlock failure still fills the record (as the binary does) and then zeroes it.
//   Fill: on a pixel shader version >= ps_1_1 (0x007c118c >= 0xffff0101) the source is copied verbatim (size bytes). On older
//   devices the jump table at 0x0052500c repacks vertex types 12, 13, 14 and 19 (other types are copied verbatim):
//     12: 0x38-byte source vertices -> 0x20 bytes {src[0..0x18), src[0x30..0x38)}
//     13: 0x14-byte source vertices -> 0x08 bytes {src[0xc..0x14)}
//     14: 0x44-byte source vertices -> 0x20 bytes {src[0..0x18), src[0x30..0x38)}
//     19: 0x38-byte source vertices + the second stream (0x14-byte vertices) -> 0x28 bytes
//         {src[0..0x18), src[0x30..0x38), second[0xc..0x14)}
//   (the binary unrolls these four vertices at a time with a tail loop; here they are per-vertex loops.)
// register convention: plain stack arguments (record, vertex_type, count, source_data, second_stream, size), cdecl.
// VERIFIED against disassembly 0x524980..0x525008 (2026-09-30)

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

typedef int32_t (__stdcall *d3d_lock_fn)(void *self, uint32_t offset, uint32_t size, void **out_data, uint32_t flags);
typedef int32_t (__stdcall *d3d_unlock_fn)(void *self);

static void copy_dwords(uint8_t *dst, const uint8_t *src, uint32_t byte_count)
{
    memcpy(dst, src, byte_count);
}

// 0x524a3e..0x524fbc: fill the locked buffer from the source vertices.
static void rasterizer_vertex_buffer_fill(void *locked, int16_t vertex_type, int32_t count, const uint8_t *source,
                                          const uint8_t *second_stream, uint32_t size)
{
    uint8_t *dst = (uint8_t *)locked;
    int32_t i;

    if (rasterizer_caps.pixel_shader_version < 0xffff0101u) {
        switch (vertex_type) {
        case 12:
        case 14: {
            int32_t stride = (vertex_type == 12) ? 0x38 : 0x44;

            for (i = 0; i < count; i++) {
                copy_dwords(dst + i * 0x20, source + i * stride, 0x18);
                copy_dwords(dst + i * 0x20 + 0x18, source + i * stride + 0x30, 8);
            }
            return;
        }
        case 13:
            for (i = 0; i < count; i++) {
                copy_dwords(dst + i * 8, source + i * 0x14 + 0xc, 8);
            }
            return;
        case 19:
            for (i = 0; i < count; i++) {
                copy_dwords(dst + i * 0x28, source + i * 0x38, 0x18);
                copy_dwords(dst + i * 0x28 + 0x18, source + i * 0x38 + 0x30, 8);
                copy_dwords(dst + i * 0x28 + 0x20, second_stream + i * 0x14 + 0xc, 8);
            }
            return;
        default:
            break;
        }
    }
    memcpy(dst, source, (size_t)size);
}

// blam-cc: stack -> (record, vertex_type, count, source_data, second_stream, size)
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
            rasterizer_vertex_buffer_fill(locked_data, vertex_type, count, (const uint8_t *)source_data,
                                          (const uint8_t *)(uintptr_t)second_stream, size);
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
Original Ghidra decompilation: see `python tools/pack.py 0x524980` (the repack loops above were rewritten from the disassembly).
#endif
