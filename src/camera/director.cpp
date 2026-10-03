#include "halo/camera/director.hpp"

extern "C" {
extern void *mouse_device;
extern uint8_t input_suppressed;
extern mouse_state live_mouse_state;
extern mouse_state mouse_neutral_state;
extern uint8_t director_camera_switching;
extern director_globals camera_director_globals;
extern director directors[1];
extern uint8_t input_get_key_state(int16_t key_index);
extern void camera_input_axes_update(int16_t local_player_index, uint32_t key_bits, float zoom);
extern void camera_first_person_compute_pov(director_camera_data *data, camera_input *input, observer_command *command);
extern void camera_third_person_compute_pov(director_camera_data *data, camera_input *input, observer_command *command);
extern uint8_t *hs_camera_control_pointer;
extern player_globals *local_player_globals;
extern data_array *player_data;
extern player_control_globals *player_control_globals_ptr;
extern void director_update_seat_camera(int16_t local_player_index, uint8_t force);
extern int16_t camera_get_seat_camera_state(datum_index unit, int16_t *out_state);
extern dead_camera_data *dead_camera_new(dead_camera_data *self, int16_t local_player_index, datum_index unit);
extern void camera_track_compute_pov(director_camera_data *data, camera_input *input, observer_command *command);
extern void flying_camera_initialize(editor_camera_data *data, int16_t local_player_index);
extern void flying_camera_compute_pov(director_camera_data *data, camera_input *input, observer_command *command);
extern void camera_debug_compute_pov(director_camera_data *data, camera_input *input, observer_command *command);
extern camera_input_axis_definition camera_input_axes[4];
extern double pow(double base, double exponent);
extern void camera_initialize(void);
extern void camera_control(uint8_t enable);
uint8_t director_build_camera_input(int16_t local_player_index, camera_input *input);
void director_choose_gameplay_camera(int16_t local_player_index, uint8_t reset);
void director_set_flying_camera(int16_t local_player_index, uint8_t force);
int16_t camera_get_type_for_player(int16_t local_player_index);
void director_game_state_loaded(void);
}

namespace {

static uint8_t camera_input_key_held(uint32_t key_bits, int16_t bit)
{
    return bit != -1 && (key_bits & (1u << (bit & 0x1f))) != 0;
}

}

