#include "halo/interface/ifr1_hud_waypoints.hpp"
#include "halo/bitmaps/api.hpp"
#include <string.h>
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/render/api.hpp"

extern "C" {
extern data_array *player_data;
extern hud_waypoint_state *hud_waypoints;
extern HUDGlobals *hud_globals_tag_data;
extern player_globals *local_player_globals;
extern uint8_t render_frustum_global[];
extern uint8_t render_camera_global[];
extern int16_t render_viewport_top;
extern Rectangle2D screen_safe_area_right;
extern float sqrtf(float x);
extern float atan2f(float y, float x);
extern double pow(double base, double exponent);
extern double fmod(double x, double y);
extern long lrint(double x);
extern int32_t __ftol(double x);
extern int32_t ui_real_to_int_truncate(float value);
extern void unit_get_camera_position(datum_index unit_index, real_point3d *out);
extern uint8_t render_project_world_point_to_screen(real_point2d *out, const real_point3d *point, void *frustum,
                                                    void *camera);
extern uint32_t color_rgb_float_to_int(const float *rgb);
extern void hud_meter_resolve_bitmap_frame(datum_index bitmap_tag, int16_t sequence_index, uint16_t frame_index,
                                           void **out_data, int32_t *out_offset);
extern void hud_draw_bitmap_at(const float *uv, BitmapData *bitmap, uint8_t pixel_uvs, int16_t anchor,
                               const Point2DInt *screen_position, float scale, float rotation, uint32_t color);
extern void hud_draw_number(void *unused, uint16_t *anchor, const hud_number_placement *placement, int16_t value,
                            int16_t fraction, uint32_t flags, int32_t flash_start_time, float scale);
extern int16_t current_local_player_index;
extern void hud_waypoint_draw_one(datum_index player_index);
}

static float hud_clamp01(float value)
{
    if (value < 0.0f) {
        return 0.0f;
    }
    if (value > 1.0f) {
        return 1.0f;
    }
    return value;
}

namespace halo::interface {

void LocalPlayerVisitor::for_each_on_team(int16_t team, LocalPlayerVisitor &visitor)
{
    data_iterator iterator;
    player *p;

    iterator.data = player_data;
    iterator.next_index = 0;
    iterator.index = (datum_index)-1;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
    for (p = (player *)halo::memory::data_iterator_next(&iterator); p != 0;
         p = (player *)halo::memory::data_iterator_next(&iterator)) {
        if (p->local_player_index != -1 && (int32_t)team == p->team) {
            visitor.visit(iterator.index);
        }
    }
}

bool WaypointSlotSet::for_player(datum_index player_index, WaypointSlotSet *out)
{
    int16_t local_player_index;

    if (player_index == (datum_index)-1) {
        return false;
    }
    local_player_index = ((player *)((uint8_t *)player_data->data + (player_index & 0xffff) * 0x200))->local_player_index;
    if (local_player_index < 0 || local_player_index >= 1) {
        return false;
    }
    *out = WaypointSlotSet(hud_waypoints[local_player_index].waypoints);
    return true;
}

void WaypointSlotSet::activate(datum_index target, int16_t kind, int16_t arrow_index, float vertical_offset)
{
    int16_t free_slot = -1;
    int16_t i;

    for (i = 0; i < k_slot_count; i++) {
        hud_waypoint *waypoint = &slots[i];
        int16_t slot_kind = (int16_t)(waypoint->type << 12) >> 12;

        if (slot_kind == kind && waypoint->object_index == target) {
            waypoint->arrow_index = arrow_index;
            waypoint->vertical_offset = vertical_offset;
            return;
        }
        if (slot_kind == -1) {
            free_slot = i;
        }
    }
    if (free_slot != -1) {
        hud_waypoint *waypoint = &slots[free_slot];
        waypoint->object_index = target;
        waypoint->arrow_index = arrow_index;
        waypoint->type = (int16_t)(waypoint->type ^ ((waypoint->type ^ kind) & 0xf));
        waypoint->vertical_offset = vertical_offset;
    }
}

void WaypointSlotSet::deactivate(datum_index target, int16_t kind)
{
    int16_t i;

    for (i = 0; i < k_slot_count; i++) {
        hud_waypoint *waypoint = &slots[i];
        if ((int16_t)(waypoint->type << 12) >> 12 == kind && waypoint->object_index == target) {
            waypoint->type |= 0xf;
            waypoint->object_index = (datum_index)-1;
            waypoint->arrow_index = -1;
            return;
        }
    }
}

namespace {

class ActivateVisitor final : public LocalPlayerVisitor {
public:
    ActivateVisitor(datum_index target, int16_t kind, int16_t arrow_index, float vertical_offset)
        : target(target), kind(kind), arrow_index(arrow_index), vertical_offset(vertical_offset) {}

