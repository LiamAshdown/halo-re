// sound_looping_set_state  (Ghidra: sound_looping_set_state, already named)
// address 0x549fa0, size 3548 bytes
// name confidence: 0.5   rewrite confidence: 0.7
// evidence: out/phase4/sound_functions.md summary "Sets the playback state/immediate flag on a
//   looping-sound datum for an object, evicting cached resources for permutations that are no
//   longer needed."; types/sound.h looping_sound (location 0x0c, update_toggle 0x4c, alternate
//   0x4d, finished 0x4e, active_sound_count 0x50, state 0x52, track_sounds 0xd4) and types/tags.h
//   SoundLooping (flags, maximum_distance 0x20, continuous_damage_effect.tag_id 0x38, tracks 0x3c)
//   / SoundLoopingTrack (fade_in_duration +8, fade_out_duration +0xc, start/loop/end/
//   alternate_loop/alternate_end tag_id at 0x3c/0x4c/0x5c/0x8c/0x9c).
// Phase-4 review: rewritten line by line against the capstone disassembly (the earlier draft
//   was built from the Ghidra C, which hides the EAX/EBX/ECX arguments of every callee). The
//   ten "evict unlocked cached samples of one Sound tag" sweeps are one inlined helper in the
//   original, factored into sound_looping_evict_sound_samples here; the order of the five
//   dependencies (start, end, alternate_end, loop, alternate_loop) is the binary's.
// register convention: EAX -> owner (game_looping_sound handle, the looping_sound.owner key),
//   ECX -> definition_index (SoundLooping tag), stack -> (location, state, alternate,
//   fade_duration). Returns AL.
// state: 0 start, 1 continue, 2 stop (game_looping_sound_update passes 2 to stop).
// fade_duration: seconds; nonzero makes a stop fade every track sound out over that time
//   instead of playing the end parts.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "sound.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint8_t sound_initialized;   // 0x00725200
extern uint8_t sound_enabled;       // 0x00725201
extern uint8_t sound_disabled;      // 0x007252b6
extern data_array *looping_sound_data; // 0x00724a50
extern tag_instance *tag_instances; // 0x0087bc14
extern uint8_t sound_update_toggle; // 0x00725214
extern data_array *sound_cache_entries; // 0x006ac528
extern struct cache *sound_cache;   // 0x006ac530
extern data_array *sound_data;      // 0x007252c0

extern void datum_delete(data_array *array, datum_index handle); // 0x4d0510
extern void cache_evict_entry(datum_index handle, struct cache *self); // 0x4d1c20
extern void player_effect_apply_at_object(uint32_t tag_reference, int16_t local_player_index,
    real_point3d *origin); // 0x456900, effects module; see note at the call
extern void sound_looping_check_audibility_gate(datum_index definition_index, sound_location *location); // 0x54e740
extern datum_index sound_looping_find_by_owner(int32_t owner); // 0x54e5d0
extern datum_index sound_looping_state_new(datum_index definition_index, int32_t owner,
    sound_location *location); // 0x54d140
extern datum_index sound_looping_create_detail_sound(datum_index owner, datum_index definition_index,
    int16_t track_index, int16_t play_state); // 0x54d9f0
extern void sound_schedule_gain_fade(datum_index fade_in_handle, int16_t fade_curve, float duration_seconds,
    datum_index fade_out_handle); // 0x54af60
extern void sound_instance_queue_definition_switch(datum_index sound_handle,
    datum_index new_definition_index); // 0x54dd90
extern int16_t sound_location_check_audibility(sound_location *location, float max_distance); // 0x54bb20

// Evicts every cached, unlocked permutation sample block of one Sound tag and clears the
// permutation's runtime cache words (samples_pointer = cache datum, _pad_30 = sample pointer).
static void sound_looping_evict_sound_samples(Sound *tag)
{
    int32_t i;

    for (i = 0; i < (int32_t)tag->pitch_ranges.count; i++) {
        SoundPitchRange *range = (SoundPitchRange *)tag->pitch_ranges.pointer + i;
        int32_t j;

        for (j = 0; j < (int32_t)range->permutations.count; j++) {
            SoundPermutation *permutation = (SoundPermutation *)range->permutations.pointer + j;
            datum_index cache_index = (datum_index)permutation->samples_pointer;

            if (cache_index != (datum_index)0xffffffff &&
                ((sound_cache_entry *)sound_cache_entries->data)[cache_index & 0xffff].lock_count == 0) {
                cache_evict_entry(cache_index, sound_cache);
                permutation->samples_pointer = 0xffffffff;
                *(uint32_t *)&((struct SoundPermutation *)permutation)->cache_page = 0;
            }
        }
    }
}

static void sound_looping_evict_samples(uint32_t tag_id)
{
    if (tag_id != 0xffffffff) {
        sound_looping_evict_sound_samples((Sound *)tag_instances[tag_id & 0xffff].data);
    }
}

// Second sweep helper: returns 0 when `tag_id` names a non-music Sound (the whole sweep stops
// there), otherwise evicts it (music) or skips it (none) and returns 1.
static uint8_t sound_looping_evict_music_samples(uint32_t tag_id)
{
    Sound *tag;

    if (tag_id == 0xffffffff) {
        return 1;
    }
    tag = (Sound *)tag_instances[tag_id & 0xffff].data;
    if (tag->sound_class != soundclass_music) {
        return 0;
    }
    sound_looping_evict_sound_samples(tag);
    return 1;
}

#define TRACK_TAG(track, field) (*(uint32_t *)&(track)->field.tag_id)

