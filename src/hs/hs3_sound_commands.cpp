#include "halo/hs/hs3_commands.hpp"
#include "halo/sound/api.hpp"
#include "halo/hs/api.hpp"

extern "C" {
extern void *global_sound_effect_object;
extern void *directsound_listener;
extern float sound_listener_doppler_factor;
extern float sound_listener_rolloff_factor;
extern int16_t sound_supplementary_buffers_00746122;
extern uint8_t directsound_eax_enabled;
}

namespace halo::hs::part3 {

/**
 * Evaluate handler of the hs script function `sound_looping_set_scale`: reads its typed arguments from the
 * calling thread and hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x47fef0
 */
void SoundCommands::evaluate_sound_looping_set_scale(int16_t function_index, uint32_t thread_index, char first) const
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    halo::sound::sound_looping_set_scale((datum_index)arguments[0], *(float *)&arguments[1]);
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of the hs script function `sound_looping_start`: reads its typed arguments from the calling
 * thread and hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x47fe60
 */
void SoundCommands::evaluate_sound_looping_start(int16_t function_index, uint32_t thread_index, char first) const
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    halo::sound::sound_looping_start((datum_index)arguments[0], (datum_index)arguments[1], *(float *)&arguments[2]);
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of the hs script function `sound_looping_stop`: reads its typed arguments from the calling
 * thread and hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x47feb0
 */
void SoundCommands::evaluate_sound_looping_stop(int16_t function_index, uint32_t thread_index, char first) const
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    halo::sound::sound_looping_stop((datum_index)arguments[0]);
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of the hs script function `sound_set_effects_gain`: reads its typed arguments from the
 * calling thread and hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x480150
 */
void SoundCommands::evaluate_sound_set_effects_gain(int16_t function_index, uint32_t thread_index, char first) const
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        halo::sound::sound_set_effects_gain(*(float *)&arguments[0]);
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of the hs script function `sound_set_env`: reads its typed arguments from the calling thread
 * and hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x481600
 */
void SoundCommands::evaluate_sound_set_env(int16_t function_index, uint32_t thread_index, char first) const
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    void **vtable = *(void ***)global_sound_effect_object;

    ((void (__thiscall *)(void *, int32_t))vtable[0x18 / 4])(global_sound_effect_object, *(int16_t *)&arguments[0]);
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of the hs script function `sound_set_factor`: reads its typed arguments from the calling
 * thread and hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x4817a0
 */
void SoundCommands::evaluate_sound_set_factor(int16_t function_index, uint32_t thread_index, char first) const
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    float factor = *(float *)&arguments[0];
    void **vtable = *(void ***)directsound_listener;

    sound_listener_doppler_factor = factor;
    ((int32_t (__stdcall *)(void *, float, uint32_t))vtable[0x2c / 4])(directsound_listener, factor, 0);
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of the hs script function `sound_set_gain`: reads its typed arguments from the calling thread
 * and hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x47ac10
 */
void SoundCommands::evaluate_sound_set_gain(int16_t function_index, uint32_t thread_index, char first) const
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        float *gain = halo::hs::hs_sound_get_gain_reference((char *)arguments[0]);

        if (gain != 0) {
            *gain = *(float *)&arguments[1];
        }
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of the hs script function `sound_set_master_gain`: reads its typed arguments from the calling
 * thread and hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x480070
 */
void SoundCommands::evaluate_sound_set_master_gain(int16_t function_index, uint32_t thread_index, char first) const
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        halo::sound::sound_set_master_gain(*(float *)&arguments[0]);
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of the hs script function `sound_set_music_gain`: reads its typed arguments from the calling
 * thread and hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x4800e0
 */
void SoundCommands::evaluate_sound_set_music_gain(int16_t function_index, uint32_t thread_index, char first) const
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        halo::sound::sound_set_music_gain(*(float *)&arguments[0]);
        halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of the hs script function `sound_set_rolloff`: reads its typed arguments from the calling
 * thread and hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x481750
 */
void SoundCommands::evaluate_sound_set_rolloff(int16_t function_index, uint32_t thread_index, char first) const
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
    float factor = *(float *)&arguments[0];
    void **vtable = *(void ***)directsound_listener;

    sound_listener_rolloff_factor = factor;
    ((int32_t (__stdcall *)(void *, float, uint32_t))vtable[0x3c / 4])(directsound_listener, factor, 0);
    halo::hs::hs_thread_return(0, thread_index);
    }
}

/**
 * Evaluate handler of the hs script function `sound_set_supplementary_buffers`: reads its typed arguments from
 * the calling thread and hands the result back through the thread, exactly as the original handler did.
 *
 * @address 0x4816a0
 */
void SoundCommands::evaluate_sound_set_supplementary_buffers(int16_t function_index, uint32_t thread_index, char first) const
{
    hs_function_definition *definition = halo::hs::globals().function_definitions[function_index];
    int32_t *arguments = halo::hs::hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        int16_t count = (int16_t)arguments[0];
        uint8_t changed = 0;

        if (count < 0 || count > 2) {
            count = 2;
        }
        if (sound_supplementary_buffers_00746122 != count) {
            sound_supplementary_buffers_00746122 = count;
            changed = 1;
        }
        if ((uint8_t)arguments[1] && changed) {
            halo::sound::sound_driver_set_eax_enabled(directsound_eax_enabled, 1);
        }
        halo::hs::hs_thread_return(0, thread_index);
    }
}

}
