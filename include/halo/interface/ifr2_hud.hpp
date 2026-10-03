#pragma once

#include <stdint.h>
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "objects.h"
#include "projectiles.h"
#include "items.h"
#include "units.h"

#ifdef interface
#undef interface
#endif

namespace halo::interface {

/**
 * Per-local-player HUD waypoint visibility, update and drawing.
 */
class HudWaypoints {
public:
    explicit constexpr HudWaypoints(int16_t view_local_player_index) : local_player_index(view_local_player_index) {}
    int16_t local_player_index;

    int16_t visibility(const real_point3d *eye, const real_point3d *target, datum_index ignore_object);
    void draw_for_player();
    static void update();
    void update_for_player();
};

/**
 * Weapon HUD interface state, crosshair/element drawing and weapon action notifications.
 */
class WeaponHud {
public:
    WeaponHud() = delete;

    static void crosshairs_draw(datum_index hud_tag, const player *p, const weapon_hud_ammo_state *ammo);
    static void draw_elements(datum_index hud_tag, int16_t local_player_index, const Weapon *weapon_tag, const weapon_hud_ammo_state *ammo, const uint16_t *parent_state_flags, const uint16_t *parent_overlay_types, const int16_t *parent_numbers);
    static void meters_evaluate(datum_index hud_interface_tag_id, int16_t local_player_index, int32_t weapon_or_vehicle_index, const weapon_hud_ammo_state *state_ptr);
    static void state_update();
    static int16_t animation_stage(int16_t message_stage);
    static int16_t message_stage(int16_t item_type_code);
    static int32_t weapon_hud_interface(float *out_intensity);
    static int16_t text_message_index(datum_index object_index);
    static void notify_for_unit(datum_index unit_index, int32_t action_code);
    static void notify_for_weapon(datum_index weapon_index, int32_t action_code);
    static uint8_t ammo_state_is_empty(const weapon_hud_ammo_state *state);

private:
    static int32_t hud_percent(float value);
    static uint16_t hud_overlay_type_bits(uint16_t bits);
    static object * object_get(datum_index object_index);
};

/**
 * Motion sensor blip tracking and rendering for one local player.
 */
class MotionSensor {
public:
    explicit constexpr MotionSensor(int16_t view_local_player_index) : local_player_index(view_local_player_index) {}
    int16_t local_player_index;

    void blip_fill(datum_index object_index, motion_sensor_blip *blip);
    static uint8_t object_is_detected(datum_index unit_index);
    static void plot_blip(const float *position, uint8_t type, const motion_sensor_frame *frame, int8_t subtype, float pixels_per_unit, float alpha, float size_factor);
    static void render(uint8_t splitscreen, const int16_t *screen_center, int16_t local_player_index);
    static void reset();
    void update_for_player();
};

} // namespace halo::interface
