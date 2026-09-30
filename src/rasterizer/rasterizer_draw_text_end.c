// rasterizer_draw_text_end  (Ghidra: FUN_00531e90; the phase 3 rewrite called it
//   rasterizer_first_person_model_render_restore)
// address 0x531e90, size 50 bytes
// name confidence: 0.5   rewrite confidence: 0.95
// evidence: raw disassembly (phase 4 review). Paired with rasterizer_draw_text_begin 0x531b80
//   by the same two chimera text draws. State 8 is D3DRS_FILLMODE: the begin call forces SOLID
//   (3) under the wireframe debug toggle 0x006893e6 and this puts WIREFRAME (2) back; it then
//   restores software vertex processing.
// register convention: none -- __cdecl, no arguments.

#include "tags.h"
#include "math.h"
#include "rasterizer.h"
#include "fn_rasterizer.h"

extern uint8_t console_debug_toggle_6893e6;   // 0x006893e6
extern void *rasterizer_device;               // 0x0071d174
extern uint8_t rasterizer_software_vertex_processing; // 0x0069c680

typedef int32_t (__stdcall *d3d_call1_fn)(void *self, uint32_t a);
typedef int32_t (__stdcall *d3d_call2_fn)(void *self, uint32_t a, uint32_t b);

void rasterizer_draw_text_end(void)
{
    void **vt = *(void ***)rasterizer_device;

    if (console_debug_toggle_6893e6 != 0) {
        ((d3d_call2_fn)vt[0xe4 / 4])(rasterizer_device, 8, 2);
    }
    ((d3d_call1_fn)vt[0x134 / 4])(rasterizer_device, rasterizer_software_vertex_processing);
}

#if 0
Original Ghidra decompilation (0x531e90):

void FUN_00531e90(void)

{
  if (DAT_006893e6 != '\0') {
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,8,2);
  }
  (**(code **)(*DAT_0071d174 + 0x134))(DAT_0071d174,DAT_0069c680);
  return;
}
#endif
