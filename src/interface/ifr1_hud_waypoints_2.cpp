#include "halo/interface/ifr1_hud_waypoints.hpp"
#include <string.h>
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/render/api.hpp"

extern "C" {
extern data_array *player_data;
extern Globals *global_globals;
extern char ai_marker_name_a[];
extern uint8_t render_frustum_global[];
extern uint8_t render_camera_global[];
extern int16_t render_viewport_top;
extern float waypoint_fade_near;
extern float waypoint_fade_far;
extern int32_t object_get_node_local_transform(datum_index object_index, char *marker_name,
                                                object_marker *marker, uint32_t flags);
extern BitmapData *bitmap_group_sequence_get_bitmap_data(datum_index bitmap_tag, int16_t frame, int16_t sequence);
extern void ui_draw_rotated_screen_quad(int16_t *origin, int32_t source_record, float *corner_uvs,
                                        float scale, float rotation_radians, float alpha_fraction);
}

namespace halo::interface {

/**
 * Draws the teammate waypoint icon over the head of one player unit.
 * blam-cc: player_index -> EAX
 *
 * @address 0x4aa440
 */
void HudWaypoints::draw_one(datum_index player_index)
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
    halo::math::matrix4x3_transform_point(view_point, world_point, halo::render::globals().camera_world_to_view);
    if (!halo::render::render_project_world_point_to_screen(&screen_point, &view_point, (render_frustum *)render_frustum_global, (render_camera *)render_camera_global)) {
        return;
    }

    interface_bitmaps = (global_globals->interface_bitmaps.count != 0)
        ? (GlobalsInterfaceBitmaps *)global_globals->interface_bitmaps.pointer
        : (GlobalsInterfaceBitmaps *)0;
    bitmap = bitmap_group_sequence_get_bitmap_data(
        *(datum_index *)&interface_bitmaps->multiplayer_hud_bitmap.tag_id, 0, 0);
    if (halo::cache::texture_cache_get(bitmap, 0, 1) == 0) {
        return;
    }

    origin[0] = (int16_t)((int16_t)(int32_t)screen_point.x - halo::render::globals().viewport_left);
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

}
