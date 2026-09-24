// sound_update_range_and_ducking  (Ghidra: FUN_0054bd60, still unnamed there)
// address 0x54bd60, size 692 bytes
// name confidence: 0.5   rewrite confidence: 0.9
// evidence: out/phase4/sound_functions.md "Processes pending pitch-range loads and gradually
// ducks/restores the shared game-sound gain depending on whether music-class sounds are active."
// (the summary says "music"; the class checks are actually the three non-effect scripted-dialog
// classes -- ducking follows dialog, not music). Field mapping cross-checked against
// disassembly (scratchpad/disasm/disasm.py, 0x54bd60..0x54beeb) for every sound_schedule_gain_fade
// call's register arguments.
// register convention: void, no parameters.
// Phase-4 review (full disassembly appended below): the third fade call's EBX is set to -1 by
// `or ebx, 0xffffffff` at 0x54becd on the only path reaching it, so k_datum_index_none is exact.
// Ducking steps: 0.03 / 0.007 per elapsed ms (0x00672cd0 / 0x00673184) toward 0.7 / 1.0.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "sound.h"

extern player_globals *local_player_globals; // 0x0087a478
extern data_array *sound_data;               // 0x007252c0, "sounds" 0x200 x 0xb0
extern tag_instance *tag_instances;          // 0x0087bc14
extern uint8_t sound_paused;                 // 0x00725202
extern sound_class_definition sound_class_definitions[k_maximum_sound_classes]; // 0x0069eae0
extern float sound_dialog_ducking_gain;      // 0x006893d4
extern float sound_ducking_gain;             // 0x007252a4
extern float sound_time_delta;               // 0x00725210

extern datum_index datum_next(int16_t after_index, data_array *array); // 0x4d0630, memory module
extern int16_t sound_channel_release_detail_buffers(int16_t channel_index); // this module, 0x54d020
extern uint32_t sound_instance_invoke_location_proc(datum_index sound_handle); // this module, 0x54bcd0
extern void sound_instance_stop(datum_index sound_handle); // this module, 0x54b180
extern int16_t sound_location_check_audibility(sound_location *location, float max_distance); // this module, 0x54bb20
extern void render_debug_sound(datum_index sound_handle); // this module, 0x54e6d0
extern void sound_schedule_gain_fade(datum_index fade_in_handle, int16_t fade_curve, float duration_seconds,
    datum_index fade_out_handle); // this module, 0x54af60

