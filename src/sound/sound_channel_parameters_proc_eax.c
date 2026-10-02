// sound_channel_parameters_proc_eax  (not a Ghidra function; sound_channel_parameters_proc)
// address 0x54cf80, size 160 bytes
// name confidence: 0.7  rewrite confidence: 0.85
// evidence: sound_initialize 0x54d140 and sound_reopen_device store 0x54cf80 in
//   sound_channel_parameters_proc_ptr when the EAX path is chosen (0x54ce50 otherwise). Only
//   reachable through that pointer; written with its sibling for the first-boot track.
// objdump 0x54cf80..0x54d01f: first_person comes from the channel's sound (sound_data, stride 0xb0),
//   0 without one. Unless `update` the channel's current pitch takes parameters->pitch. A
//   first-person sound of class 4 has its gain scaled by 1.2 (double 0x672cc8) and capped at 0.6
//   (double 0x672cc0). Then the driver's channel_set_parameters (+0x34) with (channel, parameters,
//   update, first_person, class); the driver (0x5484d0) reads only the first three.
// blam-cc: stack -> channel_index, parameters, update, sound_class (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "sound.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern sound_channel sound_channels[k_maximum_sound_channels]; // 0x00724a60
extern data_array *sound_data; // 0x007252c0, "sounds" 0x200 x 0xb0
extern sound_driver *current_sound_driver; // 0x00725208

void sound_channel_parameters_proc_eax(int16_t channel_index, sound_channel_parameters *parameters,
    uint8_t update, int16_t sound_class)
{
    sound_channel *channel = &sound_channels[channel_index];
    uint8_t first_person = 0;

    if (channel->sound_index != k_datum_index_none) {
        sound *self = (sound *)((uint8_t *)sound_data->data + (channel->sound_index & 0xffff) * 0xb0);

        if (self != 0) {
            first_person = self->first_person;
        }
    }
    if (!update) {
        channel->current_pitch = parameters->pitch;
    }
    if (first_person && sound_class == 4) {
        double gain = parameters->gain * 1.2;

        if (!(gain <= 0.6)) {
            gain = 0.6;
        }
        parameters->gain = (float)gain;
    }
    ((void (*)(int16_t, sound_channel_parameters *, uint8_t, uint8_t, int16_t))
        current_sound_driver->channel_set_parameters)(channel_index, parameters, update, first_person, sound_class);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