    void visit(datum_index player_index) override
    {
        HudWaypoints::activate_for_player(player_index, target, kind, arrow_index, vertical_offset);
    }

private:
    datum_index target;
    int16_t kind;
    int16_t arrow_index;
    float vertical_offset;
};

class DeactivateVisitor final : public LocalPlayerVisitor {
public:
    DeactivateVisitor(datum_index target, int16_t kind) : target(target), kind(kind) {}

    void visit(datum_index player_index) override
    {
        HudWaypoints::deactivate_for_player(player_index, target, kind);
    }

private:
    datum_index target;
    int16_t kind;
};

}

/**
 * Shows waypoint arrow arrow_index over target (a flag, object or custom waypoint by kind) for the local
 * player of player_index.
 * blam-cc: EAX -> player_index, EBX -> target, DX -> kind, stack -> arrow_index, vertical_offset
 *
 * @address 0x4af0d0
 */
void HudWaypoints::activate_for_player(datum_index player_index, datum_index target, int16_t kind, int16_t arrow_index, float vertical_offset)
{
    WaypointSlotSet slots(nullptr);

    if (!WaypointSlotSet::for_player(player_index, &slots) || target == (datum_index)-1 || arrow_index == -1) {
        return;
    }
    slots.activate(target, kind, arrow_index, vertical_offset);
}

/**
 * Shows the waypoint for every local player on the given team.
 * blam-cc: target -> EAX
 *
 * @address 0x4af1b0
 */
void HudWaypoints::activate_for_team(datum_index target, int16_t arrow_index, int16_t team, int16_t kind, float vertical_offset)
{
    ActivateVisitor visitor(target, kind, arrow_index, vertical_offset);

    LocalPlayerVisitor::for_each_on_team(team, visitor);
}

/**
 * Index of the HUD waypoint arrow called name, -1 when there is none or no HUD globals tag.
 * blam-cc: name -> EDI
 *
 * @address 0x4af070
 */
int16_t HudWaypoints::arrow_find(const char *name)
{
    int16_t i;

    if (hud_globals_tag_data == 0) {
        return -1;
    }
    for (i = 0; (int32_t)i < (int32_t)hud_globals_tag_data->waypoint_arrows.count; i++) {
        const HUDGlobalsWaypointArrow *arrow =
            (const HUDGlobalsWaypointArrow *)hud_globals_tag_data->waypoint_arrows.pointer + i;
        if (_stricmp(name, arrow->name.string) == 0) {
            return i;
        }
    }
    return -1;
}

/**
 * Hides the waypoint of the given kind and target for the local player of player_index.
 * blam-cc: EAX -> player_index, EDI -> target, SI -> kind
 *
 * @address 0x4af230
 */
void HudWaypoints::deactivate_for_player(datum_index player_index, datum_index target, int16_t kind)
{
    WaypointSlotSet slots(nullptr);

    if (!WaypointSlotSet::for_player(player_index, &slots) || target == (datum_index)-1) {
        return;
    }
    slots.deactivate(target, kind);
}

/**
 * Hides the waypoint of the given kind and target for every local player on the given team.
 * blam-cc: kind -> EAX
 *
 * @address 0x4af2b0
 */
void HudWaypoints::deactivate_for_team(int16_t kind, int16_t team, datum_index target)
{
    DeactivateVisitor visitor(target, kind);

    LocalPlayerVisitor::for_each_on_team(team, visitor);
}

/**
 * Original engine function hud_waypoint_draw; the author notes are in
 * docs/original/interface/hud_waypoint_draw.txt.
 * blam-cc: position -> EAX
 *
 * @address 0x4af5e0
 */
void HudWaypoints::draw(const real_point3d *position, int16_t local_player_index, int16_t arrow_index, int16_t visibility, uint8_t show_distance)
{
    HUDGlobals *globals = hud_globals_tag_data;
    const HUDGlobalsWaypointArrow *arrow =
        (const HUDGlobalsWaypointArrow *)globals->waypoint_arrows.pointer + arrow_index;
    real_point3d point;
    real_point3d camera;
    real_point2d screen;
    datum_index unit_index;
    float distance;
    float scale;
    float x;
    float y;
    float half_width;
    float half_height;
    float rotation;
    BitmapData *bitmap;
    int32_t uv_offset;
    const float *uv;
    Point2DInt arrow_position;
    ColorRGB color;
    uint8_t alpha;
    int32_t whole;
    uint32_t packed;

    point = *position;
    unit_index = (datum_index)-1;
    if (local_player_index != -1 && local_player_index < 1 &&
        local_player_globals->local_players[local_player_index] != (datum_index)-1) {
        unit_index = ((player *)((uint8_t *)player_data->data +
                                 (local_player_globals->local_players[local_player_index] & 0xffff) * 0x200))->unit;
    }
    unit_get_camera_position(unit_index, &camera);
    {
        float dx = position->x - camera.x;
        float dy = position->y - camera.y;
        float dz = position->z - camera.z;
        distance = sqrtf(dz * dz + dy * dy + dx * dx);
    }
    if (distance > 15.0f) {
        scale = 0.5f;
    } else {
        scale = (float)(pow((double)(1.0f - distance * 0.06666667014360428f), 0.7) + 0.5);
    }

    halo::math::matrix4x3_transform_point(point, point, halo::render::globals().camera_world_to_view);
    if (visibility != 1 && halo::render::render_project_world_point_to_screen(&screen, &point, (render_frustum *)render_frustum_global, (render_camera *)render_camera_global) != 0) {
        x = screen.x - (float)(halo::render::globals().viewport_left + 0x140);
        y = screen.y - (float)(render_viewport_top + 0xf0);
    } else {
        x = point.x;
        visibility = 1;
        y = -point.y;
    }

    half_width = (640.0f - (globals->left_offset + globals->right_offset)) * 0.5f;
    half_height = (480.0f - (globals->top_offset + globals->bottom_offset)) * 0.5f;
    rotation = 0.0f;
    {
        float radius = half_height * half_width;
        float scaled_x = half_height * x;
        float scaled_y = half_width * y;

        if (visibility == 1 || !(scaled_y * scaled_y + scaled_x * scaled_x < radius * radius)) {
            float k = sqrtf((radius * radius) / (scaled_y * scaled_y + scaled_x * scaled_x));
            visibility = 1;
            x = x * k;
            y = y * k;
            if ((arrow->flags & 1) == 0) {
                rotation = -atan2f(x, y);
            }
        }
    }
    screen.x = x + 320.0f;
    screen.y = y + 240.0f;

    bitmap = 0;
    uv_offset = 0;
    hud_meter_resolve_bitmap_frame(*(datum_index *)&globals->arrow_bitmap.tag_id,
                                   (int16_t)(&arrow->on_screen_sequence_index)[visibility], 0, (void **)&bitmap,
                                   &uv_offset);
    if (bitmap == 0 || halo::cache::texture_cache_get(bitmap, 0, 1) == 0) {
        return;
    }
    uv = (const float *)uv_offset;
    arrow_position.x = (int16_t)__ftol((double)screen.x);
    arrow_position.y = (int16_t)__ftol((double)screen.y);

    whole = ui_real_to_int_truncate(arrow->opacity);
    if (whole * 0xff < 0) {
        alpha = 0;
    } else if (whole * 0xff > 0xff) {
        alpha = 0xff;
    } else {
        alpha = (uint8_t)-(int8_t)ui_real_to_int_truncate(arrow->opacity);
    }
    halo::bitmaps::color_rgb_int_to_real(&color, *(const uint32_t *)&arrow->color);
    color.red = hud_clamp01(1.0f - arrow->translucency) * color.red;
    color.green = hud_clamp01(1.0f - arrow->translucency) * color.green;
    color.blue = hud_clamp01(1.0f - arrow->translucency) * color.blue;
    if (show_distance == 0) {
        color.red = color.blue;
        color.green = 0.0f;
        color.blue = 0.0f;
    }
    packed = color_rgb_float_to_int(&color.red) | ((uint32_t)alpha << 24);
    hud_draw_bitmap_at(uv, bitmap, 0, 4, &arrow_position, scale, rotation, packed);

    if (visibility == 1 || show_distance == 0) {
        return;
    }
    {
        uint16_t anchor[0x12];
        hud_number_placement placement;
        float meters = distance * 3.048f;
        float power;
        int16_t number_x;
        int16_t number_y;

        memset(anchor, 0, sizeof(anchor));
        memset(&placement, 0, sizeof(placement));
        packed = color_rgb_float_to_int(&color.red) | ((uint32_t)alpha << 24);
        *(uint32_t *)&placement.flash.default_color = packed;
        packed = color_rgb_float_to_int(&color.red) | ((uint32_t)alpha << 24);
        *(uint32_t *)&placement.flash.flashing_color = packed;
        placement.maximum_number_of_digits = 3;
        placement.number_of_fractional_digits = 1;
        placement.flags = 5;
        number_x = (int16_t)__ftol((double)((uv[1] - uv[0]) * (float)(int16_t)bitmap->width * 0.5f * scale * 0.33f +
                                            (float)arrow_position.x));
        number_y = (int16_t)__ftol((double)((uv[3] - uv[2]) * (float)(int16_t)bitmap->height * 0.5f * scale * 0.66f +
                                            (float)arrow_position.y));
        placement.anchor_offset.x = (int16_t)(number_x + (int16_t)(halo::render::globals().viewport_left - screen_safe_area_right.left));
        placement.anchor_offset.y = (int16_t)(number_y + (int16_t)(render_viewport_top - screen_safe_area_right.top));
        power = (float)pow(10.0, 4.0);
        whole = (int32_t)lrint(fmod((double)(power * meters < 0.0f ? -(power * meters) : power * meters), (double)power));
        hud_draw_number((void *)(int32_t)local_player_index, anchor, &placement,
                        (int16_t)ui_real_to_int_truncate(meters), (int16_t)whole, 0, 0, 0.0f);
    }
}

/**
 * Draws a waypoint over every teammate of the local player that currently drives a unit.
 *
 * @address 0x4aa5f0
 */
void HudWaypoints::draw_all_for_player(void)
{
    datum_index local_player;
    int32_t team;
    data_iterator iterator;
    datum_index teammates[16];
    int32_t count = 0;
    int32_t i;
    player *entry;

    if (current_local_player_index == -1 || current_local_player_index >= 1) {
        local_player = (datum_index)-1;
    } else {
        local_player = local_player_globals->local_players[current_local_player_index];
    }
    team = ((player *)((uint8_t *)player_data->data + (local_player & 0xffff) * sizeof(player)))->team;
    if (local_player == (datum_index)-1) {
        return;
    }

    iterator.data = player_data;
    iterator.next_index = 0;
    iterator.index = (datum_index)-1;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
    for (entry = (player *)halo::memory::data_iterator_next(&iterator); entry != 0;
         entry = (player *)halo::memory::data_iterator_next(&iterator)) {
        if (local_player != iterator.index && entry->team == team && entry->unit != (datum_index)-1) {
            teammates[count] = iterator.index;
            count++;
        }
    }

    for (i = 0; i < count; i++) {
        hud_waypoint_draw_one(teammates[i]);
    }
}

}
