// rasterizer_effect_slot_release_active  (Ghidra: FUN_00512150; new name, evidence below)
// address 0x512150, size 56 bytes
// name confidence: 0.4   rewrite confidence: 0.65
// evidence: types/render.h globals list: "0x0071d278 the active rasterizer effect slot pointer
//   0x512150 releases (&effects[48])", cross-referenced against types/rasterizer.h's
//   rasterizer_effect_slot table at 0x0069d410 (122 entries; slot 48 matches "&effects[48]").
//   The vtable+0x108 call on *DAT_0071d278 releases the ID3DXEffect COM object (a plain
//   IUnknown::Release-style call taking only `this`); the closing vtable+0x164 call on the
//   device clears a cached device state slot.
// register convention: none (void).
// UNSURE: the exact D3D interface method at device vtable+0x164 is not identified; treated as a
//   generic (device, value) setter matching the render.h note "clears a device state slot".

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

extern void **rasterizer_effect_pool_scratch; // 0x0071d278 UNSURE: &effects[48], types/rasterizer.h rasterizer_effect_slot
extern void *rasterizer_device;              // 0x0071d174

extern void rasterizer_lens_flare_batch_flush_all(void); // 0x536c80

typedef int32_t (__stdcall *d3d_release_fn)(void *self);
typedef int32_t (__stdcall *d3d_clear_state_slot_fn)(void *device, uint32_t value);

// Flushes pending lens flare batches, releases and clears the active rasterizer effect slot's
// COM object pointer, and clears a device state slot (vtable+0x164).
void rasterizer_effect_slot_release_active(void)
{
    void **vtable;

    rasterizer_lens_flare_batch_flush_all();

    if (rasterizer_effect_pool_scratch != 0 && *rasterizer_effect_pool_scratch != 0) {
        void *effect = *rasterizer_effect_pool_scratch;
        vtable = *(void ***)effect;
        ((d3d_release_fn)vtable[0x108 / 4])(effect);
    }
    rasterizer_effect_pool_scratch = 0;

    vtable = *(void ***)rasterizer_device;
    ((d3d_clear_state_slot_fn)vtable[0x164 / 4])(rasterizer_device, 0);
}

#if 0
Original Ghidra decompilation (0x512150):

void FUN_00512150(void)

{
  int *piVar1;

  rasterizer_lens_flare_batch_flush_all();
  if ((DAT_0071d278 != (int *)0x0) && (piVar1 = (int *)*DAT_0071d278, piVar1 != (int *)0x0)) {
    (**(code **)(*piVar1 + 0x108))(piVar1);
  }
  DAT_0071d278 = (int *)0x0;
  (**(code **)(*DAT_0071d174 + 0x164))(DAT_0071d174,0);
  return;
}
#endif
