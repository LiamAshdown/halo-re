#include "halo/interface/ifr1_hud_motion_sensor.hpp"

/**
 * C ABI entry point; forwards to halo::interface::HudMotionSensor::update.
 *
 * @address 0x4b3920
 */
extern "C" void chimera__motion_sensor_update(void)
{
    halo::interface::HudMotionSensor::update();
}
