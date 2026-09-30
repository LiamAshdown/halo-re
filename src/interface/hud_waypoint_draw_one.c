// hud_waypoint_draw_one  (Ghidra: FUN_004aa440, renamed)
// address 0x4aa440, size 419 bytes
// name confidence: 0.45 (chosen)   rewrite confidence: 0.7
// evidence: rewritten in the phase-4 review from objdump -d 0x4aa440..0x4aa5e2. The player unit
// "head" marker (literal at 0x0066bfa0) is looked up with object_get_node_local_transform; its
// world position (marker +0x60, the node_transform translation) is raised by 0.3 (0x00672c94),
// transformed by the view matrix at 0x007c3178 with matrix4x3_transform_point (EAX out, EDX in),
// then projected by 0x50de30 (ECX screen point out, EDX view point, ESI 0x007c3168, EDI the camera
// at 0x007c3114). When it lands on screen the Globals interface_bitmaps multiplayer_hud_bitmap
// (+0x9c of the element: dependency +0x90, tag_id +0x0c), sequence 0 frame 0, is drawn with
// ui_draw_rotated_screen_quad @0x494d70 at the screen point minus the viewport origin
// (0x007c3140 y, 0x007c3142 x), scale 1.0, rotation 0.0, alpha 1.0.
// The distance fade crops the icon rather than dimming it: depth d = -view.z,
// f = 1 - (d - near) * 100 / (far - near) clamped to 0.075 .. 1.0, and the corner uvs are
// {0, 1, (1 - f) / 2, 1}. near and far (0x007c3240, 0x007c3244) are floats; the earlier
// rewrite declared them as integers, dropped the marker-to-point copy and passed the uvs and
// alpha in the wrong slots.
// UNSURE: 0x50de30 and the two camera globals are only characterised by this call site.
// register convention: player index in EAX.
//   // blam-cc: player_index -> EAX

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "objects.h"
#include "networking.h"
#include "interface.h"
#include "fn_interface.h"
#include <string.h>

extern data_array *player_data;        // 0x0087a480
extern Globals *global_globals;         // 0x00746fa0
extern char ai_marker_name_a[];     // 0x0066bfa0, "head"
extern real_matrix4x3 render_camera_world_to_view; // 0x007c3178
extern uint8_t render_frustum_global[];    // 0x007c3168, UNSURE: passed in ESI to 0x50de30
extern uint8_t render_camera_global[];          // 0x007c3114, UNSURE: passed in EDI to 0x50de30
extern int16_t render_viewport_top;      // 0x007c3140
extern int16_t render_viewport_left;     // 0x007c3142
extern float waypoint_fade_near;         // 0x007c3240
extern float waypoint_fade_far;          // 0x007c3244

extern int32_t object_get_node_local_transform(datum_index object_index, char *marker_name,
                                                object_marker *marker, uint32_t flags); // 0x4f6080
extern void matrix4x3_transform_point(real_point3d *out, real_point3d *point, real_matrix4x3 *m); // 0x4cbde0, blam-cc: EAX out, EDX point
extern uint8_t render_project_world_point_to_screen(real_point2d *out, const real_point3d *view_point,
                                              const void *frustum, const void *camera); // 0x50de30, blam-cc: ECX, EDX, ESI, EDI
extern BitmapData *bitmap_group_sequence_get_bitmap_data(datum_index bitmap_tag, int16_t frame, int16_t sequence); // 0x43f290, blam-cc: EAX tag, DI frame
extern void *texture_cache_get(BitmapData *bitmap, uint8_t wait, uint8_t allocate_if_missing); // 0x444550, blam-cc: EAX bitmap


// blam-cc: player_index -> EAX
// Draws the teammate waypoint icon over the head of one player unit.
void hud_waypoint_draw_one(datum_index player_index)
{
    player *p = (player *)((uint8_t *)player_data->data + (player_index & 0xffff) * sizeof(player));
    object_marker marker;
    real_point3d world_point;
    real_point3d view_point;
    real_point2d screen_point;
    int16_t origin[2];
    float uvs[4];
    GlobalsInterfaceBitmaps *interface_bitmaps;
    BitmapData *bitmap;
    float fade;

    object_get_node_local_transform(p->unit, ai_marker_name_a, &marker, 1);
    world_point = *(real_point3d *)((uint8_t *)&marker + 0x60);
    world_point.z = world_point.z + 0.3f;
    matrix4x3_transform_point(&view_point, &world_point, &render_camera_world_to_view);
    if (!render_project_world_point_to_screen(&screen_point, &view_point, render_frustum_global, render_camera_global)) {
        return;
    }

    interface_bitmaps = (global_globals->interface_bitmaps.count != 0)
        ? (GlobalsInterfaceBitmaps *)global_globals->interface_bitmaps.pointer
        : (GlobalsInterfaceBitmaps *)0;
    bitmap = bitmap_group_sequence_get_bitmap_data(
        *(datum_index *)&interface_bitmaps->multiplayer_hud_bitmap.tag_id, 0, 0);
    if (texture_cache_get(bitmap, 0, 1) == 0) {
        return;
    }

    origin[0] = (int16_t)((int16_t)(int32_t)screen_point.x - render_viewport_left); // fistp, 16 bit subtract
    origin[1] = (int16_t)((int32_t)screen_point.y - render_viewport_top);

    fade = 1.0f - ((-view_point.z - waypoint_fade_near) * 100.0f) / (waypoint_fade_far - waypoint_fade_near);
    if (fade < 0.075f) {
        fade = 0.075f;
    } else if (fade > 1.0f) {
        fade = 1.0f;
    }
    uvs[0] = 0.0f;
    uvs[1] = 1.0f;
    uvs[2] = (1.0f - fade) * 0.5f;
    uvs[3] = 1.0f;
    ui_draw_rotated_screen_quad(origin, (int32_t)(uintptr_t)bitmap, uvs, 1.0f, 0.0f, 1.0f);
}

#if 0
Original Ghidra decompilation (0x4aa440):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_004aa440(void)

{
  float fVar1;
  char cVar2;
  uint in_EAX;
  undefined4 uVar3;
  int iVar4;
  undefined4 local_80;
  undefined4 local_7c;
  float local_78;
  undefined4 local_74;
  int local_70;
  undefined1 local_6c [100];
  float local_8;

  object_get_node_local_transform
            (*(undefined4 *)((in_EAX & 0xffff) * 0x200 + 0x34 + *(int *)(DAT_0087a480 + 0x34)),
             &DAT_0066bfa0,local_6c,1);
  matrix4x3_transform_point(&DAT_007c3178);
  cVar2 = render_project_world_point_to_screen();
  if (cVar2 != '\0') {
    uVar3 = bitmap_group_sequence_get_bitmap_data(0);
    iVar4 = texture_cache_get(0,1);
    if (iVar4 != 0) {
      local_70 = (int)ROUND(local_8);
      fVar1 = 1.0 - ((-local_78 - _DAT_007c3240) * 100.0) / (_DAT_007c3244 - _DAT_007c3240);
      if (0.075 <= fVar1) {
        if (1.0 < fVar1) {
          fVar1 = 1.0;
        }
      }
      else {
        fVar1 = 0.075;
      }
      local_78 = (1.0 - fVar1) * 0.5;
      local_80 = 0;
      local_7c = 0x3f800000;
      local_74 = 0x3f800000;
      FUN_00494d70(uVar3,&local_80,0x3f800000,0,0x3f800000);
    }
  }
  return;
}
#endif