// blam-cc: EAX -> owner, ECX -> definition_index, stack -> (location, state, alternate, fade_duration)
uint8_t sound_looping_set_state(int32_t owner, datum_index definition_index,
    sound_location *location, int16_t state, uint8_t alternate, float fade_duration)
{
    SoundLooping *definition;
    looping_sound *self;
    datum_index handle;
    uint8_t is_new;
    int16_t i;

    sound_looping_check_audibility_gate(definition_index, location);

    if (sound_initialized == 0 || sound_enabled == 0 || sound_disabled != 0) {
        return state == 2;
    }

    handle = sound_looping_find_by_owner(owner);
    is_new = 0;
    if (handle == (datum_index)0xffffffff) {
        if (state == 2) {
            return 1;
        }
        handle = sound_looping_state_new(definition_index, owner, location);
        is_new = 1;
        if (handle == (datum_index)0xffffffff) {
            return 0;
        }
    }

    self = &((looping_sound *)looping_sound_data->data)[handle & 0xffff];
    definition = (SoundLooping *)tag_instances[definition_index & 0xffff].data;

    self->location = *location;
    self->update_toggle = sound_update_toggle;

    // stopped (or ran out of permutations) and nothing is still playing: free it
    if ((state == 2 || self->finished != 0) && self->active_sound_count == 0) {
        if ((definition->flags & 0x02) != 0) { // not_a_loop: nothing will replay these samples
            for (i = 0; i < (int32_t)definition->tracks.count; i++) {
                SoundLoopingTrack *track = (SoundLoopingTrack *)definition->tracks.pointer + i;

                sound_looping_evict_samples(TRACK_TAG(track, start));
                sound_looping_evict_samples(TRACK_TAG(track, end));
                sound_looping_evict_samples(TRACK_TAG(track, alternate_end));
                sound_looping_evict_samples(TRACK_TAG(track, loop));
                sound_looping_evict_samples(TRACK_TAG(track, alternate_loop));
            }
        }
        datum_delete(looping_sound_data, handle);
        return 1;
    }

    if (*(uint32_t *)&definition->continuous_damage_effect.tag_id != 0xffffffff) {
        // binary: tag id on the stack, ESI = &location->position; 0x456900 always applies to
        // local player 0 (it zeroes DX itself), so the index passed here is only documentary
        player_effect_apply_at_object(*(uint32_t *)&definition->continuous_damage_effect.tag_id, 0,
            (real_point3d *)&location->position);
    }

    for (i = 0; i < (int32_t)definition->tracks.count; i++) {
        SoundLoopingTrack *track = (SoundLoopingTrack *)definition->tracks.pointer + i;

        if (is_new) {
            self->track_sounds[i] = (datum_index)0xffffffff;
        }

        if (state == 0) {
            if (TRACK_TAG(track, start) != 0xffffffff) {
                self->track_sounds[i] = sound_looping_create_detail_sound(handle, TRACK_TAG(track, start),
                    i, _sound_play_loop_start);
            }
        } else if (state == 2) {
            goto stop_track;
        }

        if (self->finished != 0) {
            goto stop_track;
        }

        {
            uint32_t loop_tag = TRACK_TAG(track, loop);

            if (alternate != 0 && TRACK_TAG(track, alternate_loop) != 0xffffffff) {
                loop_tag = TRACK_TAG(track, alternate_loop);
            }
            if (loop_tag == 0xffffffff) {
                continue;
            }

            if (self->track_sounds[i] == (datum_index)0xffffffff ||
                (state == 0 && (track->flags & 0x01) != 0)) { // fade_in_at_start
                datum_index new_sound = sound_looping_create_detail_sound(handle, loop_tag, i, _sound_play_loop);

                if (new_sound != (datum_index)0xffffffff) {
                    if (state != 0) {
                        sound_schedule_gain_fade(new_sound, _sound_fade_linear, 2.0f, (datum_index)0xffffffff);
                    } else if ((track->flags & 0x01) != 0) {
                        sound_schedule_gain_fade(new_sound, _sound_fade_linear, track->fade_in_duration,
                            (datum_index)0xffffffff);
                    }
                    self->track_sounds[i] = new_sound;
                }
            } else if (alternate != self->alternate && (track->flags & 0x04) != 0) { // fade_in_alternate
                datum_index new_sound = sound_looping_create_detail_sound(handle, loop_tag, i, _sound_play_loop);

                if (new_sound != (datum_index)0xffffffff) {
                    // crossfade: new loop in, old loop out, both over fade_out_duration
                    sound_schedule_gain_fade(new_sound, _sound_fade_linear, track->fade_out_duration,
                        self->track_sounds[i]);
                    self->track_sounds[i] = new_sound;
                }
            } else if (!is_new) {
                sound_instance_queue_definition_switch(self->track_sounds[i], loop_tag);
            }
        }
        continue;

    stop_track:
        if (self->state == 2) {
            continue;
        }
        if (fade_duration != 0.0f) {
            sound_schedule_gain_fade((datum_index)0xffffffff, _sound_fade_linear, fade_duration,
                self->track_sounds[i]);
            continue;
        }

        if (self->track_sounds[i] != (datum_index)0xffffffff &&
            ((track->flags & 0x02) != 0 || // fade_out_at_stop
             (TRACK_TAG(track, end) == 0xffffffff && (definition->flags & 0x02) == 0))) {
            sound_schedule_gain_fade((datum_index)0xffffffff, _sound_fade_linear, track->fade_out_duration,
                self->track_sounds[i]);
        }

        if (TRACK_TAG(track, end) != 0xffffffff) {
            uint32_t end_tag = TRACK_TAG(track, end);

            if (alternate != 0 && TRACK_TAG(track, alternate_end) != 0xffffffff) {
                end_tag = TRACK_TAG(track, alternate_end);
            }

            if ((track->flags & 0x02) != 0) { // fade_out_at_stop: the end part is a separate sound
                sound_looping_create_detail_sound(handle, end_tag, i, _sound_play_loop_end);
            } else {
                datum_index track_sound = self->track_sounds[i];

                if (track_sound != (datum_index)0xffffffff) {
                    sound *playing = &((sound *)sound_data->data)[track_sound & 0xffff];

                    if (playing->channel_index != -1) {
                        // the loop finishes its current permutation, then switches to the end part
                        sound_instance_queue_definition_switch(track_sound, end_tag);
                        playing->play_state = _sound_play_loop_stopping;
                    }
                }
            }
        }
    }

    // nothing audible from here and nothing playing: drop cached music samples and the datum
    if (self->active_sound_count == 0 &&
        sound_location_check_audibility(location, definition->maximum_distance) == -1) {
        for (i = 0; i < (int32_t)definition->tracks.count; i++) {
            SoundLoopingTrack *track = (SoundLoopingTrack *)definition->tracks.pointer + i;

            if (!sound_looping_evict_music_samples(TRACK_TAG(track, start)) ||
                !sound_looping_evict_music_samples(TRACK_TAG(track, end)) ||
                !sound_looping_evict_music_samples(TRACK_TAG(track, alternate_end)) ||
                !sound_looping_evict_music_samples(TRACK_TAG(track, loop)) ||
                !sound_looping_evict_music_samples(TRACK_TAG(track, alternate_loop))) {
                break;
            }
        }
        datum_delete(looping_sound_data, handle);
        // falls through: the binary still stores state/alternate into the deleted slot
    }

    self->state = state;
    self->alternate = alternate;
    return 0;
}

#undef TRACK_TAG

#if 0
Original Ghidra decompilation (0x549fa0) -- see out/phase2/sound/02.md or
`python tools/pack.py 0x549fa0` for the full 498-line verbatim decompile (five near-identical
25-line cache-eviction blocks, repeated twice, are folded into
sound_looping_evict_tag_cache above). Structural skeleton:

bool sound_looping_set_state(undefined4 *param_1,short param_2,char param_3,float param_4)
{
  FUN_0054e740(param_1);
  if (sound not initialized/enabled or disabled) return param_2 == 2;
  local_18 = FUN_0054e5d0();                 // find existing looping_sound by owner reference
  if (local_18 == -1) {
    if (param_2 == 2) return true;
    local_18 = FUN_0054d140();                // allocate new looping_sound
    bVar3 = true;
    if (local_18 == -1) return false;
  }
  // copy *param_1 (16 dwords) into looping_sound.location; update_toggle = sound_update_toggle
  if ((param_2 == 2 || looping_sound.finished) && looping_sound.active_sound_count == 0) {
    if (definition.flags & 2 /* not_a_loop */) {
      for each track: evict cache for start/end/alternate_end/loop/alternate_loop tag_id
    }
    datum_delete(); return true;
  }
  if (definition.continuous_damage_effect.tag_id != -1) FUN_00456900(...);
  for each track {
    if (bVar3) track_sounds[i] = -1;
    if (param_2 == 0) {
      if (track.start.tag_id != -1) track_sounds[i] = FUN_0054d9f0(local_18, start, i, 1);
LAB_0054a639:
      if (looping_sound.finished) goto LAB_0054ac96;
      // pick loop or alternate_loop tag, create/fade/replace the loop-part sound
    } else {
      if (param_2 != 2) goto LAB_0054a639;
LAB_0054ac96:
      if (looping_sound.state != 2) {
        // pick end or alternate_end tag, fade out and hard-stop or create the end-part sound
      }
    }
  }
  if (looping_sound.state == 0 && FUN_0054bb20(definition.maximum_distance) == -1) {
    for each track (breaking at the first non-music dependency):
      evict cache for start/end/alternate_end/loop/alternate_loop tag_id
    datum_delete();
  }
  looping_sound.state = param_2;
  looping_sound.alternate = param_3;
  return false;
}

Disassembly (0x549fa0..0x54adac, capstone; phase-4 review):

