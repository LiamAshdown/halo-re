#pragma once

/* Include after the engine type headers (types/*.h carry no include guards). */

namespace halo::game::engine1 {

/**
 * Local player control input digitisation and look vector helpers. Stateless service class: every function is
 * a static member and the state it acts on lives in the engine globals.
 */
class LocalControl {
public:
    static void build_local_player_control_input(int16_t local_player_index, real delta_time, player_control_input *out);
    static void compute_local_player_look_vector(real_vector3d *out_forward, int16_t local_player_index);
    static void compute_look_angles_from_vector(real_vector3d *facing, int16_t local_player_index);
    static void digitize_control_input(player_control_input *input);
    static real get_max_look_pitch(int16_t local_player_index);
    static void init_player_look_state_from_object(datum_index unit, int16_t local_player_index);

private:
    static real control_input_absolute(real value);
};

}
