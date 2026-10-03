#include "halo/camera/camera.hpp"
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"

extern "C" {
extern director_globals camera_director_globals;
extern director directors[1];
extern camera_input_axis_definition camera_input_axes[4];
extern void camera_first_person_compute_pov(director_camera_data *data, camera_input *input, observer_command *command);
extern uint8_t controls_input_capture_flags;
extern player_globals *local_player_globals;
extern uint8_t director_camera_switching;
extern director_pov_proc director_last_pov_proc;
extern observer observers[1];
extern void camera_debug_compute_pov(director_camera_data *data, camera_input *input, observer_command *command);
extern void director_choose_gameplay_camera(int16_t local_player_index, uint8_t reset);
extern void director_set_flying_camera(int16_t local_player_index, uint8_t force);
extern uint8_t director_build_camera_input(int16_t local_player_index, camera_input *input);
extern uint8_t *hs_camera_control_pointer;
extern camera_script_globals camera_script;
extern player_control_globals *player_control_globals_ptr;
extern int16_t camera_get_seat_camera_state(datum_index unit, int16_t *out_state);
extern void camera_third_person_compute_pov(director_camera_data *data, camera_input *input, observer_command *command);
extern data_array *object_data;
extern data_array *player_data;
extern Scenario *global_scenario;
extern float observer_dt;
extern void camera_update(float dt);
extern void observer_set_command(int16_t local_player_index);
extern void observer_advance(int16_t local_player_index);
extern void observer_commit(int16_t local_player_index);
extern void editor_camera_set_position_and_direction(editor_camera_data *out, Vector3D *direction, Point3D *position);
extern void vector3d_compute_up_from_forward(Vector3D *forward, Vector3D *out_up);
extern void editor_camera_compute_pov(director_camera_data *data, camera_input *input, observer_command *command);
extern game_engine_definition *current_game_engine;
void camera_initialize(void);
void camera_control(uint8_t enable);
uint8_t camera_is_local_player_default_first_person(void);
void camera_script_set_animation(datum_index animation_tag, char *name);
datum_index camera_dead_find_next_teammate(datum_index reference_player, datum_index current_target, uint8_t require_same_team);
uint8_t camera_dead_player_has_teammate(datum_index reference_player);
void camera_debug_start(int16_t camera_point_index, int16_t ticks, datum_index relative_object);
void camera_debug_load_from_file(void);
void camera_debug_save_to_file(void);
dead_camera_data *dead_camera_new(dead_camera_data *self, int16_t local_player_index, datum_index unit);
}

