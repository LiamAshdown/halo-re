// rasterizer_dynamic_vertex_draw  (Ghidra: FUN_0051bec0)
// address 0x51bec0, size 455 bytes
// name confidence: 0.35   rewrite confidence: 0.9
// evidence: non-indexed draw of the vertices of rasterizer_dynamic_vertex_slots[slot]
//   (0x006d99d8) out of the per type cache buffer. primitive_kind selects the primitive: 2
//   lines (D3DPT_LINELIST, 2 vertices each), 3 triangles (TRIANGLELIST), 4 quads, and any other
//   value k a single strip of k vertices (TRIANGLESTRIP, k - 2 primitives; the count argument is
//   overwritten with k - 2). Lines, triangles and strips go through DrawPrimitive (+0x144) with
//   start vertex slot.first_vertex + k * first_primitive in chunks of 10000 primitives. Quads
//   reserve 2 * count dynamic indices (0x51bd60), fill them through the locked slot
//   (rasterizer_dynamic_index_slot_lock) as {4q, 4q+1, 4q+2, 4q, 4q+2, 4q+3} per quad, unlock the shared index
//   buffer (+0x30) and hand off to rasterizer_dynamic_index_cache_draw 0x51c090 with
//   first primitive 0.
//   Spot-check fix (phase 4 review): rewritten from the raw code 0x51bec0..0x51c086; the earlier
//   rewrite read the first primitive and the primitive type as live-in EBX/EDI values.
// register convention: __cdecl, (first_primitive, primitive_count, dynamic_vertex_slot,
//   primitive_kind as int16) on the stack.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

extern void *rasterizer_device;                                     // 0x0071d174
extern void debug_fp_draw_state_note(const char *site, int32_t hresult, uint32_t primitive_type,
    uint32_t vertex_count, uint32_t primitive_count); // TEMPORARY first-person diagnostics
extern uint8_t rasterizer_software_vertex_processing;               // 0x0069c680
extern rasterizer_vertex_declaration rasterizer_vertex_declarations[k_rasterizer_vertex_type_count]; // 0x006e1a90
extern int16_t rasterizer_vertex_sizes[k_rasterizer_vertex_type_count]; // 0x0065de00
extern rasterizer_dynamic_vertex_cache rasterizer_dynamic_vertex_caches[k_rasterizer_vertex_type_count]; // 0x006d98e8
extern rasterizer_dynamic_vertex_slot rasterizer_dynamic_vertex_slots[k_rasterizer_dynamic_vertex_slots]; // 0x006d99d8
extern rasterizer_vertex_buffer_slot rasterizer_vertex_buffer_slots[k_rasterizer_vertex_buffer_slots]; // 0x007bf060
extern void *rasterizer_dynamic_index_buffer;                       // 0x006e09e8

// blam-cc: EDX -> index_count
extern int32_t rasterizer_dynamic_index_cache_reserve(int32_t index_count);  // 0x51bd60
// blam-cc: ECX -> dynamic_index_slot; locks the slot range and returns its first index
extern uint16_t *rasterizer_dynamic_index_slot_lock(int32_t dynamic_index_slot);          // 0x511e80, render module
extern void rasterizer_dynamic_index_cache_draw(int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count,
                                                int32_t dynamic_vertex_slot); // 0x51c090

typedef int32_t (__stdcall *d3d_call0_fn)(void *self);
typedef int32_t (__stdcall *d3d_call1_fn)(void *self, uint32_t a);
typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);
typedef int32_t (__stdcall *d3d_set_stream_source_fn)(void *self, uint32_t stream, void *buffer, uint32_t offset, uint32_t stride);

static void **device_vtable(void)
{
    return *(void ***)rasterizer_device;
}

