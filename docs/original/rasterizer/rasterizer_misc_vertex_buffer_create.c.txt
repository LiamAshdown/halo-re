// rasterizer_misc_vertex_buffer_create  (Ghidra: FUN_00534e50)
// address 0x534e50, size 296 bytes
// name confidence: 0.45   rewrite confidence: 0.6
// evidence: functions.md summary ("Creates and zero-initializes a shared 64KB dynamic vertex
//   buffer used elsewhere in the rasterizer for miscellaneous small draws"); matches
//   types/rasterizer.h's documented global "0x0071d270: void *rasterizer_misc_vertex_buffer 0x10000
//   bytes, 0x534e50" exactly. CreateVertexBuffer (device +0x68) and Lock/Unlock (buffer vtable
//   +0x2c/+0x30) follow the same signatures established elsewhere in this module.
// register convention: none -- __cdecl, no arguments.
// Phase 4 review against the raw code (0x534e50..0x534f77): FVF 0, Pool = (usage & 0x10 or
//   usage & 0x200) ? SYSTEMMEM (2) : MANAGED (1), the same rule as rasterizer_dx9_create_vertex_buffer
//   0x530570; the usage word is declarations[16].usage (0x006e1b58). Only the first 0x2000 of the
//   0x10000 bytes are zeroed, exactly as the original does.
// UNSURE: when creation fails the original does not return; it clears its result and goes on to
//   Lock through the null pointer. The early return below avoids modelling that crash.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void *rasterizer_device; // 0x0071d174
extern uint8_t rasterizer_software_vertex_processing; // 0x0069c680
extern uint32_t unknown_006e1b58; // 0x006e1b58 rasterizer_vertex_declarations[16].usage
extern void *rasterizer_misc_vertex_buffer; // 0x0071d270
extern int16_t rasterizer_vertex_buffer_lock_state; // 0x0069c632

typedef int32_t (__stdcall *d3d_create_vertex_buffer_fn)(void *device, uint32_t length, uint32_t usage, uint32_t fvf,
                                               uint32_t pool, void **out_buffer, void *shared_handle);
typedef int32_t (__stdcall *d3d_lock_fn)(void *self, uint32_t offset, uint32_t size, void **out_data, uint32_t flags);
typedef int32_t (__stdcall *d3d_call0_fn)(void *self);

// Creates and zero-initializes a shared 64KB dynamic vertex buffer used elsewhere in the
// rasterizer for miscellaneous small draws.
uint8_t rasterizer_misc_vertex_buffer_create(void)
{
    void **vt;
    uint32_t sw_flag;
    uint32_t usage;
    int32_t hr;
    void *buffer;
    void *data;
    uint32_t *cursor;
    int32_t count;

    sw_flag = (rasterizer_software_vertex_processing != 0) ? 0x10u : 0u;
    usage = sw_flag | unknown_006e1b58;

    buffer = 0;
    vt = *(void ***)rasterizer_device;
    hr = ((d3d_create_vertex_buffer_fn)vt[0x68 / 4])(rasterizer_device, 0x10000, usage, 0,
                                                     (usage & 0x210) != 0 ? 2 : 1, &buffer, 0);
    rasterizer_misc_vertex_buffer = (hr >= 0) ? buffer : 0;
    if (rasterizer_misc_vertex_buffer == 0) {
        return 0;
    }

    data = 0;
    rasterizer_vertex_buffer_lock_state = 2;
    vt = *(void ***)rasterizer_misc_vertex_buffer;
    hr = ((d3d_lock_fn)vt[0x2c / 4])(rasterizer_misc_vertex_buffer, 0, 0x10000, &data, 0);
    rasterizer_vertex_buffer_lock_state = 0;

    if (hr >= 0 && data != 0) {
        cursor = (uint32_t *)data;
        for (count = 0x400; count != 0; count--) {
            cursor[0] = 0;
            cursor[1] = 0;
            cursor += 2;
        }
        vt = *(void ***)rasterizer_misc_vertex_buffer;
        hr = ((d3d_call0_fn)vt[0x30 / 4])(rasterizer_misc_vertex_buffer); // Unlock
        if (hr >= 0) {
            return 1;
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x534e50):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

bool FUN_00534e50(void)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  bool bVar4;
  undefined4 *puVar5;
  undefined4 uStack_44;
  undefined4 *puStack_40;
  undefined4 uStack_3c;
  undefined4 local_24 [9];

  local_24[0] = 0;
  uVar1 = -(uint)(DAT_0069c680 != '\0') & 0x10;
  if ((uVar1 != 0 || (DAT_006e1b58 & 0x10) != 0) || (uStack_44 = 1, (DAT_006e1b58 & 0x200) != 0)) {
    uStack_44 = 2;
  }
  uStack_3c = 0;
  puStack_40 = local_24;
  iVar2 = (**(code **)(*DAT_0071d174 + 0x68))(DAT_0071d174,0x10000,uVar1 | DAT_006e1b58,0);
  DAT_0071d270 = (int *)((iVar2 < 0) - 1 & (uint)puStack_40);
  bVar4 = DAT_0071d270 != (int *)0x0;
  puVar5 = &uStack_44;
  _DAT_0069c632 = 2;
  iVar2 = (**(code **)(*DAT_0071d270 + 0x2c))(DAT_0071d270,0,0x10000,puVar5,0);
  _DAT_0069c632 = 0;
  if ((-1 < iVar2 && bVar4) && (puVar5 != (undefined4 *)0x0)) {
    uStack_44 = 0x3f800000;
    puStack_40 = (undefined4 *)0x3f800000;
    uStack_3c = 0x3f800000;
    iVar3 = 0x400;
    do {
      *puVar5 = 0;
      puVar5[1] = 0;
      puVar5 = puVar5 + 2;
      iVar3 = iVar3 + -1;
    } while (iVar3 != 0);
    iVar3 = (**(code **)(*DAT_0071d270 + 0x30))(DAT_0071d270);
    if (-1 < iVar3) {
      return -1 < iVar2 && bVar4;
    }
  }
  return false;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
