/**
 * @file src/shaders/shaders_api.cpp
 * The shaders module's free-function API (include/halo/shaders/api.hpp), forwarding to the class implementations.
 */

#include <stdarg.h>
#include "halo/shaders/shaders.hpp"
#include "halo/shaders/api.hpp"

namespace halo::shaders {

int16_t chimera__shader_get_vertex_shader_permutation(Shader *shader)
{
    return halo::shaders::shader_view(shader).vertex_shader_permutation();
}

uint8_t shader_is_decal(Shader *shader)
{
    return halo::shaders::shader_view(shader).is_decal();
}

uint8_t shader_draw_before_water(Shader *shader)
{
    return halo::shaders::shader_view(shader).draw_before_water();
}

int16_t numeric_countdown_timer_get_digit(int16_t digit_index)
{
    return halo::shaders::numeric_countdown_timer::get_digit(digit_index);
}

void numeric_countdown_timer_update(void)
{
    halo::shaders::numeric_countdown_timer::update();
}

}