// Per-update pass over every live sound: stops any whose channel/location proc has failed,
// range-checks it against a listener and fades it in or out of audible range, and (for scripted
// dialog sounds, when no player currently has a unit) fades it out once its track has finished.
// Also ramps the shared dialog-ducking gain toward its target (0.7 while a scripted dialog
// sound -- other than scripted_effect -- was seen this pass, otherwise back toward 1.0).
void sound_update_range_and_ducking(void)
{
    uint8_t no_player_has_a_unit;
    datum_index sound_handle;
    sound *instance;
    Sound *definition;
    float max_distance;
    int16_t listener_index;
    uint8_t saw_dialog_class;
    float step, delta, clamped_step;

    no_player_has_a_unit = local_player_globals->no_player_has_a_unit;
    saw_dialog_class = 0;

    sound_handle = datum_next(-1, sound_data);
    while (sound_handle != 0xffffffff) {
        instance = (sound *)((uint8_t *)sound_data->data + (sound_handle & 0xffff) * sizeof(sound));
        definition = (Sound *)tag_instances[instance->definition_index & 0xffff].data;

        if ((instance->channel_index != -1 && sound_channel_release_detail_buffers(instance->channel_index) == 0 &&
             instance->play_state != _sound_play_loop && instance->play_state != _sound_play_loop_stopping) ||
            (sound_instance_invoke_location_proc(sound_handle) == 0 && !sound_paused)) {
            sound_instance_stop(sound_handle);
            goto next_sound;
        }

        max_distance = definition->maximum_distance;
        if (max_distance == 0.0f) {
            max_distance = sound_class_definitions[definition->sound_class].default_maximum_distance;
        }
        listener_index = sound_location_check_audibility(&instance->location, max_distance);
        render_debug_sound(sound_handle);

        if (definition->sound_class == soundclass_scripted_dialog_player ||
            definition->sound_class == soundclass_scripted_dialog_other ||
            definition->sound_class == soundclass_scripted_dialog_force_unspatialized) {
            saw_dialog_class = 1;
        }

        if (listener_index == -1) {
            if (!(instance->flags & _sound_out_of_range_bit)) {
                sound_schedule_gain_fade(0xffffffff, 0, 2.0f, sound_handle);
                instance->flags |= _sound_out_of_range_bit;
            }
        } else {
            instance->listener_index = listener_index;
            if (instance->flags & _sound_out_of_range_bit) {
                sound_schedule_gain_fade(sound_handle, 0, 0.5f, 0xffffffff);
                instance->flags &= ~_sound_out_of_range_bit;
            }
        }

        if (no_player_has_a_unit) {
            if (definition->sound_class == soundclass_scripted_dialog_player) {
                if (instance->channel_index == -1) {
                    sound_instance_stop(sound_handle);
                    goto next_sound;
                }
                sound_schedule_gain_fade(0xffffffff, 0, 0.3f, sound_handle);
            } else if (definition->sound_class == soundclass_scripted_dialog_other && instance->channel_index == -1) {
                sound_instance_stop(sound_handle);
                goto next_sound;
            }
        }

    next_sound:
        sound_handle = datum_next((int16_t)sound_handle, sound_data);
    }

    if (saw_dialog_class) {
        step = sound_time_delta * 0.03f;
        delta = sound_dialog_ducking_gain - sound_ducking_gain;
    } else {
        step = sound_time_delta * 0.007f;
        delta = 1.0f - sound_ducking_gain;
    }
    clamped_step = -step;
    if (-step <= delta) {
        clamped_step = step;
        if (delta <= step) {
            sound_ducking_gain = delta + sound_ducking_gain;
            return;
        }
    }
    sound_ducking_gain = clamped_step + sound_ducking_gain;
}

#if 0
Original Ghidra decompilation (0x54bd60): see out/phase2/sound/01.md and
scratchpad/sound_packs/0x54bd60.md for the full 692-byte listing.

Disassembly (0x54bd60..0x54c014, capstone; phase-4 review):