void rasterizer_dynamic_vertex_draw(int32_t first_primitive, int32_t primitive_count, int32_t dynamic_vertex_slot,
                                    int16_t primitive_kind)
{
    while (primitive_count > 0 && dynamic_vertex_slot != -1) {
        rasterizer_dynamic_vertex_slot *vertex_slot;
        uint32_t primitive_type;
        int16_t type;
        uint32_t stride;
        int32_t chunk;
        int32_t handle;
        void *buffer;

        switch (primitive_kind) {
        case 2:
            primitive_type = 2;                                 // D3DPT_LINELIST
            break;
        case 3:
            primitive_type = 4;                                 // D3DPT_TRIANGLELIST
            break;
        case 4: {
            // quads: build the two triangles of each quad in the dynamic index buffer
            int32_t triangle_count = primitive_count * 2;
            int32_t index_slot = rasterizer_dynamic_index_cache_reserve(triangle_count);
            uint16_t *indices;
            int16_t triangle;

            if (index_slot == -1) {
                return;                                         // note: no SetSoftwareVertexProcessing reset
            }
            indices = rasterizer_dynamic_index_slot_lock(index_slot);
            for (triangle = 0; triangle < triangle_count; triangle += 2) {
                uint16_t base = (uint16_t)((triangle / 2) * 4);
                uint16_t *quad = indices + triangle * 3;

                quad[0] = base;
                quad[1] = (uint16_t)(base + 1);
                quad[2] = (uint16_t)(base + 2);
                quad[3] = base;
                quad[4] = (uint16_t)(base + 2);
                quad[5] = (uint16_t)(base + 3);
            }
            ((d3d_call0_fn)(*(void ***)rasterizer_dynamic_index_buffer)[0x30 / 4])(rasterizer_dynamic_index_buffer); // Unlock
            rasterizer_dynamic_index_cache_draw(index_slot, 0, triangle_count, dynamic_vertex_slot);
            return;
        }
        default:
            primitive_count = primitive_kind - 2;               // one strip of primitive_kind vertices
            primitive_type = 5;                                 // D3DPT_TRIANGLESTRIP
            break;
        }

        vertex_slot = &rasterizer_dynamic_vertex_slots[dynamic_vertex_slot];
        type = vertex_slot->vertex_type;
        stride = (uint32_t)(int32_t)rasterizer_vertex_sizes[type];
        chunk = primitive_count > k_rasterizer_draw_chunk_size ? k_rasterizer_draw_chunk_size : primitive_count;

        ((d3d_call1_fn)device_vtable()[0x134 / 4])(rasterizer_device, ((rasterizer_software_vertex_processing != 0 ? 0x10 : 0) |
                                                                       rasterizer_vertex_declarations[type].usage) & 0x10);
        handle = rasterizer_dynamic_vertex_caches[type].buffer_handle;
        buffer = handle == 0 ? 0 : (void *)rasterizer_vertex_buffer_slots[handle - 1].hardware_buffer;
        ((d3d_set_stream_source_fn)device_vtable()[0x190 / 4])(rasterizer_device, 0, buffer, 0, stride);
        {
            int32_t debug_hr = ((d3d_call3_fn)device_vtable()[0x144 / 4])(rasterizer_device, primitive_type,
                                                   (uint32_t)(primitive_kind * first_primitive + vertex_slot->first_vertex),
                                                   (uint32_t)chunk);
            debug_fp_draw_state_note("vd", debug_hr, 0, 0, (uint32_t)chunk); // TEMPORARY
        }        // DrawPrimitive
        primitive_count -= chunk;
        first_primitive += chunk;
    }
    ((d3d_call1_fn)device_vtable()[0x134 / 4])(rasterizer_device, rasterizer_software_vertex_processing);
}

#if 0
Original Ghidra decompilation (0x51bec0):

void FUN_0051bec0(undefined4 param_1,int param_2,int param_3,short param_4)

{
  short *psVar1;
  short sVar2;
  undefined4 unaff_EBX;
  int iVar3;
  int iVar4;
  short sVar5;
  int unaff_EDI;
  int iVar6;
  undefined4 uVar7;
  undefined4 uVar8;

  while ((0 < param_2 && (param_3 != -1))) {
    iVar6 = (int)param_4;
    if ((iVar6 != 2) && (iVar6 != 3)) {
      if (iVar6 == 4) {
        param_2 = param_2 * 2;
        iVar6 = FUN_0051bd60();
        if (iVar6 == -1) {
          return;
        }
        iVar3 = FUN_00511e80();
        sVar5 = 0;
        if (0 < param_2) {
          iVar4 = 0;
          do {
            sVar2 = (short)(iVar4 / 2) * 4;
            psVar1 = (short *)(iVar3 + iVar4 * 6);
            sVar5 = sVar5 + 2;
            iVar4 = (int)sVar5;
            psVar1[1] = sVar2 + 1;
            *psVar1 = sVar2;
            psVar1[3] = sVar2;
            psVar1[2] = sVar2 + 2;
            psVar1[4] = sVar2 + 2;
            psVar1[5] = sVar2 + 3;
          } while (iVar4 < param_2);
        }
        (**(code **)(*DAT_006e09e8 + 0x30))(DAT_006e09e8);
        FUN_0051c090(iVar6,0,param_2,iVar6);
        return;
      }
      param_2 = iVar6 + -2;
    }
    sVar5 = *(short *)(&DAT_006d99d8 + param_3 * 0x10);
    iVar3 = param_2;
    if (10000 < param_2) {
      iVar3 = 10000;
    }
    (**(code **)(*DAT_0071d174 + 0x134))
              (DAT_0071d174,-(uint)(DAT_0069c680 != '\0') & 0x10 | (&DAT_006e1a98)[sVar5 * 3] & 0x10
              );
    if ((&DAT_006d98f0)[sVar5 * 3] == 0) {
      uVar7 = 0;
    }
    else {
      uVar7 = *(undefined4 *)(&DAT_007bf04c + (&DAT_006d98f0)[sVar5 * 3] * 10);
    }
    uVar8 = unaff_EBX;
    (**(code **)(*DAT_0071d174 + 400))(DAT_0071d174,0,uVar7,0,unaff_EBX);
    (**(code **)(*DAT_0071d174 + 0x144))
              (DAT_0071d174,uVar8,iVar6 * unaff_EDI + *(int *)(&DAT_006d99dc + param_3 * 0x10),iVar3
              );
    param_2 = param_2 - iVar3;
  }
  (**(code **)(*DAT_0071d174 + 0x134))(DAT_0071d174,DAT_0069c680);
  return;
}
#endif