namespace halo::camera {

/**
 * Original function director_build_camera_input; the author notes are in
 * docs/original/camera/director_build_camera_input.c.txt.
 *
 * @address 0x445f90
 */
uint8_t DirectorHandle::build_camera_input(camera_input *input)
{
    int16_t local_player_index = (int16_t)player_handle;

    director *director = &directors[local_player_index];
    mouse_state *mouse;
    uint32_t key_bits;
    uint8_t result;
    uint32_t *zero = (uint32_t *)input;
    int32_t i;

    for (i = 0; i < (int32_t)(sizeof(camera_input) / sizeof(uint32_t)); i++) {
        zero[i] = 0;
    }
    input->local_player_index = local_player_index;
    input->dt = camera_director_globals.dt;

    if (mouse_device == 0) {
        return 0;
    }
    if (!director_camera_switching) {
        return 0;
    }
    mouse = input_suppressed ? &mouse_neutral_state : &live_mouse_state;
    result = (input_get_key_state(0x1d) == 1);

    if (director->pov_proc == camera_first_person_compute_pov ||
        director->pov_proc == camera_third_person_compute_pov ||
        mouse->button_frames[1] == 0) {
        return result;
    }

    key_bits = (input_get_key_state(0x20) != 0);
    if (input_get_key_state(0x2e)) key_bits |= 0x02; else key_bits &= ~0x02u;
    if (input_get_key_state(0x2d)) key_bits |= 0x04; else key_bits &= ~0x04u;
    if (input_get_key_state(0x2f)) key_bits |= 0x08; else key_bits &= ~0x08u;
    if (input_get_key_state(0x22)) key_bits |= 0x10; else key_bits &= ~0x10u;
    if (input_get_key_state(0x30)) key_bits |= 0x20; else key_bits &= ~0x20u;
    if (input_get_key_state(0x23)) key_bits |= 0x40; else key_bits &= ~0x40u;
    if (input_get_key_state(0x31)) key_bits |= 0x80; else key_bits &= ~0x80u;

    camera_input_axes_update(local_player_index, key_bits, (float)mouse->wheel);

    input->yaw_delta = (float)mouse->x * -0.0031415927f;   
    input->pitch_delta = (float)mouse->y * 0.0031415927f;  
    input->roll_delta += director->axes[1].delta;
    input->zoom_delta = (float)mouse->wheel;
    input->move_forward += director->axes[2].delta;
    input->move_left += director->axes[3].delta;
    input->has_look_input = 1;
    input->move_up += director->axes[0].delta;
    director->look_input_consumed = 1;
    director->suppress_look_update = 1;
    return result;
}

/**
 * Original function director_choose_gameplay_camera; the author notes are in
 * docs/original/camera/director_choose_gameplay_camera.c.txt.
 *
 * @address 0x445dc0
 */
void DirectorHandle::choose_gameplay_camera(uint8_t reset)
{
    int16_t local_player_index = (int16_t)player_handle;

    director *director = &directors[local_player_index];
    datum_index player_index;
    player *player_record;
    uint8_t player_is_dead;

    if (reset) {
        director->data.first_person.field_of_view = 0.0f;
        director->pov_proc = camera_first_person_compute_pov;
        director->look_scale = 1.0f;
        director->unknown_c0 = 0;
        return;
    }

    if (local_player_index == -1 || local_player_index >= 1) {
        player_index = k_datum_index_none;
    } else {
        player_index = local_player_globals->local_players[local_player_index];
    }
    
    player_record = (player *)((uint8_t *)player_data->data + (player_index & 0xffff) * sizeof(player));
    player_is_dead = (player_record->unit == k_datum_index_none && player_record->deaths > 0);

    if (*hs_camera_control_pointer != 0) {
        return;
    }

    director_update_seat_camera(local_player_index, 0);

    if (player_is_dead) {
        if (director->pov_proc != camera_track_compute_pov) {
            dead_camera_new(&director->data.dead, local_player_index, k_datum_index_none);
            director->unknown_c0 = 0;
            director->pov_proc = camera_track_compute_pov;
            director->look_scale = 1.0f;
            director->transition_time = 1.0f;
        }
    } else if (director->pov_proc == camera_track_compute_pov) {
        int16_t seat_camera_state;
        int16_t third_person = camera_get_seat_camera_state(
            player_control_globals_ptr->local_players[local_player_index].unit, &seat_camera_state);

        if (third_person == 1) {
            third_person_camera_data *third = &director->data.third_person;

            third->unit = k_datum_index_none;
            third->seat_index = -1;
            third->initialized = 0;
            third->unknown_01 = 0;
            third->crouch_or_jump = 0;
            third->unknown_03 = 0;
            third->unknown_04 = 0;
            third->pitch_offset = 0.0f;
            third->yaw_offset = 0.0f;
            third->distance_scale = 1.0f;
            director->look_scale = 1.0f;
            director->pov_proc = camera_third_person_compute_pov;
            director->unknown_c0 = 0;
            director->seat_camera_state = seat_camera_state;
        } else {
            director->data.first_person.field_of_view = 0.0f;
            director->pov_proc = camera_first_person_compute_pov;
            director->look_scale = 1.0f;
            director->unknown_c0 = 0;
            director->seat_camera_state = seat_camera_state;
        }
    }
}

/**
 * Original function director_set_flying_camera; the author notes are in
 * docs/original/camera/director_set_flying_camera.c.txt.
 *
 * @address 0x445f40
 */
void DirectorHandle::set_flying_camera(uint8_t force)
{
    int16_t local_player_index = (int16_t)player_handle;

    director *director = &directors[local_player_index];

    if (force || director->pov_proc != flying_camera_compute_pov) {
        flying_camera_initialize(&director->data.editor, local_player_index);
        director->pov_proc = flying_camera_compute_pov;
        director->look_scale = 1.0f;
        director->unknown_c0 = 0;
    }
}

/**
 * Original function director_update_seat_camera; the author notes are in
 * docs/original/camera/director_update_seat_camera.c.txt.
 *
 * @address 0x445c00
 */
void DirectorHandle::update_seat_camera(uint8_t force)
{
    int16_t local_player_index = (int16_t)player_handle;

    director *director = &directors[local_player_index];
    int16_t seat_camera_state;
    int16_t third_person = camera_get_seat_camera_state(
        player_control_globals_ptr->local_players[local_player_index].unit, &seat_camera_state);

    if (!force && director->seat_camera_state == seat_camera_state) {
        return;
    }

    if (third_person == 1) {
        if (force || director->pov_proc == camera_first_person_compute_pov) {
            third_person_camera_data *third = &director->data.third_person;

            third->unit = k_datum_index_none;
            third->seat_index = -1;
            third->initialized = 0;
            third->unknown_01 = 0;
            third->crouch_or_jump = 0;
            third->unknown_03 = 0;
            third->unknown_04 = 0;
            third->pitch_offset = 0.0f;
            third->yaw_offset = 0.0f;
            third->distance_scale = 1.0f;
            director->pov_proc = camera_third_person_compute_pov;
            goto switched;
        }
    } else {
        if (force || director->pov_proc == camera_third_person_compute_pov) {
            director->data.first_person.field_of_view = 0.0f;
            director->pov_proc = camera_first_person_compute_pov;
            goto switched;
        }
    }
    director->seat_camera_state = seat_camera_state;
    return;

switched:
    director->look_scale = 1.0f;
    director->unknown_c0 = 0;
    if (!force) {
        director->transition_time = 1.0f;
    }
    director->seat_camera_state = seat_camera_state;
}

/**
 * Original function camera_get_type_for_player; the author notes are in
 * docs/original/camera/camera_get_type_for_player.c.txt.
 *
 * Register convention in the original: local player index in CX (in_CX); no stack parameters.
 *
 * @address 0x445ac0
 */
int16_t DirectorHandle::get_type_for_player()
{
    int16_t local_player_index = (int16_t)player_handle;

    director *d = &directors[local_player_index];

    if (d->pov_proc == camera_first_person_compute_pov) {
        if (d->transition_time == 0.0f) { 
            d->camera_type = _director_camera_type_first_person;
        }
    } else if (d->pov_proc == camera_third_person_compute_pov) {
        d->camera_type = _director_camera_type_third_person;
    } else {
        d->camera_type = (d->pov_proc != camera_debug_compute_pov) ? _director_camera_type_other
                                                                    : _director_camera_type_scripted;
    }

    return d->camera_type;
}

/**
 * Original function camera_input_axes_update; the author notes are in
 * docs/original/camera/camera_input_axes_update.c.txt.
 *
 * @address 0x446170
 */
void DirectorHandle::input_axes_update(uint32_t key_bits, float zoom)
{
    int16_t local_player_index = (int16_t)player_handle;

    director *director = &directors[local_player_index];
    camera_input_axis_definition *definition = camera_input_axes;
    camera_input_axis_state *state = director->axes;
    float look_scale;
    int32_t count;

    look_scale = (float)(pow((double)1.3f, (double)zoom) * director->look_scale);
    director->look_scale = look_scale;
    if (look_scale < 0.01f) {
        look_scale = 0.01f;
    } else if (look_scale > 50.0f) {
        look_scale = 50.0f;
    }
    director->look_scale = look_scale;

    for (count = 4; count != 0; count--, definition++, state++) {
        float scale = camera_input_axes[0].scale_by_zoom ? director->look_scale : 1.0f;
        float damping = camera_director_globals.dt * 5.0f;
        uint8_t decrease;
        uint8_t increase;
        uint8_t reset;
        float velocity;
        float value;

        if (damping < 0.0f) {
            damping = 0.0f;
        } else if (damping > 1.0f) {
            damping = 1.0f;
        }
        decrease = camera_input_key_held(key_bits, definition->decrease_key_bit);
        increase = camera_input_key_held(key_bits, definition->increase_key_bit);
        reset = camera_input_key_held(key_bits, definition->reset_key_bit);

        velocity = (1.0f - damping) * state->velocity;
        state->velocity = velocity;
        if (decrease) {
            if (!increase) {
                state->velocity = velocity -
                    camera_director_globals.dt * definition->acceleration * scale * 25.0f;
            }
        } else if (increase) {
            state->velocity = camera_director_globals.dt * definition->acceleration * scale * 25.0f +
                velocity;
        }

        state->delta = camera_director_globals.dt * state->velocity;
        if (reset) {
            state->value = definition->reset_value;
        } else {
            state->value = state->delta + state->value;
        }

        value = state->value;
        if (value < definition->minimum_value) {
            value = definition->minimum_value;
        } else if (value > definition->maximum_value) {
            value = definition->maximum_value;
        }
        state->value = value;
    }
}

/**
 * Original function director_game_state_loaded; the author notes are in
 * docs/original/camera/director_game_state_loaded.c.txt.
 *
 * Register convention in the original: none; cdecl, no arguments.
 *
 * @address 0x445560
 */
void DirectorEvents::game_state_loaded()
{
    camera_initialize();
    camera_control(*hs_camera_control_pointer);
}

}

extern "C" {

uint8_t director_build_camera_input(int16_t local_player_index, camera_input *input)
{
    return halo::camera::DirectorHandle(local_player_index).build_camera_input(input);
}

void director_choose_gameplay_camera(int16_t local_player_index, uint8_t reset)
{
    halo::camera::DirectorHandle(local_player_index).choose_gameplay_camera(reset);
}

void director_set_flying_camera(int16_t local_player_index, uint8_t force)
{
    halo::camera::DirectorHandle(local_player_index).set_flying_camera(force);
}

void director_update_seat_camera(int16_t local_player_index, uint8_t force)
{
    halo::camera::DirectorHandle(local_player_index).update_seat_camera(force);
}

int16_t camera_get_type_for_player(int16_t local_player_index)
{
    return halo::camera::DirectorHandle(local_player_index).get_type_for_player();
}

void camera_input_axes_update(int16_t local_player_index, uint32_t key_bits, float zoom)
{
    halo::camera::DirectorHandle(local_player_index).input_axes_update(key_bits, zoom);
}

void director_game_state_loaded(void)
{
    halo::camera::DirectorEvents::game_state_loaded();
}

}
