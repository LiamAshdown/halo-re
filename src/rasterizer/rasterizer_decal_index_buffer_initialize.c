// rasterizer_decal_index_buffer_initialize  (Ghidra: FUN_0051bb90, unnamed; named per
// types/rasterizer.h's rasterizer_dynamic_vertex_cache note, which already refers to this
// function by this name)
// address 0x51bb90, size 284 bytes
// name confidence: 0.5   rewrite confidence: 0.5
// evidence: CreateIndexBuffer (device vtable +0x6c) for exactly
//   k_rasterizer_dynamic_index_buffer_size (0x30000) bytes into rasterizer_dynamic_index_buffer,
//   then sets capacity/buffer_handle for all 20 rasterizer_dynamic_vertex_caches entries
//   (0x006d98e8, stride 0xc matches the struct) from a fixed per vertex type capacity table
//   (0x800 for type 4, 0x2000 for 6/15, 2 for 7, 0x4000 for 8, 0 otherwise), matching the type
//   header's own note for this exact function.
// register convention: none -- no parameters.
// UNSURE: rasterizer_vertex_buffer_slot_allocate (the vertex buffer slot allocator) is called here with no visible
//   arguments; almost certainly (vertex_type, byte_length) but not confirmed at this call site.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

extern uint8_t rasterizer_software_vertex_processing; // 0x0069c680
extern void *rasterizer_device;                       // 0x0071d174
extern void *rasterizer_dynamic_index_buffer;          // 0x006e09e8
extern rasterizer_dynamic_vertex_cache rasterizer_dynamic_vertex_caches[k_rasterizer_vertex_type_count]; // 0x006d98e8

extern int32_t rasterizer_vertex_buffer_slot_allocate(void); // 0x5305f0, UNSURE: arguments not resolved here

typedef int32_t (*d3d_create_index_buffer_fn)(void *device, uint32_t length, uint32_t usage, uint32_t format,
                                                uint32_t pool, void **out_buffer, uint32_t shared_handle);

// Creates the shared decal dynamic index buffer and allocates a per vertex type geometry
// sub-cache sized according to a fixed type table.
uint32_t rasterizer_decal_index_buffer_initialize(void)
{
    uint32_t usage = (-(uint32_t)(rasterizer_software_vertex_processing != 0) & 0x10) | 0x208;
    void **vtable = *(void ***)rasterizer_device;
    int32_t hr = ((d3d_create_index_buffer_fn)vtable[0x6c / 4])(rasterizer_device, 0x30000, usage,
                                                                   0x65, 2, &rasterizer_dynamic_index_buffer, 0);
    uint8_t ok;
    int32_t i;

    if (hr < 0) {
        usage = 0;
    }
    ok = (hr >= 0);
    if (rasterizer_dynamic_index_buffer == (void *)0) {
        usage = 0;
        ok = 0;
    } else if (!ok) {
        rasterizer_dynamic_index_buffer = (void *)0;
    }

    for (i = 0; i < k_rasterizer_vertex_type_count && ok; i++) {
        int32_t capacity;
        switch (i) {
        case 4: capacity = 0x800; break;
        case 6: case 0xf: capacity = 0x2000; break;
        case 7: capacity = 2; break;
        case 8: capacity = 0x4000; break;
        default:
            capacity = 0;
            rasterizer_dynamic_vertex_caches[i].buffer_handle = 0;
            rasterizer_dynamic_vertex_caches[i].capacity = capacity;
            continue;
        }
        {
            int32_t handle = rasterizer_vertex_buffer_slot_allocate(); // UNSURE: arguments
            rasterizer_dynamic_vertex_caches[i].buffer_handle = handle;
            if (handle == 0) {
                usage = 0;
            }
            ok = (uint8_t)(usage >> 0x18);
        }
        rasterizer_dynamic_vertex_caches[i].capacity = capacity;
    }

    return ok;
}

#if 0
Original Ghidra decompilation (0x51bb90):

undefined4 FUN_0051bb90(void)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  char cVar5;
  undefined4 uVar6;

  uVar1 = -(uint)(DAT_0069c680 != '\0') & 0x10 | 0x208;
  iVar2 = (**(code **)(*DAT_0071d174 + 0x6c))(DAT_0071d174,0x30000,uVar1,0x65,2,&DAT_006e09e8,0);
  if (iVar2 < 0) {
    uVar1 = 0;
  }
  cVar5 = iVar2 >= 0;
  if (DAT_006e09e8 == 0) {
    uVar1 = 0;
    cVar5 = false;
  }
  else if ((bool)cVar5) goto LAB_0051bbf4;
  DAT_006e09e8 = 0;
LAB_0051bbf4:
  iVar2 = 0;
  do {
    if ((cVar5 == '\0') || (0x13 < (short)iVar2)) {
      return CONCAT31((int3)((uint)iVar2 >> 8),cVar5);
    }
    iVar3 = (int)(short)iVar2;
    switch(iVar3) {
    case 4:
      uVar6 = 0x800;
      break;
    default:
      uVar6 = 0;
      (&DAT_006d98f0)[iVar3 * 3] = 0;
      goto LAB_0051bc61;
    case 6:
    case 0xf:
      uVar6 = 0x2000;
      break;
    case 7:
      uVar6 = 2;
      break;
    case 8:
      uVar6 = 0x4000;
    }
    iVar4 = FUN_005305f0();
    (&DAT_006d98f0)[iVar3 * 3] = iVar4;
    if (iVar4 == 0) {
      uVar1 = 0;
    }
    cVar5 = (char)(uVar1 >> 0x18);
LAB_0051bc61:
    iVar2 = iVar2 + 1;
    (&DAT_006d98ec)[iVar3 * 3] = uVar6;
  } while( true );
}
#endif
