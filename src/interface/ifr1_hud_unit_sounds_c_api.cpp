#include "halo/interface/ifr1_hud_unit_sounds.hpp"

/**
 * C ABI entry point; forwards to halo::interface::HudUnitSounds::play.
 *
 * @address 0x4afd30
 */
extern "C" void hud_unit_sounds_play(uint32_t active_mask, const TagReflexive *sounds, int32_t *handles, uint16_t *playing)
{
    halo::interface::HudUnitSounds::play(active_mask, sounds, handles, playing);
}

/**
 * C ABI entry point; forwards to halo::interface::HudUnitSounds::update.
 * blam-cc: player -> EAX
 *
 * @address 0x4afee0
 */
extern "C" void hud_unit_sounds_update(player *p, uint8_t hud_enabled)
{
    halo::interface::HudUnitSounds::update(p, hud_enabled);
}
