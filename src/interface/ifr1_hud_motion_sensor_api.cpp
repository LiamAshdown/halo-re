#include "halo/interface/ifr1_hud_motion_sensor.hpp"
#include "halo/interface/api.hpp"

namespace halo::interface {

/**
 * C ABI entry point; forwards to halo::interface::HudMotionSensor::update.
 *
 * @address 0x4b3920
 */
void chimera__motion_sensor_update(void)
{
    halo::interface::HudMotionSensor::update();
}

}
