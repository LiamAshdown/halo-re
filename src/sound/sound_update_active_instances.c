// sound_update_active_instances  (Ghidra: sound_update_active_instances, already named)
// address 0x54c900, size 1058 bytes
// name confidence: 0.5   rewrite confidence: 0.75
// evidence: out/phase4/sound_functions.md "Per-tick update of every active playback channel:
// repositions/attenuates it in 3D, applies gain via the instant or looping gain path, and
// retires finished channels."; driver->channel_set_spatial (vtable+0x30, types/sound.h) matches
// its 7-argument calls here exactly; the 2D-channel distance attenuation reuses
// minimum_distance/maximum_distance with the same class-default fallback pattern as every other
// function in this module; play_state==0 dispatches to sound_update_instance_gain (0x54c750),
// otherwise to sound_update_looping_gain (0x54deb0), matching sound_play_state's own
// _sound_play_impulse == 0.
// register convention: void, no parameters.
// Phase-4 review (disassembly appended below) resolved the register arguments: the inverse
// point transform is (ECX m = &sound_listeners[listener_index].scale, EDX out, ESI point), the
// normal/vector transforms are (EAX out, EDX in, stack m); the doppler velocity is
// location.velocity (+0x24, the draft used forward) times 30 (ticks to seconds) minus the
// listener velocity; the 2D distance attenuation also uses the sound's own listener_index; the
// lip-sync sample index is __ftol(sound_channel.play_time); the lip-sync target is the sound's
// owner object (object_try_and_get(owner, 3) in ECX, then unit_accumulate_clamped_offset with
// EAX = owner). The binary does not test current_permutation for NULL before reading its
// mouth data; the rewrite keeps a NULL guard.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "sound.h"
#include "fn_sound.h"
#include "fn_units.h"

extern int16_t sound_channel_count;   // 0x007252b4
extern sound_channel sound_channels[k_maximum_sound_channels]; // 0x00724a60
extern data_array *sound_data;        // 0x007252c0, "sounds" 0x200 x 0xb0
extern tag_instance *tag_instances;   // 0x0087bc14
extern sound_class_definition sound_class_definitions[k_maximum_sound_classes]; // 0x0069eae0
extern sound_driver *current_sound_driver; // 0x00725208, header calls this "sound_driver"
extern sound_listener sound_listeners[1]; // 0x00725218
extern data_array *game_looping_sound_data; // 0x007461a0

extern const real_point3d *global_zero_vector3d_pointer; // 0x006966f8
extern const real_vector3d *global_forward3d_pointer;   // 0x00696718
extern const real_point3d *global_origin3d_pointer;    // 0x00696714


extern void sound_instance_stop(datum_index sound_handle); // this module, 0x54b180


extern void matrix4x3_inverse_transform_point(real_matrix4x3 *m, real_point3d *out, real_point3d *point); // 0x4cbf80, math module
extern void matrix4x3_inverse_transform_normal(real_vector3d *out, real_vector3d *normal, real_matrix4x3 *m); // 0x4cc080, math module
extern void matrix4x3_inverse_transform_vector(real_vector3d *out, real_vector3d *v, real_matrix4x3 *m); // 0x4cc010, math module
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0

extern double sqrt(double x); // FSQRT, Ghidra's SQRT() pseudo-function

