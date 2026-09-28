// object_compute_level_of_detail_pixels  (Ghidra: FUN_0050f740; new name, evidence below)
// address 0x50f740, size 233 bytes
// name confidence: 0.5   rewrite confidence: 0.8
// evidence: types/render.h cached_object_render_state.level_of_detail_pixels ("0x50f740
//   projected size; above 400 or 100 pixels the refresh interval drops"). Computes the
//   view-space depth of object->bounding_center through render_frustum.world_to_view (the
//   frustum's forward.z / left.z / up.z / position.z terms at 0x007c3184/90/9c/a8, i.e.
//   frustum + 0x1c/0x28/0x34/0x40 -- see out/phase4/render_types_notes.md derivation), then
//   returns 2 * radius * (render_frustum.projection_world_to_screen.j / depth), the standard
//   world-radius-to-screen-radius projection (projection_world_to_screen at frustum + 0x184/0x188,
//   i.e. 0x007c32ec/0x007c32f0; only the .j term, 0x007c32f0, is used here). object->bounding_radius
//   is itself halved or quartered by a detail-setting global before the projection.
// register convention: EAX = object index (datum_index, low 16 bits used), no other arguments.
//   // blam-cc: EAX -> object_index
// UNSURE: object flags bit 0x400000 (early-out to FLT_MAX) and the two gating globals
//   (unnamed_006f187c_9, render_detail_setting) are not documented anywhere in types/. Named
//   generically and marked below.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "rasterizer.h"
#include "units.h"
#include "cutscene.h"

extern data_array *object_data;                                  // 0x008603b0
extern cinematic_globals *cinematic_globals_ptr; // 0x006f187c
extern int32_t unknown_00689450;      // 0x00689450 UNSURE: video detail setting, forced to 2 (high)
extern render_frustum render_frustum_global; // 0x007c3168, this module (named render_frustum_global;
                                             // a variable cannot share the render_frustum typedef's
                                             // own name in C; matches src/render/render_nonplayer_frame.c)

// Projects an object's bounding sphere to an approximate on-screen pixel radius, used by
// object_render_state_refresh (0x50f270) to pick the cached-lighting refresh interval. Objects
// flagged with the (unnamed) 0x400000 bit are exempt from the detail falloff while
// cinematic_globals_ptr->in_progress is set, and always report the maximum size.
real object_compute_level_of_detail_pixels(datum_index object_index)
{
    object *obj;
    real radius;
    real depth;

    obj = ((object_header *)object_data->data)[(uint16_t)object_index].data;

    if (cinematic_globals_ptr->in_progress != 0 && (obj->flags & 0x400000) != 0) {
        return 3.4028235e+38f; // UNSURE: FLT_MAX, object exempt from LOD falloff
    }

    radius = obj->bounding_radius;
    // read as a word (mov ax,ds:0x689450 at 0x50f79a)
    if ((int16_t)unknown_00689450 == 1) {
        radius = radius * 0.5f;
    } else if ((int16_t)unknown_00689450 == 0) {
        radius = radius * 0.25f;
    }

    depth = render_frustum_global.world_to_view.left.k * obj->bounding_center.y +
            render_frustum_global.world_to_view.forward.k * obj->bounding_center.x +
            render_frustum_global.world_to_view.up.k * obj->bounding_center.z +
            render_frustum_global.world_to_view.position.z;
    if (depth < 0.0f) {
        depth = -depth;
    }
    if (depth <= 0.1f) {
        depth = 0.1f;
    }

    radius = (render_frustum_global.projection_world_to_screen.j / depth) * radius;
    return radius + radius;
}

#if 0
Original Ghidra decompilation (0x50f740):

float10 FUN_0050f740(void)

{
  int iVar1;
  uint in_EAX;
  float10 fVar2;
  float10 fVar3;

  if ((*(char *)(DAT_006f187c + 9) != '\0') &&
     ((*(uint *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc) + 0x10) &
      0x400000) != 0)) {
    return (float10)3.4028235e+38;
  }
  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  fVar2 = (float10)*(float *)(iVar1 + 0xac);
  if (DAT_00689450 == 1) {
    fVar2 = fVar2 * (float10)0.5;
  }
  else if (DAT_00689450 == 0) {
    fVar2 = fVar2 * (float10)0.25;
  }
  fVar3 = (float10)_DAT_007c3190 * (float10)*(float *)(iVar1 + 0xa4) +
          (float10)_DAT_007c3184 * (float10)*(float *)(iVar1 + 0xa0) +
          (float10)_DAT_007c319c * (float10)*(float *)(iVar1 + 0xa8) + (float10)_DAT_007c31a8;
  if (fVar3 < (float10)0.0) {
    fVar3 = -fVar3;
  }
  if (fVar3 <= (float10)0.1) {
    fVar3 = (float10)0.1;
  }
  fVar2 = ((float10)_DAT_007c32f0 / fVar3) * fVar2;
  return fVar2 + fVar2;
}
#endif
