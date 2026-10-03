#include "halo/interface/ifr1_hud_timer.hpp"
#include "halo/interface/records.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/core/datum.hpp"
#include "halo/cache/api.hpp"
#include "halo/interface/api.hpp"
#include "halo/game/api.hpp"
#include "halo/core/link.hpp"
#include "halo/game/vars.hpp"
#include "halo/interface/vars.hpp"

static auto &hud_messaging = halo::link::ref<hud_messaging_globals *>(halo::ui::vars().hud_messaging);
static auto &global_globals = halo::link::ref<Globals *>(halo::game::vars().global_globals);
static auto &hud_globals_tag_data = halo::link::ref<HUDGlobals *>(halo::ui::vars().hud_globals_tag_data);
static auto &current_local_player_index = halo::link::ref<int16_t>(halo::ui::vars().current_local_player_index);
extern "C" {
extern int32_t __ftol(double x);
}

namespace halo::interface {

/**
 * Binds the timer to the engine's HUD messaging state and game clock.
 */
HudTimer::HudTimer() : messaging(hud_messaging), time(halo::game::globals().game_time)
{
}

/**
 * Freezes or resumes the HUD countdown timer, preserving the remaining tick count across the pause.
 * blam-cc: paused -> DL
 *
 * @address 0x4adc60
 */
void HudTimer::pause(uint8_t paused)
{
    int16_t ticks = (int16_t)messaging->timer_ticks;

    messaging->timer_paused = paused;
    if (ticks <= 0) {
        return;
    }
    if (paused != 0) {
        messaging->timer_ticks =
            (uint16_t)((uint16_t)((uint16_t)messaging->timer_start_time - (uint16_t)time->game_time) + ticks);
    } else {
        messaging->timer_ticks =
            (uint16_t)((uint16_t)((uint16_t)time->game_time - (uint16_t)messaging->timer_start_time) + ticks);
    }
}

/**
 * Starts the HUD countdown timer at minutes:seconds.
 * blam-cc: minutes -> ECX, seconds -> EAX
 *
 * @address 0x4adbf0
 */
void HudTimer::set_time(int32_t minutes, int32_t seconds)
{
    int16_t anchor;

    messaging->timer_ticks = (uint16_t)((minutes * 60 + seconds) * 30);
    messaging->timer_paused = 0;
    messaging->timer_active = 1;
    messaging->timer_start_time = time->game_time;
    anchor = messaging->timer_anchor;
    if (anchor < 0) {
        anchor = 0;
    } else if (anchor > 4) {
        anchor = 4;
    }
    messaging->timer_anchor = anchor;
}

/**
 * Draws the HUD countdown timer when a script armed it.
 *
 * @address 0x4add10
 */
void HudTimer::draw(void)
{
    uint16_t anchor[0x12];
    hud_number_placement placement;
    GlobalsInterfaceBitmaps *interface_bitmaps;
    datum_index digits_tag;
    int32_t now;
    int32_t digit_step;
    int16_t ticks;
    int32_t clock;
    int32_t minutes;
    int32_t seconds;
    int32_t sub_second;
    uint32_t flash;
    double spacing;
    int16_t i;

    if (messaging->timer_active == 0) {
        return;
    }
    now = time->game_time;
    for (i = 1; i < 0x12; i++) {
        anchor[i] = 0;
    }
    anchor[0] = (uint16_t)messaging->timer_anchor;
    flash = 0;
    digit_step = 0;
    ticks = (int16_t)this->ticks();
    placement.maximum_number_of_digits = 2;
    placement.number_of_fractional_digits = 4;
    placement.flags = 1;
    placement.width_scale = 1.0f;
    placement.height_scale = 1.0f;
    placement.anchor_offset = messaging->timer_offset;

    interface_bitmaps = global_globals->interface_bitmaps.count != 0
        ? (GlobalsInterfaceBitmaps *)global_globals->interface_bitmaps.pointer
        : (GlobalsInterfaceBitmaps *)0;
    digits_tag = *(datum_index *)&interface_bitmaps->hud_digits_definition.tag_id;
    if (digits_tag != (datum_index)-1) {
        HUDNumber *digits = halo::interface::tag_data<HUDNumber>(digits_tag);
        digit_step = __ftol((double)((float)(int32_t)digits->screen_digit_width + (float)(int32_t)digits->screen_digit_width));
    }
    switch (messaging->timer_anchor) {
    case 1:
    case 3:
        placement.anchor_offset.x = (int16_t)(placement.anchor_offset.x + digit_step * 5);
        digit_step = -digit_step;
        break;
    case 4:
        placement.anchor_offset.x = (int16_t)(placement.anchor_offset.x + digit_step * -3);
        break;
    }

    if (ticks > 0) {
        placement.flash = *(hud_flash_parameters *)&hud_globals_tag_data->not_much_time_left_default_color;
        placement.disabled_color = hud_globals_tag_data->not_much_time_left_disabled_color;
        if (ticks <= messaging->timer_warning_ticks) {
            flash = 1;
            if ((int16_t)messaging->timer_ticks > messaging->timer_warning_ticks) {
                messaging->timer_ticks = (uint16_t)messaging->timer_warning_ticks;
                messaging->timer_start_time = messaging->timer_warning_ticks - ticks + now;
            }
        }
    } else {
        placement.flash = *(hud_flash_parameters *)&hud_globals_tag_data->time_out_flash_default_color;
        placement.disabled_color = hud_globals_tag_data->time_out_flash_disabled_color;
        messaging->timer_ticks = halo::k_word_none;
        flash = 1;
        if (messaging->timer_start_time == -1) {
            messaging->timer_start_time = time->game_time;
        }
    }

    clock = ticks > 0 ? ticks : 0;
    sub_second = (int16_t)clock % 30;
    minutes = (int16_t)((int16_t)clock / 30) / 60;
    seconds = (int16_t)((int16_t)clock / 30) % 60;
    halo::interface::hud_draw_number((void *)(uint32_t)current_local_player_index, anchor, &placement, (int16_t)minutes, -1, flash,
                    messaging->timer_start_time, 2.0f);

    spacing = (double)(int16_t)digit_step * 2.5;
    placement.anchor_offset.x = (int16_t)__ftol((double)placement.anchor_offset.x + spacing);
    halo::interface::hud_draw_number((void *)(uint32_t)current_local_player_index, anchor, &placement, (int16_t)seconds, -1, flash,
                    messaging->timer_start_time, 2.0f);

    placement.anchor_offset.x = (int16_t)__ftol((double)placement.anchor_offset.x + spacing);
    halo::interface::hud_draw_number((void *)(uint32_t)current_local_player_index, anchor, &placement,
                    (int16_t)(sub_second * 100 / 30), -1, flash, messaging->timer_start_time, 2.0f);
}

/**
 * Ticks left on the HUD countdown timer (see the header for the special values).
 *
 * @address 0x4adcc0
 */
uint32_t HudTimer::ticks(void) const
{
    uint16_t ticks;

    if (messaging->timer_active == 0) {
        return 0;
    }
    ticks = messaging->timer_ticks;
    if (ticks == halo::k_word_none) {
        return halo::k_dword_none;
    }
    if (messaging->timer_paused != 0) {
        return ticks;
    }
    return (uint16_t)(ticks - (uint16_t)time->game_time + (uint16_t)messaging->timer_start_time);
}

}
