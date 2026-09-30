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
// FIXED (first-boot track, objdump 0x51bb90..0x51bcc0): the allocator gets EBX vertex type, ESI the type's
//   declaration FVF (+4 of the 0x006e1a90 entry), EDI vertex size (0x0065de00) * capacity; the result is the
//   capacity switch's success flag, not a shift of the usage word. Historical note: rasterizer_vertex_buffer_slot_allocate (the vertex buffer slot allocator) is called here with no visible
//   arguments; almost certainly (vertex_type, byte_length) but not confirmed at this call site.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "fn_rasterizer.h"

extern uint8_t rasterizer_software_vertex_processing; // 0x0069c680
extern void *rasterizer_device;                       // 0x0071d174
extern void *rasterizer_dynamic_index_buffer;          // 0x006e09e8
extern rasterizer_dynamic_vertex_cache rasterizer_dynamic_vertex_caches[k_rasterizer_vertex_type_count]; // 0x006d98e8


    // blam-cc: EBX -> vertex_type, ESI -> fvf, EDI -> length
extern int16_t rasterizer_vertex_sizes[k_rasterizer_vertex_type_count]; // 0x0065de00
extern rasterizer_vertex_declaration rasterizer_vertex_declarations[k_rasterizer_vertex_type_count]; // 0x006e1a90

typedef int32_t (__stdcall *d3d_create_index_buffer_fn)(void *device, uint32_t length, uint32_t usage, uint32_t format,
                                                uint32_t pool, void **out_buffer, uint32_t shared_handle);

// Creates the shared dynamic index buffer (0x30000 bytes of D3DFMT_INDEX16, write-only | dynamic, software processing
// when enabled, D3DPOOL_SYSTEMMEM) and, while everything succeeds, one vertex buffer slot per vertex type that has a
// dynamic cache: vertex size * capacity bytes of that type's FVF. Returns whether all of it succeeded.
uint8_t rasterizer_decal_index_buffer_initialize(void)
{
    uint32_t usage = (rasterizer_software_vertex_processing != 0 ? 0x10u : 0u) | 0x208;
    void **vtable = *(void ***)rasterizer_device;
    uint8_t ok = 1;
    int32_t type;

    if (((d3d_create_index_buffer_fn)vtable[0x6c / 4])(rasterizer_device, 0x30000, usage, 0x65 /* D3DFMT_INDEX16 */,
                                                       2 /* D3DPOOL_SYSTEMMEM */, &rasterizer_dynamic_index_buffer,
                                                       0) < 0) {
        ok = 0;
    }
    if (rasterizer_dynamic_index_buffer == (void *)0 || !ok) {
        ok = 0;
        rasterizer_dynamic_index_buffer = (void *)0;
    }

    for (type = 0; ok && (int16_t)type < k_rasterizer_vertex_type_count; type++) {
        rasterizer_dynamic_vertex_cache *cache = &rasterizer_dynamic_vertex_caches[(int16_t)type];
        int32_t capacity;

        switch (type) {   // 0x51bc16: byte table 0x51bcc0 over types 4..15, jump table 0x51bcac
        case 4: capacity = 0x800; break;
        case 6: case 15: capacity = 0x2000; break;
        case 7: capacity = 2; break;
        case 8: capacity = 0x4000; break;
        default:
            cache->capacity = 0;
            cache->buffer_handle = 0;
            continue;
        }
        cache->buffer_handle = rasterizer_vertex_buffer_slot_allocate(type, rasterizer_vertex_declarations[type].fvf,
                                                                      rasterizer_vertex_sizes[type] * capacity);
        if (cache->buffer_handle == 0) {
            ok = 0;
        }
        cache->capacity = capacity;
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