// Per-update pass over every active playback channel: retires ones whose fade has reached
// silence, computes/applies 3D spatialization (or listener-relative distance attenuation for
// non-3D channels), applies gain through the one-shot or looping path depending on play_state,
// and drives mouth-data lip sync for dialog-class channels using the object marker location proc.
void sound_update_active_instances(void)
{
    int16_t channel_index;
    datum_index sound_handle;
    sound *instance;
    Sound *definition;

    for (channel_index = 0; channel_index < sound_channel_count; channel_index++) {
        sound_handle = sound_channels[channel_index].sound_index;
        if (sound_handle == 0xffffffff) {
            continue;
        }

        instance = (sound *)((uint8_t *)sound_data->data + (sound_handle & 0xffff) * sizeof(sound));
        definition = (Sound *)tag_instances[instance->definition_index & 0xffff].data;

        {
            float gain = sound_evaluate_fade_gain(sound_handle);

            if (gain == 0.0f && instance->fade_end_gain == 0.0f) {
                sound_instance_stop(sound_handle);
                sound_channels[channel_index].sound_index = 0xffffffff;
                continue;
            }

            if (!(sound_channels[channel_index].type_flags & _sound_channel_3d_bit)) {
                // Non-3D channel: apply listener-relative distance attenuation instead of full
                // spatialization.
                if (instance->location.type == _sound_location_absolute ||
                    instance->location.type == _sound_location_listener_relative) {
                    real_point3d transformed = *(real_point3d *)&instance->location.position;
                    float min_distance, max_distance, distance, attenuation;

                    if (instance->location.type == _sound_location_absolute) {
                        matrix4x3_inverse_transform_point((real_matrix4x3 *)&sound_listeners[instance->listener_index].scale,
                            &transformed, (real_point3d *)&instance->location.position);
                    }

                    distance = (float)sqrt((double)(transformed.x * transformed.x + transformed.y * transformed.y +
                        transformed.z * transformed.z));
                    min_distance = definition->minimum_distance;
                    if (min_distance == 0.0f) {
                        min_distance = sound_class_definitions[definition->sound_class].default_minimum_distance;
                    }
                    max_distance = definition->maximum_distance;
                    if (max_distance == 0.0f) {
                        max_distance = sound_class_definitions[definition->sound_class].default_maximum_distance;
                    }
                    attenuation = 1.0f - (distance - min_distance) / (max_distance - min_distance);
                    if (attenuation < 0.0f) {
                        attenuation = 0.0f;
                    } else if (attenuation > 1.0f) {
                        attenuation = 1.0f;
                    }
                    gain = attenuation * gain;
                }
            } else if (instance->location.type == _sound_location_absolute) {
                real_matrix4x3 *listener_matrix = (real_matrix4x3 *)&sound_listeners[instance->listener_index].scale;
                if (!instance->first_person) {
                    real_point3d transformed_position;
                    real_vector3d transformed_forward;
                    real_vector3d transformed_velocity;
                    sound_channel_spatial spatial;

                    matrix4x3_inverse_transform_point(listener_matrix, &transformed_position,
                        (real_point3d *)&instance->location.position);
                    matrix4x3_inverse_transform_normal(&transformed_forward, (real_vector3d *)&instance->location.forward,
                        listener_matrix);
                    matrix4x3_inverse_transform_vector(&transformed_velocity, (real_vector3d *)&instance->location.velocity,
                        listener_matrix);
                    spatial.position = *(Point3D *)&transformed_position;
                    spatial.forward = *(Vector3D *)&transformed_forward;
                    spatial.velocity.i = transformed_velocity.i * 30.0f - sound_listeners[instance->listener_index].velocity.i;
                    spatial.velocity.j = transformed_velocity.j * 30.0f - sound_listeners[instance->listener_index].velocity.j;
                    spatial.velocity.k = transformed_velocity.k * 30.0f - sound_listeners[instance->listener_index].velocity.k;

                    current_sound_driver->channel_set_spatial(channel_index, 1, &spatial,
                        instance->location.obstruction, instance->location.occlusion,
                        sound_listeners[instance->listener_index].underwater, definition->sound_class);
                } else {
                    sound_channel_spatial default_spatial;
                    default_spatial.position = *(Point3D *)global_zero_vector3d_pointer;
                    default_spatial.forward = *(Vector3D *)global_forward3d_pointer;
                    default_spatial.velocity = *(Vector3D *)global_origin3d_pointer;
                    current_sound_driver->channel_set_spatial(channel_index, 0, &default_spatial, 0.0f, 0.0f,
                        sound_listeners[instance->listener_index].underwater, definition->sound_class);
                }
            } else if (instance->location.type == _sound_location_listener_relative) {
                current_sound_driver->channel_set_spatial(channel_index, 1,
                    (sound_channel_spatial *)&instance->location.position, 0.0f, 0.0f, 0, definition->sound_class);
            }

            if (instance->play_state == _sound_play_impulse) {
                sound_update_instance_gain(channel_index, gain);
            } else {
                sound_update_looping_gain(channel_index, gain);
            }

            if (sound_class_definitions[definition->sound_class].dialog != 0 &&
                instance->location_proc == sound_location_object_marker) {
                SoundPermutation *current_permutation = sound_channels[channel_index].current_permutation;
                float lip_sync_value;

                if (current_permutation == 0 || current_permutation->mouth_data.size == 0) {
                    lip_sync_value = 0.0f;
                } else {
                    int16_t tick = (int16_t)(int32_t)sound_channels[channel_index].play_time;
                    int32_t clamped_tick;
                    int32_t mouth_data_count = current_permutation->mouth_data.size - 1;

                    if (tick < 0) {
                        clamped_tick = 0;
                    } else {
                        clamped_tick = tick;
                        if (mouth_data_count < tick) {
                            clamped_tick = mouth_data_count;
                        }
                    }
                    lip_sync_value = (float)((uint8_t *)current_permutation->mouth_data.pointer)[clamped_tick] *
                        0.003921569f;
                }

                if (game_looping_sound_data->valid &&
                    object_try_and_get(instance->owner_index, 3) != 0) {
                    unit_accumulate_clamped_offset(instance->owner_index, lip_sync_value);
                }
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x54c900): see out/phase2/sound/01.md and
scratchpad/sound_packs/0x54c900.md for the full 1058-byte listing.

Disassembly (0x54c900..0x54cd22, capstone; phase-4 review):

0x54c900: sub esp, 0x7c
0x54c903: mov eax, dword ptr [0x6966f8]
0x54c908: mov ecx, dword ptr [eax]
0x54c90a: mov edx, dword ptr [eax + 4]
0x54c90d: mov eax, dword ptr [eax + 8]
0x54c910: mov dword ptr [esp + 0x50], ecx
0x54c914: mov ecx, dword ptr [0x696718]
0x54c91a: mov dword ptr [esp + 0x54], edx
0x54c91e: mov edx, dword ptr [ecx]
0x54c920: mov dword ptr [esp + 0x5c], edx
0x54c924: mov edx, dword ptr [0x696714]
0x54c92a: mov dword ptr [esp + 0x58], eax
0x54c92e: mov eax, dword ptr [ecx + 4]
0x54c931: mov ecx, dword ptr [ecx + 8]
0x54c934: mov dword ptr [esp + 0x60], eax
0x54c938: mov eax, dword ptr [edx]
0x54c93a: mov dword ptr [esp + 0x64], ecx
0x54c93e: mov ecx, dword ptr [edx + 4]
0x54c941: mov edx, dword ptr [edx + 8]
0x54c944: push esi
0x54c945: xor esi, esi
0x54c947: cmp word ptr [0x7252b4], si
0x54c94e: mov dword ptr [esp + 0x6c], eax
0x54c952: mov dword ptr [esp + 0x70], ecx
0x54c956: mov dword ptr [esp + 0x74], edx
0x54c95a: mov dword ptr [esp + 4], esi
0x54c95e: jle 0x54cd24
0x54c964: push ebx
0x54c965: push ebp
0x54c966: push edi
0x54c967: jmp 0x54c970
0x54c969: lea esp, [esp]
0x54c970: movsx eax, si
0x54c973: lea ebp, [eax + eax*2]
0x54c976: mov eax, dword ptr [ebp*8 + 0x724a60]
0x54c97d: cmp eax, -1
0x54c980: lea ebp, [ebp*8 + 0x724a60]
0x54c987: mov dword ptr [esp + 0x18], ebp
0x54c98b: je 0x54cd0f
0x54c991: mov ecx, dword ptr [0x7252c0]
0x54c997: mov edx, dword ptr [ecx + 0x34]
0x54c99a: mov ebx, dword ptr [0x87bc14]
0x54c9a0: mov edi, eax
0x54c9a2: and edi, 0xffff
0x54c9a8: imul edi, edi, 0xb0
0x54c9ae: add edi, edx
0x54c9b0: mov edx, dword ptr [edi + 8]
0x54c9b3: and edx, 0xffff
0x54c9b9: shl edx, 5
0x54c9bc: mov ecx, dword ptr [edx + ebx + 0x14]
0x54c9c0: mov dword ptr [esp + 0x24], ecx
0x54c9c4: call 0x54e3c0
0x54c9c9: fstp dword ptr [esp + 0x14]
0x54c9cd: fld dword ptr [0x672ac0]
0x54c9d3: fld dword ptr [esp + 0x14]
0x54c9d7: fucompp 
0x54c9d9: fnstsw ax
0x54c9db: test ah, 0x44
0x54c9de: jp 0x54ca0a
0x54c9e0: fld dword ptr [0x672ac0]
0x54c9e6: fld dword ptr [edi + 0xa0]
0x54c9ec: fucompp 
0x54c9ee: fnstsw ax
0x54c9f0: test ah, 0x44
0x54c9f3: jp 0x54ca0a
0x54c9f5: mov edx, dword ptr [ebp]
0x54c9f8: push edx
0x54c9f9: call 0x54b180
0x54c9fe: mov dword ptr [ebp], 0xffffffff
0x54ca05: jmp 0x54cd0c
0x54ca0a: test byte ptr [ebp + 4], 1
0x54ca0e: je 0x54cb56
0x54ca14: movsx eax, word ptr [edi + 0x14]
0x54ca18: dec eax
0x54ca19: je 0x54ca56
0x54ca1b: dec eax
0x54ca1c: jne 0x54cc52
0x54ca22: mov eax, dword ptr [edi + 8]
0x54ca25: and eax, 0xffff
0x54ca2a: shl eax, 5
0x54ca2d: mov ecx, dword ptr [eax + ebx + 0x14]
0x54ca31: xor edx, edx
0x54ca33: mov dx, word ptr [ecx + 4]
0x54ca37: mov ecx, dword ptr [0x725208]
0x54ca3d: lea eax, [edi + 0x20]
0x54ca40: push edx
0x54ca41: push 0
0x54ca43: push 0
0x54ca45: push 0
0x54ca47: push eax
0x54ca48: push 1
0x54ca4a: push esi
0x54ca4b: call dword ptr [ecx + 0x30]
0x54ca4e: add esp, 0x1c
0x54ca51: jmp 0x54cc52
0x54ca56: movsx ebp, word ptr [edi + 6]
0x54ca5a: mov edx, dword ptr [edi + 8]
0x54ca5d: imul ebp, ebp, 0x44
0x54ca60: mov al, byte ptr [edi + 0xac]
0x54ca66: and edx, 0xffff
0x54ca6c: shl edx, 5
0x54ca6f: mov ebx, dword ptr [edx + ebx + 0x14]
0x54ca73: add ebp, 0x725218
0x54ca79: test al, al
0x54ca7b: mov dword ptr [esp + 0x20], ebx
0x54ca7f: jne 0x54cb21
0x54ca85: lea ebx, [ebp + 4]
0x54ca88: lea esi, [edi + 0x20]
0x54ca8b: lea edx, [esp + 0x34]
0x54ca8f: mov ecx, ebx
0x54ca91: call 0x4cbf80
0x54ca96: lea edx, [edi + 0x2c]
0x54ca99: push ebx
0x54ca9a: lea eax, [esp + 0x44]
0x54ca9e: call 0x4cc080
0x54caa3: lea edx, [edi + 0x38]
0x54caa6: push ebx
0x54caa7: lea eax, [esp + 0x54]
0x54caab: call 0x4cc010
0x54cab0: fld dword ptr [esp + 0x54]
0x54cab4: fmul dword ptr [0x672ac8]
0x54caba: mov eax, dword ptr [esp + 0x28]
0x54cabe: xor ecx, ecx
0x54cac0: xor edx, edx
0x54cac2: fsub dword ptr [ebp + 0x38]
0x54cac5: mov dl, byte ptr [ebp + 1]
0x54cac8: fstp dword ptr [esp + 0x54]
0x54cacc: fld dword ptr [esp + 0x58]
0x54cad0: fmul dword ptr [0x672ac8]
0x54cad6: fsub dword ptr [ebp + 0x3c]
0x54cad9: fstp dword ptr [esp + 0x58]
0x54cadd: fld dword ptr [esp + 0x5c]
0x54cae1: fmul dword ptr [0x672ac8]
0x54cae7: fsub dword ptr [ebp + 0x40]
0x54caea: fstp dword ptr [esp + 0x5c]
0x54caee: mov cx, word ptr [eax + 4]
0x54caf2: mov eax, dword ptr [edi + 0x50]
0x54caf5: push ecx
0x54caf6: mov ecx, dword ptr [edi + 0x4c]
0x54caf9: push edx
0x54cafa: push eax
0x54cafb: mov eax, dword ptr [esp + 0x24]
0x54caff: push ecx
0x54cb00: mov ecx, dword ptr [0x725208]
0x54cb06: lea edx, [esp + 0x4c]
0x54cb0a: push edx
0x54cb0b: push 1
0x54cb0d: push eax
0x54cb0e: call dword ptr [ecx + 0x30]
0x54cb11: mov ebp, dword ptr [esp + 0x3c]
0x54cb15: mov esi, dword ptr [esp + 0x34]
0x54cb19: add esp, 0x24
0x54cb1c: jmp 0x54cc52
0x54cb21: xor edx, edx
0x54cb23: mov dx, word ptr [ebx + 4]
0x54cb27: xor eax, eax
0x54cb29: mov al, byte ptr [ebp + 1]
0x54cb2c: lea ecx, [esp + 0x60]
0x54cb30: push edx
0x54cb31: mov edx, dword ptr [esp + 0x14]
0x54cb35: push eax
0x54cb36: mov eax, dword ptr [0x725208]
0x54cb3b: push 0
0x54cb3d: push 0
0x54cb3f: push ecx
0x54cb40: push 0
0x54cb42: push edx
0x54cb43: call dword ptr [eax + 0x30]
0x54cb46: mov ebp, dword ptr [esp + 0x34]
0x54cb4a: mov esi, dword ptr [esp + 0x2c]
0x54cb4e: add esp, 0x1c
0x54cb51: jmp 0x54cc52
0x54cb56: lea eax, [edi + 0x20]
0x54cb59: mov ecx, eax
0x54cb5b: mov edx, dword ptr [ecx]
0x54cb5d: mov dword ptr [esp + 0x28], edx
0x54cb61: mov edx, dword ptr [ecx + 4]
0x54cb64: mov ecx, dword ptr [ecx + 8]
0x54cb67: mov dword ptr [esp + 0x30], ecx
0x54cb6b: movsx ecx, word ptr [edi + 0x14]
0x54cb6f: dec ecx
0x54cb70: mov dword ptr [esp + 0x2c], edx
0x54cb74: je 0x54cb7e
0x54cb76: dec ecx
0x54cb77: je 0x54cb9a
0x54cb79: jmp 0x54cc52
0x54cb7e: movsx ecx, word ptr [edi + 6]
0x54cb82: imul ecx, ecx, 0x44
0x54cb85: add ecx, 0x72521c
0x54cb8b: lea edx, [esp + 0x28]
0x54cb8f: mov esi, eax
0x54cb91: call 0x4cbf80
0x54cb96: mov esi, dword ptr [esp + 0x10]
0x54cb9a: mov edx, dword ptr [edi + 8]
0x54cb9d: and edx, 0xffff
0x54cba3: shl edx, 5
0x54cba6: mov ebx, dword ptr [edx + ebx + 0x14]
0x54cbaa: fld dword ptr [ebx + 8]
0x54cbad: fld dword ptr [0x672ac0]
0x54cbb3: fld st(1)
0x54cbb5: fucompp 
0x54cbb7: fnstsw ax
0x54cbb9: test ah, 0x44
0x54cbbc: jp 0x54cbcd
0x54cbbe: movsx eax, word ptr [ebx + 4]
0x54cbc2: fstp st(0)
0x54cbc4: imul eax, eax, 0x2c
0x54cbc7: fld dword ptr [eax + 0x69eaf8]
0x54cbcd: fld dword ptr [ebx + 0xc]
0x54cbd0: fld dword ptr [0x672ac0]
0x54cbd6: fld st(1)
0x54cbd8: fucompp 
0x54cbda: fnstsw ax
0x54cbdc: test ah, 0x44
0x54cbdf: jp 0x54cbf0
0x54cbe1: movsx ecx, word ptr [ebx + 4]
0x54cbe5: fstp st(0)
0x54cbe7: imul ecx, ecx, 0x2c
0x54cbea: fld dword ptr [ecx + 0x69eafc]
0x54cbf0: fld dword ptr [esp + 0x30]
0x54cbf4: fmul dword ptr [esp + 0x30]
0x54cbf8: fld dword ptr [esp + 0x2c]
0x54cbfc: fmul dword ptr [esp + 0x2c]
0x54cc00: faddp st(1)
0x54cc02: fld dword ptr [esp + 0x28]
0x54cc06: fmul dword ptr [esp + 0x28]
0x54cc0a: faddp st(1)
0x54cc0c: fsqrt 
0x54cc0e: fsub st(2)
0x54cc10: fxch st(1)
0x54cc12: fsub st(2)
0x54cc14: fdivp st(1)
0x54cc16: fsubr dword ptr [0x672ac4]
0x54cc1c: fstp st(1)
0x54cc1e: fcom dword ptr [0x672ac0]
0x54cc24: fnstsw ax
0x54cc26: test ah, 5
0x54cc29: jp 0x54cc35
0x54cc2b: fstp st(0)
0x54cc2d: fld dword ptr [0x672ac0]
0x54cc33: jmp 0x54cc4a
0x54cc35: fcom dword ptr [0x672ac4]
0x54cc3b: fnstsw ax
0x54cc3d: test ah, 0x41
0x54cc40: jne 0x54cc4a
0x54cc42: fstp st(0)
0x54cc44: fld dword ptr [0x672ac4]
0x54cc4a: fmul dword ptr [esp + 0x14]
0x54cc4e: fstp dword ptr [esp + 0x14]
0x54cc52: cmp word ptr [edi + 2], 0
0x54cc57: jne 0x54cc66
0x54cc59: mov edx, dword ptr [esp + 0x14]
0x54cc5d: push edx
0x54cc5e: push esi
0x54cc5f: call 0x54c750
0x54cc64: jmp 0x54cc71
0x54cc66: mov eax, dword ptr [esp + 0x14]
0x54cc6a: push eax
0x54cc6b: push esi
0x54cc6c: call 0x54deb0
0x54cc71: mov ecx, dword ptr [esp + 0x2c]
0x54cc75: movsx edx, word ptr [ecx + 4]
0x54cc79: imul edx, edx, 0x2c
0x54cc7c: mov al, byte ptr [edx + 0x69eae8]
0x54cc82: add esp, 8
0x54cc85: test al, al
0x54cc87: je 0x54cd0f
0x54cc8d: cmp dword ptr [edi + 0x10], 0x5448c0
0x54cc94: jne 0x54cd0f
0x54cc96: fld dword ptr [ebp + 8]
0x54cc99: call 0x6391b4
0x54cc9e: mov ebp, dword ptr [ebp + 0x10]
0x54cca1: mov ecx, dword ptr [ebp + 0x54]
0x54cca4: test ecx, ecx
0x54cca6: je 0x54ccd9
0x54cca8: test ax, ax
0x54ccab: jge 0x54ccb1
0x54ccad: xor eax, eax
0x54ccaf: jmp 0x54ccbb
0x54ccb1: movsx eax, ax
0x54ccb4: dec ecx
0x54ccb5: cmp eax, ecx
0x54ccb7: jle 0x54ccbb
0x54ccb9: mov eax, ecx
0x54ccbb: mov ecx, dword ptr [ebp + 0x60]
0x54ccbe: movsx eax, ax
0x54ccc1: movzx edx, byte ptr [ecx + eax]
0x54ccc5: mov dword ptr [esp + 0x24], edx
0x54ccc9: fild dword ptr [esp + 0x24]
0x54cccd: fmul dword ptr [0x672ad4]
0x54ccd3: fstp dword ptr [esp + 0x1c]
0x54ccd7: jmp 0x54cce1
0x54ccd9: mov dword ptr [esp + 0x1c], 0
0x54cce1: mov eax, dword ptr [0x7461a0]
0x54cce6: mov cl, byte ptr [eax + 0x24]
0x54cce9: test cl, cl
0x54cceb: mov edi, dword ptr [edi + 0xc]
0x54ccee: je 0x54cd0f
0x54ccf0: push 3
0x54ccf2: mov ecx, edi
0x54ccf4: call 0x4f6ec0
0x54ccf9: add esp, 4
0x54ccfc: test eax, eax
0x54ccfe: je 0x54cd0f
0x54cd00: mov ecx, dword ptr [esp + 0x1c]
0x54cd04: push ecx
0x54cd05: mov eax, edi
0x54cd07: call 0x570400
0x54cd0c: add esp, 4
0x54cd0f: inc esi
0x54cd10: cmp si, word ptr [0x7252b4]
0x54cd17: mov dword ptr [esp + 0x10], esi
0x54cd1b: jl 0x54c970
0x54cd21: pop edi
#endif
