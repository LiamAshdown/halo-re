#include "halo/cutscene/api.hpp"
#include "halo/core/link.hpp"
#include "halo/cutscene/vars.hpp"
#include "halo/game/vars.hpp"

static auto &cinematic_globals_ptr = halo::link::ref<cinematic_globals *>(halo::game::vars().cinematic_globals_ptr);
static auto &cinematic_saved_music_gain = halo::link::ref<float>(halo::cutscene::vars().cinematic_saved_music_gain);
static auto &cinematic_screen_effect_state = halo::link::ref<cinematic_screen_effect_globals *>(halo::cutscene::vars().cinematic_screen_effect_state);

namespace halo::cutscene {

Globals &globals()
{
    static Globals instance{::cinematic_globals_ptr, ::cinematic_saved_music_gain, ::cinematic_screen_effect_state};
    return instance;
}
}
