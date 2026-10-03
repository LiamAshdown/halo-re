#include "halo/interface/ifr1_audio_options_profile.hpp"

/**
 * C ABI entry point; forwards to halo::interface::AudioOptionsProfile::apply_from_profile.
 *
 * @address 0x4a26a0
 */
extern "C" uint32_t audio_options_apply_from_profile(widget_instance *widget)
{
    return halo::interface::AudioOptionsProfile::apply_from_profile(widget);
}
