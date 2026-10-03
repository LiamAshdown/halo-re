#include "halo/cutscene/api.hpp"

extern "C" {
extern cinematic_globals *cinematic_globals_ptr;
extern float cinematic_saved_music_gain;
extern cinematic_screen_effect_globals *cinematic_screen_effect_state;
}

namespace halo::cutscene {

Globals &globals()
{
    static Globals instance{::cinematic_globals_ptr, ::cinematic_saved_music_gain, ::cinematic_screen_effect_state};
    return instance;
}
}
