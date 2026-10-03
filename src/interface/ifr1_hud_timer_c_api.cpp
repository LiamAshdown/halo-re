#include "halo/interface/ifr1_hud_timer.hpp"

/**
 * C ABI entry point; forwards to halo::interface::HudTimer::pause_timer.
 * blam-cc: paused -> DL
 *
 * @address 0x4adc60
 */
extern "C" void hud_pause_timer(uint8_t paused)
{
    halo::interface::HudTimer::pause_timer(paused);
}

/**
 * C ABI entry point; forwards to halo::interface::HudTimer::set_timer_time.
 * blam-cc: minutes -> ECX, seconds -> EAX
 *
 * @address 0x4adbf0
 */
extern "C" void hud_set_timer_time(int32_t minutes, int32_t seconds)
{
    halo::interface::HudTimer::set_timer_time(minutes, seconds);
}

/**
 * C ABI entry point; forwards to halo::interface::HudTimer::draw.
 *
 * @address 0x4add10
 */
extern "C" void hud_timer_draw(void)
{
    halo::interface::HudTimer::draw();
}

/**
 * C ABI entry point; forwards to halo::interface::HudTimer::get_ticks.
 *
 * @address 0x4adcc0
 */
extern "C" uint32_t hud_timer_get_ticks(void)
{
    return halo::interface::HudTimer::get_ticks();
}
