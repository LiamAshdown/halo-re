// rasterizer_lens_flare_batch_draw_slot  (Ghidra: FUN_00536c10)
// address 0x536c10, size 109 bytes
// name confidence: 0.45   rewrite confidence: 0.7
// evidence: functions.md summary ("Draws and clears one batched vertex-quad slot of the
//   screen-space sprite (lens-flare/decal) rendering system"); the indexed globals
//   (0x0075efc0/c4/d4, stride 0x6006 dwords = 0x18018 bytes) match lens_flare_batch's
//   vertex_count/key/last_used fields exactly against the base at 0x00746fc0.
// register convention: EAX -> batch_index.
// blam-cc: EAX -> batch_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "fn_rasterizer.h"

extern lens_flare_batch lens_flare_batches[k_lens_flare_batch_slots]; // 0x00746fc0
extern void *rasterizer_device; // 0x0071d174


typedef int32_t (__stdcall *d3d_draw_primitive_up_fn)(void *self, uint32_t primitive_type, uint32_t primitive_count,
                                            const void *data, uint32_t stride);

// Draws and clears one batched vertex-quad slot of the screen-space sprite (lens-flare/decal)
// rendering system, if it has any vertices queued.
void rasterizer_lens_flare_batch_draw_slot(int32_t batch_index)
{
    lens_flare_batch *batch = &lens_flare_batches[batch_index];

    if (batch->vertex_count != 0) {
        if (rasterizer_lens_flare_batch_apply_material(&batch->key) != 0) {
            void **vt = *(void ***)rasterizer_device;
            ((d3d_draw_primitive_up_fn)vt[0x14c / 4])(rasterizer_device, 4, (uint32_t)batch->vertex_count / 3,
                                                      batch->vertices, 0x20);
        }
    }
    batch->last_used = 0;
    batch->vertex_count = 0;
}

#if 0
Original Ghidra decompilation (0x536c10):

void FUN_00536c10(void)

{
  char cVar1;
  int in_EAX;

  if ((&DAT_0075efc0)[in_EAX * 0x6006] != 0) {
    cVar1 = FUN_00536b70();
    if (cVar1 != '\0') {
      (**(code **)(*DAT_0071d174 + 0x14c))
                (DAT_0071d174,4,(uint)(&DAT_0075efc0)[in_EAX * 0x6006] / 3,
                 &DAT_00746fc0 + in_EAX * 0x18018,0x20);
    }
    (&DAT_0075efd4)[in_EAX * 0x6006] = 0;
    (&DAT_0075efc0)[in_EAX * 0x6006] = 0;
    return;
  }
  (&DAT_0075efd4)[in_EAX * 0x6006] = 0;
  (&DAT_0075efc0)[in_EAX * 0x6006] = 0;
  return;
}
#endif
