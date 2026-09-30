// sound_channel_parameters_proc_default  (not a Ghidra function; sound_channel_parameters_proc)
// address 0x54ce50, size 304 bytes
// name confidence: 0.7  rewrite confidence: 0.85
// evidence: sound_initialize 0x54d140 and sound_reopen_device store 0x54ce50 in
//   sound_channel_parameters_proc_ptr (the non-EAX choice; 0x54cf80 is the EAX one). Only reachable
//   through that pointer. First-boot track: the UI map's sounds run it.
// objdump 0x54ce50..0x54cf7f: the channel's sound (sound_data, stride 0xb0) or none. Unless `update`
//   the channel's current pitch takes parameters->pitch. For a sound at an absolute location
//   (location.type 1) the gain is rebuilt in millibels (ESI minimum -10000 for every call):
//     a = mB(1 - occlusion), c = ftol(mB(gain) + a * 0.1), e = ftol(mB(1 - obstruction) * 0.1 + c)
//   (a * 0.1 is stored as a float, c reloaded as a float), e clamped to -10000..0, and
//   parameters->gain = sound_evaluate_volume_curve(e, 0, 1). Then the driver's
//   channel_set_parameters (+0x34) with (channel, parameters, update, first_person, class); the
//   last two are pushed but the driver (0x5484d0) reads only the first three.
// blam-cc: stack -> channel_index, parameters, update, sound_class (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "sound.h"
#include "fn_sound.h"

extern sound_channel sound_channels[k_maximum_sound_channels]; // 0x00724a60
extern data_array *sound_data; // 0x007252c0, "sounds" 0x200 x 0xb0
extern sound_driver *current_sound_driver; // 0x00725208


    // 0x54cda0, blam-cc: ESI -> minimum, stack -> (gain, bias_and_maximum)


void sound_channel_parameters_proc_default(int16_t channel_index, sound_channel_parameters *parameters,
    uint8_t update, int16_t sound_class)
{
    sound_channel *channel = &sound_channels[channel_index];
    sound *self = 0;
    uint8_t first_person;

    if (channel->sound_index != k_datum_index_none) {
        self = (sound *)((uint8_t *)sound_data->data + (channel->sound_index & 0xffff) * 0xb0);
    }
    if (!update) {
        channel->current_pitch = parameters->pitch;
    }
    if (self == 0) {
        first_person = 0;
    } else {
        if (self->location.type == _sound_location_absolute) {
            float scaled;
            int32_t combined;
            int32_t attenuation;

            scaled = (float)((double)sound_linear_gain_to_millibels_clamped(-10000, 1.0f - self->location.occlusion, 0) *
                0.1f);
            combined = (int32_t)((double)sound_linear_gain_to_millibels_clamped(-10000, parameters->gain, 0) +
                scaled);
            scaled = (float)combined;
            attenuation = (int32_t)((double)sound_linear_gain_to_millibels_clamped(-10000,
                1.0f - self->location.obstruction, 0) * 0.1f + scaled);
            if (attenuation < -10000) {
                attenuation = -10000;
            } else if (attenuation > 0) {
                attenuation = 0;
            }
            parameters->gain = sound_evaluate_volume_curve(attenuation, 0, 1);
        }
        first_person = self->first_person;
    }
    ((void (*)(int16_t, sound_channel_parameters *, uint8_t, uint8_t, int16_t))
        current_sound_driver->channel_set_parameters)(channel_index, parameters, update, first_person, sound_class);
}