0x549fa0: sub esp, 0x2c
0x549fa3: cmp word ptr [esp + 0x34], 2
0x549fa9: push ebx
0x549faa: push ebp
0x549fab: mov ebp, dword ptr [esp + 0x38]
0x549faf: push esi
0x549fb0: push edi
0x549fb1: mov esi, ecx
0x549fb3: mov edi, eax
0x549fb5: push ebp
0x549fb6: mov eax, esi
0x549fb8: sete bl
0x549fbb: call 0x54e740
0x549fc0: mov al, byte ptr [0x725200]
0x549fc5: add esp, 4
0x549fc8: test al, al
0x549fca: je 0x54ada2
0x549fd0: mov al, byte ptr [0x725201]
0x549fd5: test al, al
0x549fd7: je 0x54ada2
0x549fdd: mov al, byte ptr [0x7252b6]
0x549fe2: test al, al
0x549fe4: jne 0x54ada2
0x549fea: push edi
0x549feb: call 0x54e5d0
0x549ff0: add esp, 4
0x549ff3: cmp eax, -1
0x549ff6: mov cl, 1
0x549ff8: mov dword ptr [esp + 0x24], eax
0x549ffc: mov byte ptr [esp + 0x12], 0
0x54a001: mov byte ptr [esp + 0x13], cl
0x54a005: jne 0x54a030
0x54a007: cmp word ptr [esp + 0x44], 2
0x54a00d: je 0x54ad98
0x54a013: push ebp
0x54a014: push edi
0x54a015: push esi
0x54a016: call 0x54d140
0x54a01b: add esp, 0xc
0x54a01e: cmp eax, -1
0x54a021: mov dword ptr [esp + 0x24], eax
0x54a025: mov byte ptr [esp + 0x12], 1
0x54a02a: je 0x54ad8e
0x54a030: mov ebp, dword ptr [esp + 0x24]
0x54a034: mov eax, dword ptr [0x724a50]
0x54a039: mov edi, dword ptr [eax + 0x34]
0x54a03c: mov ecx, dword ptr [0x87bc14]
0x54a042: mov edx, dword ptr [esp + 0x40]
0x54a046: and ebp, 0xffff
0x54a04c: imul ebp, ebp, 0xe4
0x54a052: and esi, 0xffff
0x54a058: add ebp, edi
0x54a05a: shl esi, 5
0x54a05d: cmp word ptr [esp + 0x44], 2
0x54a063: mov ebx, dword ptr [esi + ecx + 0x14]
0x54a067: lea edi, [ebp + 0xc]
0x54a06a: mov ecx, 0x10
0x54a06f: mov esi, edx
0x54a071: rep movsd dword ptr es:[edi], dword ptr [esi]
0x54a073: mov al, byte ptr [0x725214]
0x54a078: mov dword ptr [esp + 0x38], ebp
0x54a07c: mov dword ptr [esp + 0x20], ebx
0x54a080: mov byte ptr [ebp + 0x4c], al
0x54a083: je 0x54a090
0x54a085: mov al, byte ptr [ebp + 0x4e]
0x54a088: test al, al
0x54a08a: je 0x54a5b3
0x54a090: cmp word ptr [ebp + 0x50], 0
0x54a095: jne 0x54a5b3
0x54a09b: test byte ptr [ebx], 2
0x54a09e: je 0x54a599
0x54a0a4: mov ecx, dword ptr [esp + 0x20]
0x54a0a8: mov eax, dword ptr [ecx + 0x3c]
0x54a0ab: xor edi, edi
0x54a0ad: cmp eax, edi
0x54a0af: mov dword ptr [esp + 0x30], edi
0x54a0b3: jle 0x54a599
0x54a0b9: xor eax, eax
0x54a0bb: jmp 0x54a0c0
0x54a0bd: lea ecx, [ecx]
0x54a0c0: lea edx, [eax + eax*4]
0x54a0c3: mov eax, dword ptr [esp + 0x20]
0x54a0c7: mov esi, dword ptr [eax + 0x40]
0x54a0ca: shl edx, 5
0x54a0cd: mov eax, dword ptr [edx + esi + 0x3c]
0x54a0d1: add edx, esi
0x54a0d3: cmp eax, -1
0x54a0d6: mov dword ptr [esp + 0x28], edx
0x54a0da: je 0x54a1c0
0x54a0e0: mov ecx, dword ptr [0x87bc14]
0x54a0e6: and eax, 0xffff
0x54a0eb: shl eax, 5
0x54a0ee: mov ecx, dword ptr [eax + ecx + 0x14]
0x54a0f2: cmp dword ptr [ecx + 0x98], edi
0x54a0f8: mov dword ptr [esp + 0x18], ecx
0x54a0fc: mov dword ptr [esp + 0x14], edi
0x54a100: jle 0x54a1c0
0x54a106: mov dword ptr [esp + 0x2c], edi
0x54a10a: lea ebx, [ebx]
0x54a110: mov ebp, dword ptr [ecx + 0x9c]
0x54a116: mov esi, dword ptr [esp + 0x2c]
0x54a11a: mov eax, dword ptr [ebp + esi + 0x3c]
0x54a11e: add ebp, esi
0x54a120: cmp eax, edi
0x54a122: mov dword ptr [esp + 0x44], edi
0x54a126: jle 0x54a19e
0x54a128: mov dword ptr [esp + 0x1c], edi
0x54a12c: lea esp, [esp]
0x54a130: mov ebx, dword ptr [esp + 0x1c]
0x54a134: mov esi, dword ptr [ebp + 0x40]
0x54a137: add esi, ebx
0x54a139: mov ebx, dword ptr [esi + 0x2c]
0x54a13c: cmp ebx, -1
0x54a13f: je 0x54a183
0x54a141: mov edi, dword ptr [0x6ac528]
0x54a147: mov edi, dword ptr [edi + 0x34]
0x54a14a: mov eax, ebx
0x54a14c: and eax, 0xffff
0x54a151: shl eax, 4
0x54a154: cmp byte ptr [eax + edi + 5], 0
0x54a159: seta al
0x54a15c: test al, al
0x54a15e: jne 0x54a181
0x54a160: mov edi, dword ptr [0x6ac530]
0x54a166: call 0x4d1c20
0x54a16b: mov edx, dword ptr [esp + 0x28]
0x54a16f: mov ecx, dword ptr [esp + 0x18]
0x54a173: mov dword ptr [esi + 0x2c], 0xffffffff
0x54a17a: mov dword ptr [esi + 0x30], 0
0x54a181: xor edi, edi
0x54a183: mov eax, dword ptr [esp + 0x44]
0x54a187: mov ebx, dword ptr [esp + 0x1c]
0x54a18b: mov esi, dword ptr [ebp + 0x3c]
0x54a18e: inc eax
0x54a18f: add ebx, 0x7c
0x54a192: cmp eax, esi
0x54a194: mov dword ptr [esp + 0x44], eax
0x54a198: mov dword ptr [esp + 0x1c], ebx
0x54a19c: jl 0x54a130
0x54a19e: mov eax, dword ptr [esp + 0x14]
0x54a1a2: mov ebx, dword ptr [esp + 0x2c]
0x54a1a6: mov esi, dword ptr [ecx + 0x98]
0x54a1ac: inc eax
0x54a1ad: add ebx, 0x48
0x54a1b0: cmp eax, esi
0x54a1b2: mov dword ptr [esp + 0x14], eax
0x54a1b6: mov dword ptr [esp + 0x2c], ebx
0x54a1ba: jl 0x54a110
0x54a1c0: mov eax, dword ptr [edx + 0x5c]
0x54a1c3: cmp eax, -1
0x54a1c6: je 0x54a2ae
0x54a1cc: mov ecx, dword ptr [0x87bc14]
0x54a1d2: and eax, 0xffff
0x54a1d7: shl eax, 5
0x54a1da: mov eax, dword ptr [eax + ecx + 0x14]
0x54a1de: cmp dword ptr [eax + 0x98], edi
0x54a1e4: mov dword ptr [esp + 0x18], eax
0x54a1e8: mov dword ptr [esp + 0x14], edi
0x54a1ec: jle 0x54a2ae
0x54a1f2: mov dword ptr [esp + 0x1c], edi
0x54a1f6: jmp 0x54a200
0x54a1f8: lea esp, [esp]
0x54a1ff: nop 
0x54a200: mov edx, dword ptr [esp + 0x18]
0x54a204: mov ebp, dword ptr [edx + 0x9c]
0x54a20a: mov ecx, dword ptr [esp + 0x1c]
0x54a20e: mov eax, dword ptr [ebp + ecx + 0x3c]
0x54a212: add ebp, ecx
0x54a214: cmp eax, edi
0x54a216: mov dword ptr [esp + 0x44], edi
0x54a21a: jle 0x54a288
0x54a21c: mov dword ptr [esp + 0x2c], edi
0x54a220: mov esi, dword ptr [ebp + 0x40]
0x54a223: mov ecx, dword ptr [esp + 0x2c]
0x54a227: mov ebx, dword ptr [esi + ecx + 0x2c]
0x54a22b: add esi, ecx
0x54a22d: cmp ebx, -1
0x54a230: je 0x54a26d
0x54a232: mov ecx, dword ptr [0x6ac528]
0x54a238: mov edx, dword ptr [ecx + 0x34]
0x54a23b: mov eax, ebx
0x54a23d: and eax, 0xffff
0x54a242: shl eax, 4
0x54a245: mov cl, byte ptr [eax + edx + 5]
0x54a249: test cl, cl
0x54a24b: seta al
0x54a24e: test al, al
0x54a250: jne 0x54a26d
0x54a252: mov edi, dword ptr [0x6ac530]
0x54a258: call 0x4d1c20
0x54a25d: mov dword ptr [esi + 0x2c], 0xffffffff
0x54a264: mov dword ptr [esi + 0x30], 0
0x54a26b: xor edi, edi
0x54a26d: mov eax, dword ptr [esp + 0x44]
0x54a271: mov edx, dword ptr [esp + 0x2c]
0x54a275: mov ecx, dword ptr [ebp + 0x3c]
0x54a278: inc eax
0x54a279: add edx, 0x7c
0x54a27c: cmp eax, ecx
0x54a27e: mov dword ptr [esp + 0x44], eax
0x54a282: mov dword ptr [esp + 0x2c], edx
0x54a286: jl 0x54a220
0x54a288: mov eax, dword ptr [esp + 0x14]
0x54a28c: mov esi, dword ptr [esp + 0x1c]
0x54a290: mov ecx, dword ptr [esp + 0x18]
0x54a294: mov edx, dword ptr [ecx + 0x98]
0x54a29a: inc eax
0x54a29b: add esi, 0x48
0x54a29e: cmp eax, edx
0x54a2a0: mov dword ptr [esp + 0x14], eax
0x54a2a4: mov dword ptr [esp + 0x1c], esi
0x54a2a8: jl 0x54a200
0x54a2ae: mov edx, dword ptr [esp + 0x28]
0x54a2b2: mov eax, dword ptr [edx + 0x9c]
0x54a2b8: cmp eax, -1
0x54a2bb: je 0x54a39e
0x54a2c1: mov ecx, dword ptr [0x87bc14]
0x54a2c7: and eax, 0xffff
0x54a2cc: shl eax, 5
0x54a2cf: mov eax, dword ptr [eax + ecx + 0x14]
0x54a2d3: cmp dword ptr [eax + 0x98], edi
0x54a2d9: mov dword ptr [esp + 0x18], eax
0x54a2dd: mov dword ptr [esp + 0x14], edi
0x54a2e1: jle 0x54a39e
0x54a2e7: mov dword ptr [esp + 0x1c], edi
0x54a2eb: jmp 0x54a2f0
0x54a2ed: lea ecx, [ecx]
0x54a2f0: mov edx, dword ptr [esp + 0x18]
0x54a2f4: mov ebp, dword ptr [edx + 0x9c]
0x54a2fa: mov ecx, dword ptr [esp + 0x1c]
0x54a2fe: mov eax, dword ptr [ebp + ecx + 0x3c]
0x54a302: add ebp, ecx
0x54a304: cmp eax, edi
0x54a306: mov dword ptr [esp + 0x44], edi
0x54a30a: jle 0x54a378
0x54a30c: mov dword ptr [esp + 0x2c], edi
0x54a310: mov esi, dword ptr [ebp + 0x40]
0x54a313: mov ecx, dword ptr [esp + 0x2c]
0x54a317: mov ebx, dword ptr [esi + ecx + 0x2c]
0x54a31b: add esi, ecx
0x54a31d: cmp ebx, -1
0x54a320: je 0x54a35d
0x54a322: mov ecx, dword ptr [0x6ac528]
0x54a328: mov edx, dword ptr [ecx + 0x34]
0x54a32b: mov eax, ebx
0x54a32d: and eax, 0xffff
0x54a332: shl eax, 4
0x54a335: mov cl, byte ptr [eax + edx + 5]
0x54a339: test cl, cl
0x54a33b: seta al
0x54a33e: test al, al
0x54a340: jne 0x54a35d
0x54a342: mov edi, dword ptr [0x6ac530]
0x54a348: call 0x4d1c20
0x54a34d: mov dword ptr [esi + 0x2c], 0xffffffff
0x54a354: mov dword ptr [esi + 0x30], 0
0x54a35b: xor edi, edi
0x54a35d: mov eax, dword ptr [esp + 0x44]
0x54a361: mov edx, dword ptr [esp + 0x2c]
0x54a365: mov ecx, dword ptr [ebp + 0x3c]
0x54a368: inc eax
0x54a369: add edx, 0x7c
0x54a36c: cmp eax, ecx
0x54a36e: mov dword ptr [esp + 0x44], eax
0x54a372: mov dword ptr [esp + 0x2c], edx
0x54a376: jl 0x54a310
0x54a378: mov eax, dword ptr [esp + 0x14]
0x54a37c: mov esi, dword ptr [esp + 0x1c]
0x54a380: mov ecx, dword ptr [esp + 0x18]
0x54a384: mov edx, dword ptr [ecx + 0x98]
0x54a38a: inc eax
0x54a38b: add esi, 0x48
0x54a38e: cmp eax, edx
0x54a390: mov dword ptr [esp + 0x14], eax
0x54a394: mov dword ptr [esp + 0x1c], esi
0x54a398: jl 0x54a2f0
0x54a39e: mov edx, dword ptr [esp + 0x28]
0x54a3a2: mov eax, dword ptr [edx + 0x4c]
0x54a3a5: cmp eax, -1
0x54a3a8: je 0x54a48e
0x54a3ae: mov ecx, dword ptr [0x87bc14]
0x54a3b4: and eax, 0xffff
0x54a3b9: shl eax, 5
0x54a3bc: mov eax, dword ptr [eax + ecx + 0x14]
0x54a3c0: cmp dword ptr [eax + 0x98], edi
0x54a3c6: mov dword ptr [esp + 0x18], eax
0x54a3ca: mov dword ptr [esp + 0x14], edi
0x54a3ce: jle 0x54a48e
0x54a3d4: mov dword ptr [esp + 0x1c], edi
0x54a3d8: jmp 0x54a3e0
0x54a3da: lea ebx, [ebx]
0x54a3e0: mov edx, dword ptr [esp + 0x18]
0x54a3e4: mov ebp, dword ptr [edx + 0x9c]
0x54a3ea: mov ecx, dword ptr [esp + 0x1c]
0x54a3ee: mov eax, dword ptr [ebp + ecx + 0x3c]
0x54a3f2: add ebp, ecx
0x54a3f4: cmp eax, edi
0x54a3f6: mov dword ptr [esp + 0x44], edi
0x54a3fa: jle 0x54a468
0x54a3fc: mov dword ptr [esp + 0x2c], edi
0x54a400: mov esi, dword ptr [ebp + 0x40]
0x54a403: mov ecx, dword ptr [esp + 0x2c]
0x54a407: mov ebx, dword ptr [esi + ecx + 0x2c]
0x54a40b: add esi, ecx
0x54a40d: cmp ebx, -1
0x54a410: je 0x54a44d
0x54a412: mov ecx, dword ptr [0x6ac528]
0x54a418: mov edx, dword ptr [ecx + 0x34]
0x54a41b: mov eax, ebx
0x54a41d: and eax, 0xffff
0x54a422: shl eax, 4
0x54a425: mov cl, byte ptr [eax + edx + 5]
0x54a429: test cl, cl
0x54a42b: seta al
0x54a42e: test al, al
0x54a430: jne 0x54a44d
0x54a432: mov edi, dword ptr [0x6ac530]
0x54a438: call 0x4d1c20
0x54a43d: mov dword ptr [esi + 0x2c], 0xffffffff
0x54a444: mov dword ptr [esi + 0x30], 0
0x54a44b: xor edi, edi
0x54a44d: mov eax, dword ptr [esp + 0x44]
0x54a451: mov edx, dword ptr [esp + 0x2c]
0x54a455: mov ecx, dword ptr [ebp + 0x3c]
0x54a458: inc eax
0x54a459: add edx, 0x7c
0x54a45c: cmp eax, ecx
0x54a45e: mov dword ptr [esp + 0x44], eax
0x54a462: mov dword ptr [esp + 0x2c], edx
0x54a466: jl 0x54a400
0x54a468: mov eax, dword ptr [esp + 0x14]
0x54a46c: mov esi, dword ptr [esp + 0x1c]
0x54a470: mov ecx, dword ptr [esp + 0x18]
0x54a474: mov edx, dword ptr [ecx + 0x98]
0x54a47a: inc eax
0x54a47b: add esi, 0x48
0x54a47e: cmp eax, edx
0x54a480: mov dword ptr [esp + 0x14], eax
0x54a484: mov dword ptr [esp + 0x1c], esi
0x54a488: jl 0x54a3e0
0x54a48e: mov edx, dword ptr [esp + 0x28]
0x54a492: mov eax, dword ptr [edx + 0x8c]
0x54a498: cmp eax, -1
0x54a49b: je 0x54a57e
0x54a4a1: mov ecx, dword ptr [0x87bc14]
0x54a4a7: and eax, 0xffff
0x54a4ac: shl eax, 5
0x54a4af: mov eax, dword ptr [eax + ecx + 0x14]
0x54a4b3: cmp dword ptr [eax + 0x98], edi
0x54a4b9: mov dword ptr [esp + 0x18], eax
0x54a4bd: mov dword ptr [esp + 0x14], edi
0x54a4c1: jle 0x54a57e
0x54a4c7: mov dword ptr [esp + 0x28], edi
0x54a4cb: jmp 0x54a4d0
0x54a4cd: lea ecx, [ecx]
0x54a4d0: mov edx, dword ptr [esp + 0x18]
0x54a4d4: mov ebp, dword ptr [edx + 0x9c]
0x54a4da: mov ecx, dword ptr [esp + 0x28]
0x54a4de: mov eax, dword ptr [ebp + ecx + 0x3c]
0x54a4e2: add ebp, ecx
0x54a4e4: cmp eax, edi
0x54a4e6: mov dword ptr [esp + 0x44], edi
0x54a4ea: jle 0x54a558
0x54a4ec: mov dword ptr [esp + 0x2c], edi
0x54a4f0: mov esi, dword ptr [ebp + 0x40]
0x54a4f3: mov ecx, dword ptr [esp + 0x2c]
0x54a4f7: mov ebx, dword ptr [esi + ecx + 0x2c]
0x54a4fb: add esi, ecx
0x54a4fd: cmp ebx, -1
0x54a500: je 0x54a53d
0x54a502: mov ecx, dword ptr [0x6ac528]
0x54a508: mov edx, dword ptr [ecx + 0x34]
0x54a50b: mov eax, ebx
0x54a50d: and eax, 0xffff
0x54a512: shl eax, 4
0x54a515: mov cl, byte ptr [eax + edx + 5]
0x54a519: test cl, cl
0x54a51b: seta al
0x54a51e: test al, al
0x54a520: jne 0x54a53d
0x54a522: mov edi, dword ptr [0x6ac530]
0x54a528: call 0x4d1c20
0x54a52d: mov dword ptr [esi + 0x2c], 0xffffffff
0x54a534: mov dword ptr [esi + 0x30], 0
0x54a53b: xor edi, edi
0x54a53d: mov eax, dword ptr [esp + 0x44]
0x54a541: mov edx, dword ptr [esp + 0x2c]
0x54a545: mov ecx, dword ptr [ebp + 0x3c]
0x54a548: inc eax
0x54a549: add edx, 0x7c
0x54a54c: cmp eax, ecx
0x54a54e: mov dword ptr [esp + 0x44], eax
0x54a552: mov dword ptr [esp + 0x2c], edx
0x54a556: jl 0x54a4f0
0x54a558: mov eax, dword ptr [esp + 0x14]
0x54a55c: mov esi, dword ptr [esp + 0x28]
0x54a560: mov ecx, dword ptr [esp + 0x18]
0x54a564: mov edx, dword ptr [ecx + 0x98]
0x54a56a: inc eax
0x54a56b: add esi, 0x48
0x54a56e: cmp eax, edx
0x54a570: mov dword ptr [esp + 0x14], eax
0x54a574: mov dword ptr [esp + 0x28], esi
0x54a578: jl 0x54a4d0
0x54a57e: mov eax, dword ptr [esp + 0x30]
0x54a582: mov edx, dword ptr [esp + 0x20]
0x54a586: mov ecx, dword ptr [edx + 0x3c]
0x54a589: inc eax
0x54a58a: mov dword ptr [esp + 0x30], eax
0x54a58e: movsx eax, ax
0x54a591: cmp eax, ecx
0x54a593: jl 0x54a0c0
0x54a599: mov edx, dword ptr [esp + 0x24]
0x54a59d: mov eax, dword ptr [0x724a50]
0x54a5a2: call 0x4d0510
0x54a5a7: mov al, byte ptr [esp + 0x13]
0x54a5ab: pop edi
0x54a5ac: pop esi
0x54a5ad: pop ebp
0x54a5ae: pop ebx
0x54a5af: add esp, 0x2c
0x54a5b2: ret 
0x54a5b3: mov eax, dword ptr [ebx + 0x38]
0x54a5b6: cmp eax, -1
0x54a5b9: mov byte ptr [esp + 0x13], 0
0x54a5be: je 0x54a5cc
0x54a5c0: lea esi, [edx + 0xc]
0x54a5c3: push eax
0x54a5c4: call 0x456900
0x54a5c9: add esp, 4
0x54a5cc: mov eax, dword ptr [ebx + 0x3c]
0x54a5cf: test eax, eax
0x54a5d1: mov dword ptr [esp + 0x1c], 0
0x54a5d9: jle 0x54a723
0x54a5df: xor edi, edi
0x54a5e1: mov ecx, dword ptr [ebx + 0x40]
0x54a5e4: mov al, byte ptr [esp + 0x12]
0x54a5e8: lea esi, [edi + edi*4]
0x54a5eb: shl esi, 5
0x54a5ee: add esi, ecx
0x54a5f0: test al, al
0x54a5f2: je 0x54a5ff
0x54a5f4: mov dword ptr [ebp + edi*4 + 0xd4], 0xffffffff
0x54a5ff: mov ax, word ptr [esp + 0x44]
0x54a604: test ax, ax
0x54a607: jne 0x54a62f
0x54a609: mov eax, dword ptr [esi + 0x3c]
0x54a60c: cmp eax, -1
0x54a60f: je 0x54a639
0x54a611: mov ecx, dword ptr [esp + 0x1c]
0x54a615: mov edx, dword ptr [esp + 0x24]
0x54a619: push 1
0x54a61b: push ecx
0x54a61c: push eax
0x54a61d: push edx
0x54a61e: call 0x54d9f0
0x54a623: add esp, 0x10
0x54a626: mov dword ptr [ebp + edi*4 + 0xd4], eax
0x54a62d: jmp 0x54a639
0x54a62f: cmp ax, 2
0x54a633: je 0x54ac96
0x54a639: mov al, byte ptr [ebp + 0x4e]
0x54a63c: test al, al
0x54a63e: jne 0x54ac96
0x54a644: mov dl, byte ptr [esp + 0x48]
0x54a648: test dl, dl
0x54a64a: mov ecx, dword ptr [esi + 0x4c]
0x54a64d: je 0x54a65c
0x54a64f: mov eax, dword ptr [esi + 0x8c]
0x54a655: cmp eax, -1
0x54a658: je 0x54a65c
0x54a65a: mov ecx, eax
0x54a65c: cmp ecx, -1
0x54a65f: je 0x54a70e
0x54a665: mov eax, dword ptr [ebp + edi*4 + 0xd4]
0x54a66c: cmp eax, -1
0x54a66f: je 0x54a6c1
0x54a671: cmp word ptr [esp + 0x44], 0
0x54a677: jne 0x54a67e
0x54a679: test byte ptr [esi], 1
0x54a67c: jne 0x54a6c1
0x54a67e: cmp dl, byte ptr [ebp + 0x4d]
0x54a681: je 0x54a6b2
0x54a683: test byte ptr [esi], 4
0x54a686: je 0x54a6b2
0x54a688: mov eax, dword ptr [esp + 0x1c]
0x54a68c: push 2
0x54a68e: push eax
0x54a68f: push ecx
0x54a690: mov ecx, dword ptr [esp + 0x30]
0x54a694: push ecx
0x54a695: call 0x54d9f0
0x54a69a: mov ebx, eax
0x54a69c: add esp, 0x10
0x54a69f: cmp ebx, -1
0x54a6a2: je 0x54a70a
0x54a6a4: mov edx, dword ptr [ebp + edi*4 + 0xd4]
0x54a6ab: mov eax, dword ptr [esi + 0xc]
0x54a6ae: push edx
0x54a6af: push eax
0x54a6b0: jmp 0x54a6f9
0x54a6b2: mov dl, byte ptr [esp + 0x12]
0x54a6b6: test dl, dl
0x54a6b8: jne 0x54a70e
0x54a6ba: call 0x54dd90
0x54a6bf: jmp 0x54a70e
0x54a6c1: mov edx, dword ptr [esp + 0x1c]
0x54a6c5: mov eax, dword ptr [esp + 0x24]
0x54a6c9: push 2
0x54a6cb: push edx
0x54a6cc: push ecx
0x54a6cd: push eax
0x54a6ce: call 0x54d9f0
0x54a6d3: mov ebx, eax
0x54a6d5: add esp, 0x10
0x54a6d8: cmp ebx, -1
0x54a6db: je 0x54a70a
0x54a6dd: cmp word ptr [esp + 0x44], 0
0x54a6e3: jne 0x54a6f2
0x54a6e5: test byte ptr [esi], 1
0x54a6e8: je 0x54a703
0x54a6ea: mov ecx, dword ptr [esi + 8]
0x54a6ed: push -1
0x54a6ef: push ecx
0x54a6f0: jmp 0x54a6f9
0x54a6f2: push -1
0x54a6f4: push 0x40000000
0x54a6f9: push 0
0x54a6fb: call 0x54af60
0x54a700: add esp, 0xc
0x54a703: mov dword ptr [ebp + edi*4 + 0xd4], ebx
0x54a70a: mov ebx, dword ptr [esp + 0x20]
0x54a70e: mov eax, dword ptr [esp + 0x1c]
0x54a712: inc eax
0x54a713: movsx edi, ax
0x54a716: mov dword ptr [esp + 0x1c], eax
0x54a71a: cmp edi, dword ptr [ebx + 0x3c]
0x54a71d: jl 0x54a5e1
0x54a723: cmp word ptr [ebp + 0x50], 0
0x54a728: jne 0x54ac7a
0x54a72e: mov eax, dword ptr [ebx + 0x20]
0x54a731: push eax
0x54a732: mov eax, dword ptr [esp + 0x44]
0x54a736: call 0x54bb20
0x54a73b: add esp, 4
0x54a73e: cmp ax, 0xffff
0x54a742: jne 0x54ac7a
0x54a748: mov eax, dword ptr [ebx + 0x3c]
0x54a74b: test eax, eax
0x54a74d: mov dword ptr [esp + 0x34], 0
0x54a755: jle 0x54ac68
0x54a75b: xor eax, eax
0x54a75d: xor edx, edx
0x54a75f: nop 
0x54a760: mov ecx, dword ptr [esp + 0x20]
0x54a764: mov edi, dword ptr [ecx + 0x40]
0x54a767: lea eax, [eax + eax*4]
0x54a76a: shl eax, 5
0x54a76d: mov ecx, dword ptr [eax + edi + 0x3c]
0x54a771: add eax, edi
0x54a773: cmp ecx, -1
0x54a776: mov dword ptr [esp + 0x28], eax
0x54a77a: je 0x54a871
0x54a780: mov eax, dword ptr [0x87bc14]
0x54a785: and ecx, 0xffff
0x54a78b: shl ecx, 5
0x54a78e: mov ecx, dword ptr [ecx + eax + 0x14]
0x54a792: cmp word ptr [ecx + 4], 0x20
0x54a797: mov dword ptr [esp + 0x1c], ecx
0x54a79b: jne 0x54ac68
0x54a7a1: cmp dword ptr [ecx + 0x98], edx
0x54a7a7: mov dword ptr [esp + 0x18], edx
0x54a7ab: jle 0x54a86d
0x54a7b1: mov dword ptr [esp + 0x30], edx
0x54a7b5: jmp 0x54a7c0
0x54a7b7: lea esp, [esp]
0x54a7be: mov edi, edi
0x54a7c0: mov ebp, dword ptr [ecx + 0x9c]
0x54a7c6: mov esi, dword ptr [esp + 0x30]
0x54a7ca: mov eax, dword ptr [ebp + esi + 0x3c]
0x54a7ce: add ebp, esi
0x54a7d0: cmp eax, edx
0x54a7d2: mov dword ptr [esp + 0x14], edx
0x54a7d6: jle 0x54a84b
0x54a7d8: mov dword ptr [esp + 0x2c], edx
0x54a7dc: lea esp, [esp]
0x54a7e0: mov esi, dword ptr [ebp + 0x40]
0x54a7e3: mov edi, dword ptr [esp + 0x2c]
0x54a7e7: mov ebx, dword ptr [esi + edi + 0x2c]
0x54a7eb: add esi, edi
0x54a7ed: cmp ebx, -1
0x54a7f0: je 0x54a830
0x54a7f2: mov edi, dword ptr [0x6ac528]
0x54a7f8: mov edi, dword ptr [edi + 0x34]
0x54a7fb: mov eax, ebx
0x54a7fd: and eax, 0xffff
0x54a802: shl eax, 4
0x54a805: cmp byte ptr [eax + edi + 5], 0
0x54a80a: seta al
0x54a80d: test al, al
0x54a80f: jne 0x54a830
0x54a811: mov edi, dword ptr [0x6ac530]
0x54a817: call 0x4d1c20
0x54a81c: mov ecx, dword ptr [esp + 0x1c]
0x54a820: mov dword ptr [esi + 0x2c], 0xffffffff
0x54a827: mov dword ptr [esi + 0x30], 0
0x54a82e: xor edx, edx
0x54a830: mov eax, dword ptr [esp + 0x14]
0x54a834: mov edi, dword ptr [esp + 0x2c]
0x54a838: mov esi, dword ptr [ebp + 0x3c]
0x54a83b: inc eax
0x54a83c: add edi, 0x7c
0x54a83f: cmp eax, esi
0x54a841: mov dword ptr [esp + 0x14], eax
0x54a845: mov dword ptr [esp + 0x2c], edi
0x54a849: jl 0x54a7e0
0x54a84b: mov eax, dword ptr [esp + 0x18]
0x54a84f: mov edi, dword ptr [esp + 0x30]
0x54a853: mov esi, dword ptr [ecx + 0x98]
0x54a859: inc eax
0x54a85a: add edi, 0x48
0x54a85d: cmp eax, esi
0x54a85f: mov dword ptr [esp + 0x18], eax
0x54a863: mov dword ptr [esp + 0x30], edi
0x54a867: jl 0x54a7c0
0x54a86d: mov eax, dword ptr [esp + 0x28]
0x54a871: mov ecx, dword ptr [eax + 0x5c]
0x54a874: cmp ecx, -1
0x54a877: je 0x54a961
0x54a87d: mov eax, dword ptr [0x87bc14]
0x54a882: and ecx, 0xffff
0x54a888: shl ecx, 5
0x54a88b: mov ecx, dword ptr [ecx + eax + 0x14]
0x54a88f: cmp word ptr [ecx + 4], 0x20
0x54a894: mov dword ptr [esp + 0x1c], ecx
0x54a898: jne 0x54ac68
0x54a89e: cmp dword ptr [ecx + 0x98], edx
0x54a8a4: mov dword ptr [esp + 0x18], edx
0x54a8a8: jle 0x54a95d
0x54a8ae: mov dword ptr [esp + 0x2c], edx
0x54a8b2: mov ebp, dword ptr [ecx + 0x9c]
0x54a8b8: mov esi, dword ptr [esp + 0x2c]
0x54a8bc: mov eax, dword ptr [ebp + esi + 0x3c]
0x54a8c0: add ebp, esi
0x54a8c2: cmp eax, edx
0x54a8c4: mov dword ptr [esp + 0x14], edx
0x54a8c8: jle 0x54a93b
0x54a8ca: mov dword ptr [esp + 0x30], edx
0x54a8ce: mov edi, edi
0x54a8d0: mov esi, dword ptr [ebp + 0x40]
0x54a8d3: mov edi, dword ptr [esp + 0x30]
0x54a8d7: mov ebx, dword ptr [esi + edi + 0x2c]
0x54a8db: add esi, edi
0x54a8dd: cmp ebx, -1
0x54a8e0: je 0x54a920
0x54a8e2: mov edi, dword ptr [0x6ac528]
0x54a8e8: mov edi, dword ptr [edi + 0x34]
0x54a8eb: mov eax, ebx
0x54a8ed: and eax, 0xffff
0x54a8f2: shl eax, 4
0x54a8f5: cmp byte ptr [eax + edi + 5], 0
0x54a8fa: seta al
0x54a8fd: test al, al
0x54a8ff: jne 0x54a920
0x54a901: mov edi, dword ptr [0x6ac530]
0x54a907: call 0x4d1c20
0x54a90c: mov ecx, dword ptr [esp + 0x1c]
0x54a910: mov dword ptr [esi + 0x2c], 0xffffffff
0x54a917: mov dword ptr [esi + 0x30], 0
0x54a91e: xor edx, edx
0x54a920: mov eax, dword ptr [esp + 0x14]
0x54a924: mov edi, dword ptr [esp + 0x30]
0x54a928: mov esi, dword ptr [ebp + 0x3c]
0x54a92b: inc eax
0x54a92c: add edi, 0x7c
0x54a92f: cmp eax, esi
0x54a931: mov dword ptr [esp + 0x14], eax
0x54a935: mov dword ptr [esp + 0x30], edi
0x54a939: jl 0x54a8d0
0x54a93b: mov eax, dword ptr [esp + 0x18]
0x54a93f: mov edi, dword ptr [esp + 0x2c]
0x54a943: mov esi, dword ptr [ecx + 0x98]
0x54a949: inc eax
0x54a94a: add edi, 0x48
0x54a94d: cmp eax, esi
0x54a94f: mov dword ptr [esp + 0x18], eax
0x54a953: mov dword ptr [esp + 0x2c], edi
0x54a957: jl 0x54a8b2
0x54a95d: mov eax, dword ptr [esp + 0x28]
0x54a961: mov eax, dword ptr [eax + 0x9c]
0x54a967: cmp eax, -1
0x54a96a: je 0x54aa5d
0x54a970: mov ecx, dword ptr [0x87bc14]
0x54a976: and eax, 0xffff
0x54a97b: shl eax, 5
0x54a97e: mov eax, dword ptr [eax + ecx + 0x14]
0x54a982: cmp word ptr [eax + 4], 0x20
0x54a987: mov dword ptr [esp + 0x1c], eax
0x54a98b: jne 0x54ac68
0x54a991: cmp dword ptr [eax + 0x98], edx
0x54a997: mov dword ptr [esp + 0x18], edx
0x54a99b: jle 0x54aa5d
0x54a9a1: mov dword ptr [esp + 0x2c], edx
0x54a9a5: jmp 0x54a9b0
0x54a9a7: lea esp, [esp]
0x54a9ae: mov edi, edi
0x54a9b0: mov ecx, dword ptr [esp + 0x1c]
0x54a9b4: mov ebp, dword ptr [ecx + 0x9c]
0x54a9ba: mov ecx, dword ptr [esp + 0x2c]
0x54a9be: mov eax, dword ptr [ebp + ecx + 0x3c]
0x54a9c2: add ebp, ecx
0x54a9c4: cmp eax, edx
0x54a9c6: mov dword ptr [esp + 0x14], edx
0x54a9ca: jle 0x54aa37
0x54a9cc: mov dword ptr [esp + 0x30], edx
0x54a9d0: mov esi, dword ptr [ebp + 0x40]
0x54a9d3: mov ecx, dword ptr [esp + 0x30]
0x54a9d7: mov ebx, dword ptr [esi + ecx + 0x2c]
0x54a9db: add esi, ecx
0x54a9dd: cmp ebx, -1
0x54a9e0: je 0x54aa1c
0x54a9e2: mov ecx, dword ptr [0x6ac528]
0x54a9e8: mov ecx, dword ptr [ecx + 0x34]
0x54a9eb: mov eax, ebx
0x54a9ed: and eax, 0xffff
0x54a9f2: shl eax, 4
0x54a9f5: cmp byte ptr [eax + ecx + 5], 0
0x54a9fa: seta al
0x54a9fd: test al, al
0x54a9ff: jne 0x54aa1c
0x54aa01: mov edi, dword ptr [0x6ac530]
0x54aa07: call 0x4d1c20
0x54aa0c: mov dword ptr [esi + 0x2c], 0xffffffff
0x54aa13: mov dword ptr [esi + 0x30], 0
0x54aa1a: xor edx, edx
0x54aa1c: mov eax, dword ptr [esp + 0x14]
0x54aa20: mov esi, dword ptr [esp + 0x30]
0x54aa24: mov ecx, dword ptr [ebp + 0x3c]
0x54aa27: inc eax
0x54aa28: add esi, 0x7c
0x54aa2b: cmp eax, ecx
0x54aa2d: mov dword ptr [esp + 0x14], eax
0x54aa31: mov dword ptr [esp + 0x30], esi
0x54aa35: jl 0x54a9d0
0x54aa37: mov eax, dword ptr [esp + 0x18]
0x54aa3b: mov edi, dword ptr [esp + 0x2c]
0x54aa3f: mov ecx, dword ptr [esp + 0x1c]
0x54aa43: mov esi, dword ptr [ecx + 0x98]
0x54aa49: inc eax
0x54aa4a: add edi, 0x48
0x54aa4d: cmp eax, esi
0x54aa4f: mov dword ptr [esp + 0x18], eax
0x54aa53: mov dword ptr [esp + 0x2c], edi
0x54aa57: jl 0x54a9b0
0x54aa5d: mov eax, dword ptr [esp + 0x28]
0x54aa61: mov eax, dword ptr [eax + 0x4c]
0x54aa64: cmp eax, -1
0x54aa67: je 0x54ab4f
0x54aa6d: mov ecx, dword ptr [0x87bc14]
0x54aa73: and eax, 0xffff
0x54aa78: shl eax, 5
0x54aa7b: mov eax, dword ptr [eax + ecx + 0x14]
0x54aa7f: cmp word ptr [eax + 4], 0x20
0x54aa84: mov dword ptr [esp + 0x1c], eax
0x54aa88: jne 0x54ac68
0x54aa8e: cmp dword ptr [eax + 0x98], edx
0x54aa94: mov dword ptr [esp + 0x18], edx
0x54aa98: jle 0x54ab4f
0x54aa9e: mov dword ptr [esp + 0x2c], edx
0x54aaa2: mov eax, dword ptr [esp + 0x1c]
0x54aaa6: mov ebp, dword ptr [eax + 0x9c]
0x54aaac: mov ecx, dword ptr [esp + 0x2c]
0x54aab0: mov eax, dword ptr [ebp + ecx + 0x3c]
0x54aab4: add ebp, ecx
0x54aab6: cmp eax, edx
0x54aab8: mov dword ptr [esp + 0x14], edx
0x54aabc: jle 0x54ab29
0x54aabe: mov dword ptr [esp + 0x30], edx
0x54aac2: mov esi, dword ptr [ebp + 0x40]
0x54aac5: mov ecx, dword ptr [esp + 0x30]
0x54aac9: mov ebx, dword ptr [esi + ecx + 0x2c]
0x54aacd: add esi, ecx
0x54aacf: cmp ebx, -1
0x54aad2: je 0x54ab0e
0x54aad4: mov eax, dword ptr [0x6ac528]
0x54aad9: mov eax, dword ptr [eax + 0x34]
0x54aadc: mov ecx, ebx
0x54aade: and ecx, 0xffff
0x54aae4: shl ecx, 4
0x54aae7: cmp byte ptr [ecx + eax + 5], 0
0x54aaec: seta al
0x54aaef: test al, al
0x54aaf1: jne 0x54ab0e
0x54aaf3: mov edi, dword ptr [0x6ac530]
0x54aaf9: call 0x4d1c20
0x54aafe: mov dword ptr [esi + 0x2c], 0xffffffff
0x54ab05: mov dword ptr [esi + 0x30], 0
0x54ab0c: xor edx, edx
0x54ab0e: mov eax, dword ptr [esp + 0x14]
0x54ab12: mov esi, dword ptr [esp + 0x30]
0x54ab16: mov ecx, dword ptr [ebp + 0x3c]
0x54ab19: inc eax
0x54ab1a: add esi, 0x7c
0x54ab1d: cmp eax, ecx
0x54ab1f: mov dword ptr [esp + 0x14], eax
0x54ab23: mov dword ptr [esp + 0x30], esi
0x54ab27: jl 0x54aac2
0x54ab29: mov eax, dword ptr [esp + 0x18]
0x54ab2d: mov edi, dword ptr [esp + 0x2c]
0x54ab31: mov ecx, dword ptr [esp + 0x1c]
0x54ab35: mov esi, dword ptr [ecx + 0x98]
0x54ab3b: inc eax
0x54ab3c: add edi, 0x48
0x54ab3f: cmp eax, esi
0x54ab41: mov dword ptr [esp + 0x18], eax
0x54ab45: mov dword ptr [esp + 0x2c], edi
0x54ab49: jl 0x54aaa2
0x54ab4f: mov eax, dword ptr [esp + 0x28]
0x54ab53: mov eax, dword ptr [eax + 0x8c]
0x54ab59: cmp eax, -1
0x54ab5c: je 0x54ac4d
0x54ab62: mov ecx, dword ptr [0x87bc14]
0x54ab68: and eax, 0xffff
0x54ab6d: shl eax, 5
0x54ab70: mov eax, dword ptr [eax + ecx + 0x14]
0x54ab74: cmp word ptr [eax + 4], 0x20
0x54ab79: mov dword ptr [esp + 0x1c], eax
0x54ab7d: jne 0x54ac68
0x54ab83: cmp dword ptr [eax + 0x98], edx
0x54ab89: mov dword ptr [esp + 0x18], edx
0x54ab8d: jle 0x54ac4d
0x54ab93: mov dword ptr [esp + 0x2c], edx
0x54ab97: jmp 0x54aba0
0x54ab99: lea esp, [esp]
0x54aba0: mov eax, dword ptr [esp + 0x1c]
0x54aba4: mov ebp, dword ptr [eax + 0x9c]
0x54abaa: mov ecx, dword ptr [esp + 0x2c]
0x54abae: mov eax, dword ptr [ebp + ecx + 0x3c]
0x54abb2: add ebp, ecx
0x54abb4: cmp eax, edx
0x54abb6: mov dword ptr [esp + 0x14], edx
0x54abba: jle 0x54ac27
0x54abbc: mov dword ptr [esp + 0x30], edx
0x54abc0: mov esi, dword ptr [ebp + 0x40]
0x54abc3: mov ecx, dword ptr [esp + 0x30]
0x54abc7: mov ebx, dword ptr [esi + ecx + 0x2c]
0x54abcb: add esi, ecx
0x54abcd: cmp ebx, -1
0x54abd0: je 0x54ac0c
0x54abd2: mov eax, dword ptr [0x6ac528]
0x54abd7: mov eax, dword ptr [eax + 0x34]
0x54abda: mov ecx, ebx
0x54abdc: and ecx, 0xffff
0x54abe2: shl ecx, 4
0x54abe5: cmp byte ptr [ecx + eax + 5], 0
0x54abea: seta al
0x54abed: test al, al
0x54abef: jne 0x54ac0c
0x54abf1: mov edi, dword ptr [0x6ac530]
0x54abf7: call 0x4d1c20
0x54abfc: mov dword ptr [esi + 0x2c], 0xffffffff
0x54ac03: mov dword ptr [esi + 0x30], 0
0x54ac0a: xor edx, edx
0x54ac0c: mov eax, dword ptr [esp + 0x14]
0x54ac10: mov esi, dword ptr [esp + 0x30]
0x54ac14: mov ecx, dword ptr [ebp + 0x3c]
0x54ac17: inc eax
0x54ac18: add esi, 0x7c
0x54ac1b: cmp eax, ecx
0x54ac1d: mov dword ptr [esp + 0x14], eax
0x54ac21: mov dword ptr [esp + 0x30], esi
0x54ac25: jl 0x54abc0
0x54ac27: mov eax, dword ptr [esp + 0x18]
0x54ac2b: mov edi, dword ptr [esp + 0x2c]
0x54ac2f: mov ecx, dword ptr [esp + 0x1c]
0x54ac33: mov esi, dword ptr [ecx + 0x98]
0x54ac39: inc eax
0x54ac3a: add edi, 0x48
0x54ac3d: cmp eax, esi
0x54ac3f: mov dword ptr [esp + 0x18], eax
0x54ac43: mov dword ptr [esp + 0x2c], edi
0x54ac47: jl 0x54aba0
0x54ac4d: mov eax, dword ptr [esp + 0x34]
0x54ac51: mov ecx, dword ptr [esp + 0x20]
0x54ac55: mov esi, dword ptr [ecx + 0x3c]
0x54ac58: inc eax
0x54ac59: mov dword ptr [esp + 0x34], eax
0x54ac5d: movsx eax, ax
0x54ac60: cmp eax, esi
0x54ac62: jl 0x54a760
0x54ac68: mov edx, dword ptr [esp + 0x24]
0x54ac6c: mov eax, dword ptr [0x724a50]
0x54ac71: call 0x4d0510
0x54ac76: mov ebp, dword ptr [esp + 0x38]
0x54ac7a: mov ax, word ptr [esp + 0x44]
0x54ac7f: mov dl, byte ptr [esp + 0x48]
0x54ac83: pop edi
0x54ac84: pop esi
0x54ac85: mov word ptr [ebp + 0x52], ax
0x54ac89: mov al, byte ptr [esp + 0xb]
0x54ac8d: mov byte ptr [ebp + 0x4d], dl
0x54ac90: pop ebp
0x54ac91: pop ebx
0x54ac92: add esp, 0x2c
0x54ac95: ret 
0x54ac96: cmp word ptr [ebp + 0x52], 2
0x54ac9b: je 0x54a70e
0x54aca1: fld dword ptr [0x672ac0]
0x54aca7: fld dword ptr [esp + 0x4c]
0x54acab: fucompp 
0x54acad: fnstsw ax
0x54acaf: test ah, 0x44
0x54acb2: jnp 0x54acd3
0x54acb4: mov edx, dword ptr [ebp + edi*4 + 0xd4]
0x54acbb: mov eax, dword ptr [esp + 0x4c]
0x54acbf: push edx
0x54acc0: push eax
0x54acc1: push 0
0x54acc3: or ebx, 0xffffffff
0x54acc6: call 0x54af60
0x54accb: add esp, 0xc
0x54acce: jmp 0x54a70a
0x54acd3: mov eax, dword ptr [ebp + edi*4 + 0xd4]
0x54acda: cmp eax, -1
0x54acdd: je 0x54ad05
0x54acdf: test byte ptr [esi], 2
0x54ace2: jne 0x54acef
0x54ace4: cmp dword ptr [esi + 0x5c], -1
0x54ace8: jne 0x54ad05
0x54acea: test byte ptr [ebx], 2
0x54aced: jne 0x54ad05
0x54acef: mov ecx, dword ptr [esi + 0xc]
0x54acf2: push eax
0x54acf3: push ecx
0x54acf4: push 0
0x54acf6: or ebx, 0xffffffff
0x54acf9: call 0x54af60
0x54acfe: mov ebx, dword ptr [esp + 0x2c]
0x54ad02: add esp, 0xc
0x54ad05: mov ecx, dword ptr [esi + 0x5c]
0x54ad08: cmp ecx, -1
0x54ad0b: je 0x54a70e
0x54ad11: mov al, byte ptr [esp + 0x48]
0x54ad15: test al, al
0x54ad17: je 0x54ad26
0x54ad19: mov eax, dword ptr [esi + 0x9c]
0x54ad1f: cmp eax, -1
0x54ad22: je 0x54ad26
0x54ad24: mov ecx, eax
0x54ad26: test byte ptr [esi], 2
0x54ad29: je 0x54ad45
0x54ad2b: mov edx, dword ptr [esp + 0x1c]
0x54ad2f: mov eax, dword ptr [esp + 0x24]
0x54ad33: push 4
0x54ad35: push edx
0x54ad36: push ecx
0x54ad37: push eax
0x54ad38: call 0x54d9f0
0x54ad3d: add esp, 0x10
0x54ad40: jmp 0x54a70e
0x54ad45: mov edi, dword ptr [ebp + edi*4 + 0xd4]
0x54ad4c: cmp edi, -1
0x54ad4f: je 0x54a70e
0x54ad55: mov edx, dword ptr [0x7252c0]
0x54ad5b: mov eax, dword ptr [edx + 0x34]
0x54ad5e: mov esi, edi
0x54ad60: and esi, 0xffff
0x54ad66: imul esi, esi, 0xb0
0x54ad6c: add esi, eax
0x54ad6e: cmp word ptr [esi + 0x8c], -1
0x54ad76: je 0x54a70e
0x54ad7c: mov eax, edi
0x54ad7e: call 0x54dd90
0x54ad83: mov word ptr [esi + 2], 3
0x54ad89: jmp 0x54a70e
0x54ad8e: pop edi
0x54ad8f: pop esi
0x54ad90: pop ebp
0x54ad91: xor al, al
0x54ad93: pop ebx
0x54ad94: add esp, 0x2c
0x54ad97: ret 
0x54ad98: pop edi
0x54ad99: pop esi
0x54ad9a: pop ebp
0x54ad9b: mov al, cl
0x54ad9d: pop ebx
0x54ad9e: add esp, 0x2c
0x54ada1: ret 
0x54ada2: pop edi
0x54ada3: pop esi
0x54ada4: pop ebp
0x54ada5: mov al, bl
0x54ada7: pop ebx
0x54ada8: add esp, 0x2c
0x54adab: ret 
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
