// render_objects  (Ghidra: FUN_0050e930; renamed per out/phase4/render_types_notes.md's
// misattributed-functions table: "render_objects, render_object_shadows, render_object,
// render_object_list, render_object_shadow_begin, render_object_shadow_end")
// address 0x50e930, size 203 bytes
// name confidence: 0.55   rewrite confidence: 0.8
// evidence: calls FUN_0050eac0 (this batch's render_objects_collect / fake-shadow candidate list
//   rebuild) once, then FUN_0050eba0 (render_object, this batch) once per entry of
//   rendered_object_count -- matching the phase-2 summary "Per-frame update of the fake object
//   'blob' shadow system: rebuilds the nearby-object list and applies each shadow to nearby BSP
//   surfaces" combined with the object-pass description of 0x50eba0. The D3DRS_LIGHTING toggle
//   (state 0x89, vtable +0xe4) matches src/render/render_lighting_disable_workaround.c's already-
//   established pattern (0x511ef0), here inlined for both the enable (before) and disable (after)
//   sides. The two-pass loop (cVar1 0 then 1, comparing against console_debug_toggle_6893ee) is
//   preserved exactly even though only one of its two passes actually renders objects.
// register convention: none (void).
// review fix (phase-4 gate): the object loop builds an object_render_data at esp+0x10 (object
//   index at +0x00 from rendered_objects[i] at 0x50e9a3, shadow_pass +0x08 cleared once at
//   0x50e974) and passes it to render_object in EDI (0x50e9aa); the first draft called
//   render_object with no argument.
// UNSURE: which of the two passes (first_person_weapon_update_lighting vs the object loop) runs first is controlled by
//   console_debug_toggle_6893ee; first_person_weapon_update_lighting's purpose is not established (foreign, no evidence
//   in this batch).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "render.h"

extern uint8_t console_debug_toggle_6893ec; // 0x006893ec (matches
                                            // src/rasterizer/rasterizer_model_draw_prepare_states.c)
extern uint8_t rasterizer_render_states_dirty; // 0x0069c74c (matches
                                               // src/rasterizer/rasterizer_transparent_geometry_group_build.c)
extern uint8_t unknown_0071d1fa; // 0x0071d1fa UNSURE (matches
                                 // src/rasterizer/rasterizer_transparent_geometry_group_draw_active_camouflage.c)
extern uint32_t rasterizer_device_version; // 0x007c118c (matches src/render/render_player_frame.c)
extern void *rasterizer_device;            // 0x0071d174
extern uint8_t console_debug_toggle_6893ee; // 0x006893ee, UNSURE: selects which of the two passes
                                            // below actually renders objects
extern int16_t rendered_object_count;      // 0x006b8dc0, this module
extern datum_index rendered_objects[0x100]; // 0x006b8dc4, this module

typedef int32_t (__stdcall *d3d_set_render_state_fn)(void *device, uint32_t state, uint32_t value);

extern void first_person_weapon_update_lighting(void); // 0x4924b0, foreign; called on the pass that does not render
extern void render_objects_collect(void); // 0x50eac0, this module: rebuilds rendered_objects
extern void render_object(object_render_data *data); // 0x50eba0, this module; blam-cc: EDI=data

// Per-frame object render driver: optionally forces D3D lighting off around the whole pass on
// pre-0xffff0101 devices when the debug toggle is set, rebuilds the nearby-object candidate list,
// then runs a two-iteration loop where exactly one iteration (selected by
// console_debug_toggle_6893ee) calls render_object once per rendered object and the other calls
// the foreign first_person_weapon_update_lighting instead.
void render_objects(void)
{
    object_render_data data; // esp+0x10; only object_index and shadow_pass are set here
    uint8_t pass;
    uint8_t first_iteration;

    if (console_debug_toggle_6893ec != 0) {
        rasterizer_render_states_dirty = 1;
        unknown_0071d1fa = 0;
        if (rasterizer_device_version < 0xffff0101) {
            void **vtable = *(void ***)rasterizer_device;
            d3d_set_render_state_fn set_render_state = (d3d_set_render_state_fn)vtable[0xe4 / 4];
            set_render_state(rasterizer_device, 0x89, 1);
        }
    }

    render_objects_collect();

    data.shadow_pass = 0;
    pass = 0;
    do {
        if (pass == console_debug_toggle_6893ee) {
            int16_t i;
            for (i = 0; i < rendered_object_count; i++) {
                data.object_index = rendered_objects[i];
                render_object(&data);
            }
        } else {
            first_person_weapon_update_lighting();
        }
        first_iteration = (pass == 0);
        pass = 1;
    } while (first_iteration);

    if (console_debug_toggle_6893ec != 0 && rasterizer_device_version < 0xffff0101) {
        void **vtable = *(void ***)rasterizer_device;
        d3d_set_render_state_fn set_render_state = (d3d_set_render_state_fn)vtable[0xe4 / 4];
        set_render_state(rasterizer_device, 0x89, 0);
    }
}

#if 0
Original Ghidra decompilation (0x50e930):

void FUN_0050e930(void)

{
  char cVar1;
  short sVar2;
  bool bVar3;

  cVar1 = '\0';
  if (DAT_006893ec != '\0') {
    DAT_0069c74c = 1;
    DAT_0071d1fa = 0;
    if (DAT_007c118c < 0xffff0101) {
      (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x89,1);
    }
  }
  FUN_0050eac0();
  do {
    if (cVar1 == DAT_006893ee) {
      sVar2 = 0;
      if (0 < DAT_006b8dc0) {
        do {
          FUN_0050eba0();
          sVar2 = sVar2 + 1;
        } while (sVar2 < DAT_006b8dc0);
      }
    }
    else {
      FUN_004924b0();
    }
    bVar3 = cVar1 == '\0';
    cVar1 = '\x01';
  } while (bVar3);
  if ((DAT_006893ec != '\0') && (DAT_007c118c < 0xffff0101)) {
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x89,0);
  }
  return;
}
#endif
