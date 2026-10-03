#include "halo/interface/ifr1_hud_timer.hpp"
#include "halo/interface/api.hpp"

namespace halo::interface {

/**
 * C ABI entry point; forwards to halo::interface::HudTimer::pause.
 * blam-cc: paused -> DL
 *
 * @address 0x4adc60
 */
void hud_pause_timer(uint8_t paused)
{
    halo::interface::HudTimer().pause(paused);
}

/**
 * C ABI entry point; forwards to halo::interface::HudTimer::set_time.
 * blam-cc: minutes -> ECX, seconds -> EAX
 *
 * @address 0x4adbf0
 */
void hud_set_timer_time(int32_t minutes, int32_t seconds)
{
    halo::interface::HudTimer().set_time(minutes, seconds);
}

/**
 * C ABI entry point; forwards to halo::interface::HudTimer::draw.
 *
 * @address 0x4add10
 */
void hud_timer_draw(void)
{
    halo::interface::HudTimer().draw();
}

/**
 * C ABI entry point; forwards to halo::interface::HudTimer::ticks.
 *
 * @address 0x4adcc0
 */


}