namespace halo::camera {

/**
 * Resets the camera subsystem to its default state: following mode, first person as the active
 * pov procedure, a fresh transition, unit look scale, and every debug-look axis reset to its
 * definition's neutral value.
 *
 * Register convention in the original: __cdecl, no arguments (confirmed: no stack or register
 * reads before the first global store).
 *
 * @address 0x445580
 */
void CameraSystem::initialize()
{
    int i;

    camera_director_globals.mode = _director_camera_mode_following;
    camera_director_globals.mode_changed = 0;

    directors[0].unknown_50 = 0;
    directors[0].unknown_4c = 0;
    directors[0].transition_time = 0.0f;
    directors[0].data.raw[0] = 0;
    directors[0].data.raw[1] = 0;
    directors[0].data.raw[2] = 0;
    directors[0].data.raw[3] = 0;
    directors[0].pov_proc = camera_first_person_compute_pov;
    directors[0].look_scale = 1.0f;
    directors[0].unknown_c0 = 0;

    for (i = 0; i < k_camera_input_axis_count; i++) {
        directors[0].axes[i].value = camera_input_axes[i].reset_value;
        directors[0].axes[i].velocity = 0.0f;
        directors[0].axes[i].delta = 0.0f;
    }
}

/**
 * Top-level per-frame camera update: reads input, applies pending mode changes, computes the
 * active camera's point of view via its mode function pointer, and blends it into the
 * observer.
 *
 * Register convention in the original: __cdecl, one float stack parameter (dt).
 *
 * @address 0x445640
 */
void CameraSystem::update(float dt)
{
    camera_input input;
    observer_command command;

    director_camera_switching = (controls_input_capture_flags == 1);
    camera_director_globals.dt = dt;

    if (local_player_globals->local_players[0] == k_datum_index_none) {
        return;
    }

    directors[0].suppress_look_update = 0;
    directors[0].look_input_consumed = 0;
    director_build_camera_input(0, &input); 

    switch (camera_director_globals.mode) {
    case _director_camera_mode_following:
    case _director_camera_mode_orbiting:
        director_choose_gameplay_camera(0, camera_director_globals.mode_changed); 
        break;
    case _director_camera_mode_flying:
        director_set_flying_camera(0, camera_director_globals.mode_changed); 
        break;
    case _director_camera_mode_first_person:
        if (camera_director_globals.mode_changed != 0) {
            directors[0].data.first_person.field_of_view = 0.0f; 
            directors[0].pov_proc = camera_first_person_compute_pov;
            directors[0].look_scale = 1.0f;
            directors[0].unknown_c0 = 0;
        }
        break;
    default:
        break; 
    }

    camera_director_globals.mode_changed = 0;

    {
        uint8_t *zero = (uint8_t *)&command;
        int i;
        for (i = 0; i < (int)sizeof(command); i++) {
            zero[i] = 0;
        }
    }

    if (directors[0].pov_proc != (director_pov_proc)0 &&
        (directors[0].pov_proc != camera_debug_compute_pov ||
         local_player_globals->local_players[0] != k_datum_index_none)) {
        directors[0].pov_proc(&directors[0].data, &input, &command);
    }
    director_last_pov_proc = directors[0].pov_proc;

    if ((command.flags & 1) == 0) {
        directors[0].command.flags &= ~1u;
    } else {
        if (directors[0].transition_time != 0.0f) {
            if (directors[0].transition_time >= 0.2f || directors[0].pov_proc != camera_first_person_compute_pov) {
                if (command.timer <= directors[0].transition_time) {
                    command.timer = directors[0].transition_time;
                }
            } else {
                directors[0].transition_time = 0.0f;
                command.interpolation_flags[_observer_parameter_position] = 3;
                command.channel_times[_observer_parameter_position] = 0.0f;
                command.interpolation_flags[_observer_parameter_distance] = 3;
                command.channel_times[_observer_parameter_distance] = 0.0f;
            }
            directors[0].transition_time -= dt;
            if (directors[0].transition_time < 0.0f) {
                directors[0].transition_time = 0.0f;
            }
        }
        directors[0].command = command;
    }

    observers[0].updated = 0;
    observers[0].command = &directors[0].command;

    if (observers[0].has_command == 0) {
        directors[0].command.flags |= 8; 
        directors[0].command.channel_times[0] = 0.0f;
        directors[0].command.channel_times[1] = 0.0f;
        directors[0].command.channel_times[2] = 0.0f;
        directors[0].command.channel_times[3] = 0.0f;
        directors[0].command.channel_times[4] = 0.0f;
        directors[0].command.timer = 0.0f;
        observers[0].has_command = 1;
    }
}

/**
 * hs camera_control: true hands local player 0 to the scripted camera, false returns it to the
 * gameplay camera its seat wants.
 *
 * Register convention in the original: cdecl, one stack byte (mov al,[esp+0x8] after push
 * ecx).
 *
 * @address 0x445cc0
 */
void CameraSystem::control(uint8_t enable)
{
    int16_t seat_camera_state;

    *hs_camera_control_pointer = enable;
    if (enable) {
        directors[0].unknown_c0 = 0;
        directors[0].pov_proc = camera_debug_compute_pov;
        directors[0].look_scale = 1.0f;
        camera_script.camera_control = enable;
        camera_script.changed = 1;
        return;
    }

    {
        int16_t third_person = camera_get_seat_camera_state(
            player_control_globals_ptr->local_players[0].unit, &seat_camera_state);

        directors[0].unknown_c0 = 0;
        directors[0].look_scale = 1.0f;
        if (third_person == 1) {
            third_person_camera_data *third = &directors[0].data.third_person;

            third->initialized = 0;
            third->unknown_01 = 0;
            third->crouch_or_jump = 0;
            third->unknown_03 = 0;
            third->unknown_04 = 0;
            third->unit = k_datum_index_none;
            third->seat_index = -1;
            third->pitch_offset = 0.0f;
            third->yaw_offset = 0.0f;
            third->distance_scale = 1.0f;
            directors[0].pov_proc = camera_third_person_compute_pov;
        } else {
            directors[0].data.first_person.field_of_view = 0.0f;
            directors[0].pov_proc = camera_first_person_compute_pov;
        }
    }
    directors[0].seat_camera_state = seat_camera_state;
    camera_script.camera_control = enable;
    camera_script.changed = 1;
}

/**
 * True if the first local player exists and its active pov procedure is the default first
 * person one (i.e. not third person, scripted, dead, flying or editor).
 *
 * Register convention in the original: __cdecl, no arguments (confirmed: no stack or register
 * reads before the loop starts at index 0).
 *
 * @address 0x4455f0
 */
uint8_t CameraSystem::is_local_player_default_first_person()
{
    int16_t local_player_index;

    local_player_index = 0;
    while (local_player_index == -1 || local_player_index > 0 ||
           local_player_globals->local_players[local_player_index] == k_datum_index_none) {
        local_player_index++;
        if (local_player_index > 0) {
            return 0;
        }
    }

    return (directors[local_player_index].pov_proc == camera_first_person_compute_pov) ? 1 : 0;
}

/**
 * Original function camera_get_seat_camera_state; the author notes are in
 * docs/original/camera/camera_get_seat_camera_state.c.txt.
 *
 * Register convention in the original: unit handle in ECX (in_ECX); output
 * director_seat_camera_state as the single cdecl stack parameter (confirmed with objdump: `mov
 * ebp,[esp+8]` right after the prologue, `mov word ptr [ebp],di` writes through it before ECX
 * is even tested).
 *
 * @address 0x445b20
 */
int16_t CameraSystem::get_seat_camera_state(datum_index unit, int16_t *out_state)
{
    object_header *headers = (object_header *)object_data->data;
    object *unit_object;
    datum_index parent;
    int16_t result = 0;

    *out_state = _director_seat_camera_none;
    if (unit == k_datum_index_none) {
        return 0;
    }

    unit_object = headers[unit & 0xffff].data;
    parent = unit_object->parent_object;
    if (parent == k_datum_index_none) {
        return 0;
    }

    {
        object *parent_object = headers[parent & 0xffff].data;
        if ((1 << (parent_object->type & 0x1f)) & 3) {
            Unit *parent_unit_tag = (Unit *)halo::cache::globals().tag_instances[parent_object->definition_tag & 0xffff].data;
            uint8_t *seats = (uint8_t *)parent_unit_tag->seats.pointer;
            int16_t seat_index = ((unit_data *)((uint8_t *)unit_object + k_unit_data_offset))->vehicle_seat_index;
            uint32_t seat_flags = *(uint32_t *)(seats + (int32_t)seat_index * sizeof(UnitSeat));

            result = (seat_flags & 0x10) != 0; 

            if ((seat_flags & 0x40) != 0) { 
                if (((unit_data *)((uint8_t *)unit_object + k_unit_data_offset))->animation_state == _unit_animation_state_seat_enter) {
                    *out_state = _director_seat_camera_entering;
                    return 1;
                }
                if (((unit_data *)((uint8_t *)unit_object + k_unit_data_offset))->animation_state == _unit_animation_state_seat_exit) {
                    *out_state = _director_seat_camera_exiting;
                    return 1;
                }
            }
        }

        *out_state = _director_seat_camera_seated;
        return result;
    }
}

/**
 * hs camera animation set: looks up `name` (case-insensitively) among the ModelAnimations tag
 * `animation_tag`'s animations, and if found points camera_script_globals at it in animation
 * mode (camera_point_index -1, object none, field_of_view the 70 degree default,
 * time_remaining the animation's frame_count / 30 seconds). Does nothing if animation_tag is
 * k_datum_index_none, the tag has other than exactly one graph node, or no animation matches.
 *
 * Register convention in the original: `objdump -d -M intel --start-address=0x444b30 --stop-
 * address=0x444c00 bin/halo.exe`: EBX = animation_tag (a ModelAnimations tag datum_index,
 * tested against -1 at entry), one plain stack argument (the animation name to search for).
 *
 * @address 0x444b30
 */
void CameraSystem::script_set_animation(datum_index animation_tag, char *name)
{
    ModelAnimations *tag;
    int32_t index;
    ModelAnimationsAnimation *anim;

    if (animation_tag == k_datum_index_none) {
        return;
    }
    tag = (ModelAnimations *)halo::cache::globals().tag_instances[animation_tag & 0xffff].data;
    if (tag->nodes.count != 1) {
        return;
    }
    if (tag->animations.count <= 0) {
        return;
    }

    index = 0;
    for (;;) {
        anim = (ModelAnimationsAnimation *)((uint8_t *)tag->animations.pointer +
                                            index * sizeof(ModelAnimationsAnimation));
        if (_stricmp(name, anim->name.string) == 0) {
            break;
        }
        index = index + 1;
        if (tag->animations.count <= index) {
            return;
        }
    }

    camera_script.camera_point_index = -1;
    camera_script.object = k_datum_index_none;
    camera_script.mode = _camera_script_mode_animation;
    camera_script.changed = 1;
    camera_script.field_of_view = 1.2217305f; 
    camera_script.time_remaining = (real)(int32_t)(anim->frame_count / 30);
    camera_script.animation_tag = animation_tag;
    camera_script.animation_index = (int16_t)index;
}

/**
 * Original function camera_dead_find_next_teammate; the author notes are in
 * docs/original/camera/camera_dead_find_next_teammate.c.txt.
 *
 * Register convention in the original: two cdecl stack parameters (reference_player,
 * current_target) plus a third argument in BL (require_same_team), confirmed at the only call
 * site (camera_track_compute_pov 0x445380: `mov bl, al` right after the
 * camera_dead_player_has_teammate call, then `call.
 *
 * @address 0x4452c0
 */
datum_index CameraSystem::dead_find_next_teammate(datum_index reference_player, datum_index current_target, uint8_t require_same_team)
{
    data_iterator iterator;
    player *p;
    int32_t team;
    datum_index best;

    team = require_same_team
               ? ((player *)((uint8_t *)player_data->data +
                              (reference_player & 0xffff) * sizeof(player)))->team
               : -1;

    iterator.data = player_data;
    iterator.next_index = 0;
    iterator.index = k_datum_index_none;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
    best = k_datum_index_none;

    p = (player *)halo::memory::data_iterator_next(&iterator);
    while (p != (player *)0) {
        if (iterator.index != reference_player && p->unit != k_datum_index_none &&
            (!require_same_team || p->team == team)) {
            if (best == k_datum_index_none) {
                best = iterator.index;
            } else if ((int32_t)(iterator.index & 0xffff) > (int32_t)(current_target & 0xffff)) {
                best = iterator.index;
                break;
            }
        }
        p = (player *)halo::memory::data_iterator_next(&iterator);
    }

    return (best == k_datum_index_none) ? current_target : best;
}

/**
 * Original function camera_dead_player_has_teammate; the author notes are in
 * docs/original/camera/camera_dead_player_has_teammate.c.txt.
 *
 * Register convention in the original: single cdecl stack parameter (confirmed with objdump:
 * loaded from [esp+0x1c] right after the prologue, and the function returns with a bare
 * `ret`).
 *
 * @address 0x445240
 */
uint8_t CameraSystem::dead_player_has_teammate(datum_index reference_player)
{
    data_iterator iterator;
    player *p;
    int32_t team;

    team = ((player *)((uint8_t *)player_data->data +
                        (reference_player & 0xffff) * sizeof(player)))->team;

    iterator.data = player_data;
    iterator.next_index = 0;
    iterator.index = k_datum_index_none;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;

    p = (player *)halo::memory::data_iterator_next(&iterator);
    while (p != (player *)0) {
        if (iterator.index != reference_player && p->team == team) {
            return 1;
        }
        p = (player *)halo::memory::data_iterator_next(&iterator);
    }
    return 0;
}

/**
 * hs camera_set: point camera_script_globals at cutscene camera point `camera_point_index` of
 * the current scenario (position/orientation/fov), give it `ticks` to live and, when
 * `relative_object` is a real object, make the point relative to it. Then runs one camera
 * update tick and, if a local player already exists, forces the observer to process and commit
 * the new command immediately instead of waiting a frame.
 *
 * Register convention in the original: cutscene camera point index in AX (in_AX, 16-bit);
 * ticks (an int16: 0x444c1e movsx ecx,WORD PTR [ebp+0x8]) and the relative object are cdecl
 * stack parameters, confirmed with objdump (the function loads them at [ebp+8]/[ebp+0xc]
 * relative to the post-prologue.
 *
 * @address 0x444c00
 */
void CameraSystem::debug_start(int16_t camera_point_index, int16_t ticks, datum_index relative_object)
{
    ScenarioCutsceneCameraPoint *point;
    real_matrix4x3 matrix; 

    point = (ScenarioCutsceneCameraPoint *)((uint8_t *)global_scenario->cutscene_camera_points.pointer +
                                             camera_point_index * sizeof(ScenarioCutsceneCameraPoint));

    camera_script.mode = _camera_script_mode_point;
    camera_script.changed = 1;
    camera_script.position = point->position;
    camera_script.camera_point_index = camera_point_index;

    halo::math::matrix4x3_from_euler_angles(matrix, point->orientation.yaw, point->orientation.pitch,
                                 point->orientation.roll);
    camera_script.forward = *(Vector3D *)&matrix.forward;
    camera_script.up = *(Vector3D *)&matrix.up;

    if (point->field_of_view == 0.0f) {
        camera_script.field_of_view = 1.2217305f; 
    } else {
        camera_script.field_of_view = point->field_of_view;
    }

    camera_script.time_remaining = (float)(ticks / 30);
    camera_script.object = relative_object;

    camera_update(0.0f); 
    observer_dt = 0.0001f;

    if (local_player_globals->local_players[0] != k_datum_index_none) {
        observers[0].updated = 1;
        observer_set_command(0);      
        if (observer_dt != 0.0f) {
            observer_advance(0);      
        }
        observer_commit(0);           
    }
}

/**
 * Loads camera.txt and switches local player 0 to the editor camera at that position and
 * orientation. The roll is recovered as the signed angle between the saved up vector and the
 * roll-free up vector of the saved forward.
 *
 * Register convention in the original: none; cdecl, no arguments.
 *
 * @address 0x445940
 */
void CameraSystem::debug_load_from_file()
{
    float field_of_view;
    Vector3D saved_up;
    Vector3D computed_up;
    Vector3D forward;
    Point3D position;
    void *file = fopen("camera.txt", "r");

    if (file == 0) {
        return;
    }
    fscanf((FILE *)file, "%f %f %f\n", &position.x, &position.y, &position.z);
    fscanf((FILE *)file, "%f %f %f\n", &forward.i, &forward.j, &forward.k);
    fscanf((FILE *)file, "%f %f %f\n", &saved_up.i, &saved_up.j, &saved_up.k);
    fscanf((FILE *)file, "%f\n", &field_of_view);
    fclose((FILE *)file);

    editor_camera_set_position_and_direction(&directors[0].data.editor, &forward, &position);
    vector3d_compute_up_from_forward(&forward, &computed_up);
    directors[0].data.editor.roll =
        halo::math::vector3d_angle_between_4cd4f0(*((real_vector3d *)&saved_up), *((real_vector3d *)&computed_up));

    {
        
        float x = computed_up.k * saved_up.j - computed_up.j * saved_up.k;
        float y = saved_up.k * computed_up.i - computed_up.k * saved_up.i;
        float z = computed_up.j * saved_up.i - computed_up.i * saved_up.j;
        computed_up.i = x;
        computed_up.j = y;
        computed_up.k = z;
    }
    if (forward.k * computed_up.k + forward.j * computed_up.j + computed_up.i * forward.i > 0.0f) {
        directors[0].data.editor.roll = -directors[0].data.editor.roll;
    }

    directors[0].data.editor.field_of_view = field_of_view;
    directors[0].pov_proc = editor_camera_compute_pov;
    directors[0].look_scale = 1.0f;
    directors[0].unknown_c0 = 0;
    directors[0].unknown_00 = 2;
}

/**
 * Writes the local player's final camera (position, forward, up, field of view) to camera.txt.
 *
 * Register convention in the original: none; cdecl, no arguments.
 *
 * @address 0x445880
 */
void CameraSystem::debug_save_to_file()
{
    void *file = fopen("camera.txt", "w");
    observer_camera *camera = &observers[0].camera;

    if (file != 0) {
        fprintf((FILE *)file, "%f %f %f\n", (double)camera->position.x, (double)camera->position.y,
            (double)camera->position.z);
        fprintf((FILE *)file, "%f %f %f\n", (double)camera->forward.i, (double)camera->forward.j,
            (double)camera->forward.k);
        fprintf((FILE *)file, "%f %f %f\n", (double)camera->up.i, (double)camera->up.j, (double)camera->up.k);
        fprintf((FILE *)file, "%f\n", (double)camera->field_of_view);
        fclose((FILE *)file);
    }
}

/**
 * Original function dead_camera_new; the author notes are in
 * docs/original/camera/dead_camera_new.c.txt.
 *
 * Register convention in the original: this-pointer in EAX (in_EAX), local player index in DX
 * (in_DX, 16-bit); the target unit (or -1 to use the local player's own current unit) is the
 * single cdecl stack parameter.
 *
 * @address 0x4450e0
 */
dead_camera_data * DeadCamera::construct(dead_camera_data *self, int16_t local_player_index, datum_index unit)
{
    observer_camera *source;
    datum_index local_player;

    source = (local_player_index != -1) ? &observers[local_player_index].camera : (observer_camera *)0;
    self->focus = *(Point3D *)&source->position; 

    self->field_of_view = 1.2217305f; 

    halo::math::globals().effect_random_seed = halo::math::globals().effect_random_seed * k_random_multiplier + k_random_increment;
    self->distance = (real)(halo::math::globals().effect_random_seed >> k_random_value_shift) * 1.5259022e-05f * 4.0f + 2.0f;

    halo::math::globals().effect_random_seed = halo::math::globals().effect_random_seed * k_random_multiplier + k_random_increment;
    self->yaw = (real)(halo::math::globals().effect_random_seed >> k_random_value_shift) * 1.5259022e-05f * 6.2831855f;

    self->transition_time = 3.0f;

    halo::math::globals().effect_random_seed = halo::math::globals().effect_random_seed * k_random_multiplier + k_random_increment;
    self->pitch = -((real)(halo::math::globals().effect_random_seed >> k_random_value_shift) * 1.5259022e-05f * 0.6283184f + 0.47123894f);

    if (unit != k_datum_index_none) {
        self->retarget_time = 3.4028235e38f; 
    } else {
        self->retarget_time = (current_game_engine != (game_engine_definition *)0) ? 15.0f : 3.0f;
    }

    if (local_player_index == -1 || local_player_index > 0) {
        local_player = k_datum_index_none; 
    } else {
        local_player = local_player_globals->local_players[local_player_index];
    }
    self->local_player = local_player;

    if (unit == k_datum_index_none) {
        player *p = (player *)((uint8_t *)player_data->data + (local_player & 0xffff) * sizeof(player));
        self->target_unit = p->previous_unit; 
    } else {
        self->target_unit = unit;
    }
    self->target_player = local_player;

    return self;
}

}