0x54bd60: sub esp, 0xc
0x54bd63: mov eax, dword ptr [0x87a478]
0x54bd68: mov cl, byte ptr [eax + 0x10]
0x54bd6b: push ebx
0x54bd6c: mov ebx, dword ptr [0x7252c0]
0x54bd72: push edi
0x54bd73: or edx, 0xffffffff
0x54bd76: mov edi, ebx
0x54bd78: mov byte ptr [esp + 0xb], cl
0x54bd7c: mov byte ptr [esp + 0xa], 0
0x54bd81: call 0x4d0630
0x54bd86: mov edi, eax
0x54bd88: cmp edi, -1
0x54bd8b: je 0x54bfba
0x54bd91: push ebp
0x54bd92: push esi
0x54bd93: mov ebp, dword ptr [ebx + 0x34]
0x54bd96: mov eax, dword ptr [0x87bc14]
0x54bd9b: mov esi, edi
0x54bd9d: and esi, 0xffff
0x54bda3: imul esi, esi, 0xb0
0x54bda9: mov edx, dword ptr [esi + ebp + 8]
0x54bdad: add esi, ebp
0x54bdaf: and edx, 0xffff
0x54bdb5: xor ecx, ecx
0x54bdb7: mov cx, word ptr [esi + 0x8c]
0x54bdbe: shl edx, 5
0x54bdc1: cmp cx, -1
0x54bdc5: mov ebp, dword ptr [edx + eax + 0x14]
0x54bdc9: je 0x54bde9
0x54bdcb: call 0x54d020
0x54bdd0: test ax, ax
0x54bdd3: jne 0x54bde9
0x54bdd5: mov ax, word ptr [esi + 2]
0x54bdd9: cmp ax, 2
0x54bddd: je 0x54bde9
0x54bddf: cmp ax, 3
0x54bde3: jne 0x54befb
0x54bde9: mov eax, edi
0x54bdeb: call 0x54bcd0
0x54bdf0: test al, al
0x54bdf2: jne 0x54be01
0x54bdf4: mov al, byte ptr [0x725202]
0x54bdf9: test al, al
0x54bdfb: je 0x54befb
0x54be01: mov ecx, dword ptr [esi + 8]
0x54be04: fld dword ptr [0x672ac0]
0x54be0a: mov edx, dword ptr [0x87bc14]
0x54be10: and ecx, 0xffff
0x54be16: shl ecx, 5
0x54be19: mov ecx, dword ptr [ecx + edx + 0x14]
0x54be1d: mov eax, dword ptr [ecx + 0xc]
0x54be20: mov dword ptr [esp + 0x14], eax
0x54be24: fld dword ptr [esp + 0x14]
0x54be28: fucompp 
0x54be2a: fnstsw ax
0x54be2c: test ah, 0x44
0x54be2f: jp 0x54be42
0x54be31: movsx ecx, word ptr [ecx + 4]
0x54be35: imul ecx, ecx, 0x2c
0x54be38: mov edx, dword ptr [ecx + 0x69eafc]
0x54be3e: mov dword ptr [esp + 0x14], edx
0x54be42: mov eax, dword ptr [esp + 0x14]
0x54be46: push eax
0x54be47: lea eax, [esi + 0x14]
0x54be4a: call 0x54bb20
0x54be4f: add esp, 4
0x54be52: mov ebx, eax
0x54be54: mov eax, edi
0x54be56: call 0x54e6d0
0x54be5b: mov ax, word ptr [ebp + 4]
0x54be5f: cmp ax, 0x2c
0x54be63: je 0x54be71
0x54be65: cmp ax, 0x2e
0x54be69: je 0x54be71
0x54be6b: cmp ax, 0x2f
0x54be6f: jne 0x54be76
0x54be71: mov byte ptr [esp + 0x12], 1
0x54be76: cmp bx, -1
0x54be7a: mov al, byte ptr [esi + 4]
0x54be7d: jne 0x54be9c
0x54be7f: test al, 4
0x54be81: jne 0x54bebb
0x54be83: push edi
0x54be84: push 0x40000000
0x54be89: push 0
0x54be8b: or ebx, 0xffffffff
0x54be8e: call 0x54af60
0x54be93: add esp, 0xc
0x54be96: or byte ptr [esi + 4], 4
0x54be9a: jmp 0x54bebb
0x54be9c: test al, 4
0x54be9e: mov word ptr [esi + 6], bx
0x54bea2: je 0x54bebb
0x54bea4: push -1
0x54bea6: push 0x3f000000
0x54beab: push 0
0x54bead: mov ebx, edi
0x54beaf: call 0x54af60
0x54beb4: add esp, 0xc
0x54beb7: and byte ptr [esi + 4], 0xfb
0x54bebb: mov al, byte ptr [esp + 0x13]
0x54bebf: test al, al
0x54bec1: je 0x54bf04
0x54bec3: mov ax, word ptr [ebp + 4]
0x54bec7: cmp ax, 0x2c
0x54becb: jne 0x54beeb
0x54becd: or ebx, 0xffffffff
0x54bed0: cmp word ptr [esi + 0x8c], bx
0x54bed7: push edi
0x54bed8: je 0x54befc
0x54beda: push 0x3e99999a
0x54bedf: push 0
0x54bee1: call 0x54af60
0x54bee6: add esp, 0xc
0x54bee9: jmp 0x54bf04
0x54beeb: cmp ax, 0x2e
0x54beef: jne 0x54bf04
0x54bef1: cmp word ptr [esi + 0x8c], -1
0x54bef9: jne 0x54bf04
0x54befb: push edi
0x54befc: call 0x54b180
0x54bf01: add esp, 4
0x54bf04: mov ebx, dword ptr [0x7252c0]
0x54bf0a: lea eax, [edi + 1]
0x54bf0d: or edx, 0xffffffff
0x54bf10: test ax, ax
0x54bf13: jl 0x54bf4b
0x54bf15: mov di, word ptr [ebx + 0x2e]
0x54bf19: cmp ax, di
0x54bf1c: jge 0x54bf4b
0x54bf1e: movsx esi, word ptr [ebx + 0x22]
0x54bf22: mov ebp, dword ptr [ebx + 0x34]
0x54bf25: movsx ecx, ax
0x54bf28: imul ecx, esi
0x54bf2b: add ecx, ebp
0x54bf2d: lea ecx, [ecx]
0x54bf30: cmp word ptr [ecx], 0
0x54bf34: jne 0x54bf40
0x54bf36: inc eax
0x54bf37: add ecx, esi
0x54bf39: cmp ax, di
0x54bf3c: jl 0x54bf30
0x54bf3e: jmp 0x54bf4b
0x54bf40: movsx edx, word ptr [ecx]
0x54bf43: movsx ecx, ax
0x54bf46: shl edx, 0x10
0x54bf49: or edx, ecx
0x54bf4b: cmp edx, -1
0x54bf4e: mov edi, edx
0x54bf50: jne 0x54bd93
0x54bf56: mov al, byte ptr [esp + 0x12]
0x54bf5a: test al, al
0x54bf5c: pop esi
0x54bf5d: pop ebp
0x54bf5e: je 0x54bfba
0x54bf60: fld dword ptr [0x725210]
0x54bf66: fmul dword ptr [0x672cd0]
0x54bf6c: fstp dword ptr [esp + 0xc]
0x54bf70: fld dword ptr [0x6893d4]
0x54bf76: fsub dword ptr [0x7252a4]
0x54bf7c: fstp dword ptr [esp + 0x10]
0x54bf80: fld dword ptr [esp + 0xc]
0x54bf84: fchs 
0x54bf86: fld dword ptr [esp + 0x10]
0x54bf8a: fcomp st(1)
0x54bf8c: fnstsw ax
0x54bf8e: test ah, 5
0x54bf91: jnp 0x54c002
0x54bf93: fstp st(0)
0x54bf95: fld dword ptr [esp + 0x10]
0x54bf99: fcomp dword ptr [esp + 0xc]
0x54bf9d: fnstsw ax
0x54bf9f: test ah, 0x41
0x54bfa2: je 0x54bffe
0x54bfa4: fld dword ptr [esp + 0x10]
0x54bfa8: pop edi
0x54bfa9: fadd dword ptr [0x7252a4]
0x54bfaf: pop ebx
0x54bfb0: fstp dword ptr [0x7252a4]
0x54bfb6: add esp, 0xc
0x54bfb9: ret 
0x54bfba: fld dword ptr [0x725210]
0x54bfc0: fmul dword ptr [0x673184]
0x54bfc6: fstp dword ptr [esp + 0x10]
0x54bfca: fld dword ptr [0x672ac4]
0x54bfd0: fsub dword ptr [0x7252a4]
0x54bfd6: fstp dword ptr [esp + 0xc]
0x54bfda: fld dword ptr [esp + 0x10]
0x54bfde: fchs 
0x54bfe0: fld dword ptr [esp + 0xc]
0x54bfe4: fcomp st(1)
0x54bfe6: fnstsw ax
0x54bfe8: test ah, 5
0x54bfeb: jnp 0x54c002
0x54bfed: fstp st(0)
0x54bfef: fld dword ptr [esp + 0xc]
0x54bff3: fcomp dword ptr [esp + 0x10]
0x54bff7: fnstsw ax
0x54bff9: test ah, 0x41
0x54bffc: je 0x54bfa4
0x54bffe: fld dword ptr [esp + 0xc]
0x54c002: fadd dword ptr [0x7252a4]
0x54c008: pop edi
0x54c009: pop ebx
0x54c00a: fstp dword ptr [0x7252a4]
0x54c010: add esp, 0xc
0x54c013: ret 
#endif
