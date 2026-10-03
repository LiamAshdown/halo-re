/**
 * @file src/models/models_api.cpp
 * The models module's free-function API (include/halo/models/api.hpp), forwarding to the class implementations.
 */

#include <stdarg.h>
#include "halo/models/models.hpp"
#include "halo/models/api.hpp"

namespace halo::models {

animation_state_advance_result animation_state_advance(uint32_t animation_graph_tag_index, animation_state *state, int32_t *sound_tag_id, animation_random_stream random_stream)
{
    return halo::models::animation_graph::state_advance(animation_graph_tag_index, state, sound_tag_id, random_stream);
}

int16_t animation_choose_random_permutation(datum_index animation_graph_tag, int16_t first_animation, animation_random_stream stream)
{
    return halo::models::animation_graph::choose_random_permutation(animation_graph_tag, first_animation, stream);
}

}