extern "C" {

void camera_initialize(void)
{
    halo::camera::CameraSystem::initialize();
}

void camera_update(float dt)
{
    halo::camera::CameraSystem::update(dt);
}

void camera_control(uint8_t enable)
{
    halo::camera::CameraSystem::control(enable);
}

uint8_t camera_is_local_player_default_first_person(void)
{
    return halo::camera::CameraSystem::is_local_player_default_first_person();
}

int16_t camera_get_seat_camera_state(datum_index unit, int16_t *out_state)
{
    return halo::camera::CameraSystem::get_seat_camera_state(unit, out_state);
}

void camera_script_set_animation(datum_index animation_tag, char *name)
{
    halo::camera::CameraSystem::script_set_animation(animation_tag, name);
}

datum_index camera_dead_find_next_teammate(datum_index reference_player, datum_index current_target, uint8_t require_same_team)
{
    return halo::camera::CameraSystem::dead_find_next_teammate(reference_player, current_target, require_same_team);
}

uint8_t camera_dead_player_has_teammate(datum_index reference_player)
{
    return halo::camera::CameraSystem::dead_player_has_teammate(reference_player);
}

void camera_debug_start(int16_t camera_point_index, int16_t ticks, datum_index relative_object)
{
    halo::camera::CameraSystem::debug_start(camera_point_index, ticks, relative_object);
}

void camera_debug_load_from_file(void)
{
    halo::camera::CameraSystem::debug_load_from_file();
}

void camera_debug_save_to_file(void)
{
    halo::camera::CameraSystem::debug_save_to_file();
}

dead_camera_data *dead_camera_new(dead_camera_data *self, int16_t local_player_index, datum_index unit)
{
    return halo::camera::DeadCamera::construct(self, local_player_index, unit);
}

}
