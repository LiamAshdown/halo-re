// render_object_shadows  (Ghidra: FUN_0050eb70; renamed per out/phase4/render_types_notes.md's
// misattributed-functions table: "render_objects, render_object_shadows, render_object,
// render_object_list, render_object_shadow_begin, render_object_shadow_end")
// address 0x50eb70, size 47 bytes
// name confidence: 0.55   rewrite confidence: 0.85
// evidence: out/phase4/render_functions.md's phase-2 summary: "Applies the fake shadow effect for
//   every candidate object/surface in the current shadow candidate list." Disassembly
//   (objdump -d -M intel, 0x50eb70..0x50eb9e) confirms EAX (moved to EDI at entry and left there
//   for the call) is the object_render_data block, matching the register-conventions note for
//   render_object 0x50eba0 ("EDI = object_render_data").
// register convention: EAX = data (object_render_data*).
//   // blam-cc: EAX=data

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "render.h"

extern int16_t rendered_object_count;       // 0x006b8dc0, this module
extern datum_index rendered_objects[0x100]; // 0x006b8dc4, this module

extern void render_object(object_render_data *data); // 0x50eba0, this module

// Applies the fake object "blob" shadow effect to every object in the current shadow candidate
// list (rebuilt by render_objects_collect, 0x50eac0), reusing the single caller-supplied
// object_render_data block for each one.
void render_object_shadows(object_render_data *data) // blam-cc: EAX=data
{
    int16_t i;

    for (i = 0; i < rendered_object_count; i++) {
        data->object_index = rendered_objects[i];
        render_object(data);
    }
}

#if 0
Original Ghidra decompilation (0x50eb70):

void FUN_0050eb70(void)

{
  undefined4 *in_EAX;
  short sVar1;

  sVar1 = 0;
  if (0 < DAT_006b8dc0) {
    do {
      *in_EAX = (&DAT_006b8dc4)[sVar1];
      FUN_0050eba0();
      sVar1 = sVar1 + 1;
    } while (sVar1 < DAT_006b8dc0);
  }
  return;
}
#endif
