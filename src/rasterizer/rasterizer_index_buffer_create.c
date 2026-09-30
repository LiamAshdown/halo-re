// rasterizer_index_buffer_create  (Ghidra: already named)
// address 0x525030, size 256 bytes
// name confidence: 0.6   rewrite confidence: 0.95
// evidence: raw disassembly (phase 4 review; the earlier file guessed the pool and put the
//   output in ECX and the source in EDI). The output record is the first stack argument, the
//   source indices the second; count arrives in EAX and the TriangleBufferType in DX. Byte size:
//   type 0 (list) count * 6, type 1 (strip) count * 2 + 4, otherwise 0. The usage is the
//   vertex declaration usage of entry `type` (the binary indexes the declaration table with the
//   index buffer type) ORed with 0x10 under software vertex processing, and the pool is
//   D3DPOOL_SYSTEMMEM (2) when that usage has the software processing bit, else MANAGED (1).
// What it does: stores type and count, creates the INDEX16 buffer, locks it, copies the source
//   indices, unlocks, and on success stores the source pointer at +8 and the buffer at +0xc.
//   Any failure before the copy clears the whole record. Returns 1 on success (and when there is
//   no device yet), 0 otherwise.
// register convention: EAX -> count, DX -> type, stack -> (out, source).
// blam-cc: EAX -> count, DX -> type, stack -> (out, source)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "fn_rasterizer.h"
#include <stdint.h> // uintptr_t
#include <string.h> // memcpy (rep movsd / rep movsb)

extern void *rasterizer_device;                                        // 0x0071d174
extern uint8_t rasterizer_software_vertex_processing;                  // 0x0069c680
extern rasterizer_vertex_declaration rasterizer_vertex_declarations[k_rasterizer_vertex_type_count]; // 0x006e1a90

typedef int32_t (__stdcall *d3d_create_buffer_fn)(void *self, uint32_t length, uint32_t usage, uint32_t format,
                                        uint32_t pool, void **buffer, void *shared);
typedef int32_t (__stdcall *d3d_lock_fn)(void *self, uint32_t offset, uint32_t size, void **data, uint32_t flags);
typedef int32_t (__stdcall *d3d_unlock_fn)(void *self);

uint8_t rasterizer_index_buffer_create(int32_t count, int16_t type, rasterizer_index_buffer *out, const void *source)
{
    uint32_t byte_size = 0;
    uint8_t ok = 1;
    void *buffer;
    void *data;
    uint32_t usage;

    if (type == 0) {
        byte_size = (uint32_t)count * 6;
    } else if (type == 1) {
        byte_size = (uint32_t)count * 2 + 4;
    }
    out->type = type;
    out->count = count;
    if (rasterizer_device == NULL) {
        return ok;
    }
    usage = (rasterizer_software_vertex_processing ? 0x10 : 0) | rasterizer_vertex_declarations[type].usage;
    if (((d3d_create_buffer_fn)(*(void ***)rasterizer_device)[0x6c / 4])(rasterizer_device, byte_size, usage,
                                                                         0x65 /* D3DFMT_INDEX16 */,
                                                                         (usage & 0x10) ? 2 : 1, &buffer, NULL) < 0) {
        ok = 0;
    }
    if (buffer != NULL && ok) {
        if (((d3d_lock_fn)(*(void ***)buffer)[0x2c / 4])(buffer, 0, byte_size, &data, 0) < 0) {
            ok = 0;
        }
        if (data != NULL && ok) {
            memcpy(data, source, byte_size);
            if (((d3d_unlock_fn)(*(void ***)buffer)[0x30 / 4])(buffer) < 0) {
                ok = 0;
            }
            out->hardware_buffer = (uint32_t)(uintptr_t)buffer;
            out->data = (uint32_t)(uintptr_t)source;
            return ok;
        }
    }
    ok = 0;
    out->type = 0;
    out->unknown_02 = 0;
    out->count = 0;
    out->data = 0;
    out->hardware_buffer = 0;
    return ok;
}

#if 0
Original Ghidra decompilation (0x525030):

char rasterizer_index_buffer_create(short *param_1)

{
  short *psVar1;
  int in_EAX;
  int iVar2;
  uint uVar3;
  int *piVar4;
  short in_DX;
  char cVar5;
  uint uVar6;
  int *piVar7;
  int *unaff_EDI;
  int *piVar8;
  undefined4 *puStack_20;
  undefined4 uStack_1c;
  
  psVar1 = param_1;
  iVar2 = (int)in_DX;
  uVar6 = 0;
  cVar5 = '\x01';
  if (iVar2 == 0) {
    uVar6 = in_EAX * 6;
  }
  else if (iVar2 == 1) {
    uVar6 = in_EAX * 2 + 4;
  }
  *param_1 = in_DX;
  *(int *)(param_1 + 2) = in_EAX;
  piVar8 = DAT_0071d174;
  if (DAT_0071d174 != (int *)0x0) {
    uStack_1c = 0;
    uVar3 = -(uint)(DAT_0069c680 != '\0') & 0x10;
    piVar4 = (int *)(uVar3 | (&DAT_006e1a98)[iVar2 * 3]);
    puStack_20 = &param_1;
    cVar5 = (uVar3 != 0 || ((&DAT_006e1a98)[iVar2 * 3] & 0x10) != 0) + '\x01';
    iVar2 = (**(code **)(*DAT_0071d174 + 0x6c))();
    if (iVar2 < 0) {
      cVar5 = '\0';
    }
    if ((unaff_EDI != (int *)0x0) && (cVar5 != '\0')) {
      iVar2 = (**(code **)(*unaff_EDI + 0x2c))(unaff_EDI,0,uVar6,&puStack_20,0);
      if (iVar2 < 0) {
        cVar5 = '\0';
      }
      if ((piVar8 != (int *)0x0) && (cVar5 != '\0')) {
        piVar7 = (int *)0x65;
        for (uVar3 = uVar6 >> 2; uVar3 != 0; uVar3 = uVar3 - 1) {
          *piVar8 = *piVar7;
          piVar7 = piVar7 + 1;
          piVar8 = piVar8 + 1;
        }
        for (uVar3 = uVar6 & 3; uVar3 != 0; uVar3 = uVar3 - 1) {
          *(char *)piVar8 = (char)*piVar7;
          piVar7 = (int *)((int)piVar7 + 1);
          piVar8 = (int *)((int)piVar8 + 1);
        }
        iVar2 = (**(code **)(*piVar4 + 0x30))(piVar4);
        if (iVar2 < 0) {
          cVar5 = '\0';
        }
        *(uint *)(psVar1 + 6) = uVar6;
        *(int **)(psVar1 + 4) = piVar4;
        return cVar5;
      }
    }
    cVar5 = '\0';
    psVar1[0] = 0;
    psVar1[1] = 0;
    psVar1[2] = 0;
    psVar1[3] = 0;
    psVar1[4] = 0;
    psVar1[5] = 0;
    psVar1[6] = 0;
    psVar1[7] = 0;
  }
  return cVar5;
}
#endif
