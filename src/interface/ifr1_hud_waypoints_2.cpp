#include "halo/interface/ifr1_hud_waypoints.hpp"
#include "halo/interface/records.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/bitmaps/api.hpp"
#include <string.h>
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/render/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/interface/api.hpp"
#include "halo/game/api.hpp"
#include "halo/core/link.hpp"
#include "halo/game/vars.hpp"
#include "halo/interface/vars.hpp"
#include "halo/render/vars.hpp"
#include "halo/units/vars.hpp"
#include "halo/units/api.hpp"

static auto &global_globals = halo::link::ref<Globals *>(halo::game::vars().global_globals);
static auto &ai_marker_name_a = halo::link::ref<char []>(halo::units::vars().ai_marker_name_a);
static auto &render_frustum_global = halo::link::ref<uint8_t []>(halo::render::vars().render_frustum_global);
static auto &render_camera_global = halo::link::ref<uint8_t []>(halo::render::vars().render_camera_global);
static auto &render_viewport_top = halo::link::ref<int16_t>(halo::ui::vars().render_viewport_top);
static auto &waypoint_fade_near = halo::link::ref<float>(halo::ui::vars().waypoint_fade_near);
static auto &waypoint_fade_far = halo::link::ref<float>(halo::ui::vars().waypoint_fade_far);

namespace halo::interface {

/**
 * Draws the teammate waypoint icon over the head of one player unit.
 * blam-cc: player_index -> EAX
 *
 * @address 0x4aa440
 */
void HudWaypoints::draw_one(datum_index player_index)
{
    player *p = halo::interface::player_record(player_index);
    object_marker marker;
    real_point3d world_point;
    real_point3d view_point;
    real_point2d screen_point;
    int16_t origin[2];
    float uvs[4];
    GlobalsInterfaceBitmaps *interface_bitmaps;
    BitmapData *bitmap;
    float fade;

    halo::objects::object_get_node_local_transform(p->unit, ai_marker_name_a, &marker, 1);
    world_point = *(real_point3d *)((uint8_t *)&marker + 0x60);
    world_point.z = world_point.z + 0.3f;
    halo::math::matrix4x3_transform_point(view_point, world_point, halo::render::globals().camera_world_to_view);
    if (!halo::render::render_project_world_point_to_screen(&screen_point, &view_point, (render_frustum *)render_frustum_global, (render_camera *)render_camera_global)) {
        return;
    }

    interface_bitmaps = (global_globals->interface_bitmaps.count != 0)
        ? (GlobalsInterfaceBitmaps *)global_globals->interface_bitmaps.pointer
        : (GlobalsInterfaceBitmaps *)0;
    bitmap = halo::bitmaps::bitmap_group_sequence_get_bitmap_data(
        halo::interface::tag_handle(interface_bitmaps->multiplayer_hud_bitmap.tag_id), 0, 0);
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
    halo::interface::ui_draw_rotated_screen_quad(origin, (int32_t)(uintptr_t)bitmap, uvs, 1.0f, 0.0f, 1.0f);
}

}
