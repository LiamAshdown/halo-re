// sound_update_looping_states  (Ghidra: FUN_0054d270, still unnamed there)
// address 0x54d270, size 1886 bytes
// name confidence: 0.55   rewrite confidence: 0.8
// evidence: out/phase4/sound_functions.md "Per-tick maintenance of all looping-sound state
// datums: triggers due detail sounds and evicts predicted-resource cache entries that are no
// longer needed."; field mapping cross-checked against types/sound.h (looping_sound,
// SoundLoopingDetail, SoundLoopingTrack) and matches sound_instance_stop.c's (0x54b180) own
// track-sweep exactly, plus one extra per-track gate not present there: if any of a track's five
// referenced tags is not sound_class music, the whole track sweep for that looping_sound stops
// immediately (a real `break`, confirmed by the Ghidra source, not folded away).
// register convention: void, no parameters.
// Phase-4 review (disassembly appended below): the reseed is
//   next = (int)(random(bounds) * lerp(zero, one detail period, location.scale) * 1000.0
//                + Sound.longest_permutation_length + sound_time)
// with the detail's own Sound tag (the draft dropped the *1000 and the permutation length), and
// it runs for every due detail even when the alternate flags skip playing it. The detail sound
// is started with sound_play_new(tag, &location, owner = this looping sound,
// sound_looping_detail_location_proc, &direction, 12, 0).

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "math.h"
#include "sound.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *looping_sound_data; // 0x00724a50, "looping sounds" 0x80 x 0xe4
extern tag_instance *tag_instances;    // 0x0087bc14
extern uint8_t sound_update_toggle;    // 0x00725214
extern data_array *sound_cache_entries; // 0x006ac528
extern struct cache *sound_cache;       // 0x006ac530
extern int32_t sound_time;              // 0x0072520c
extern random_seed effect_random_seed;   // 0x00719cd4

extern datum_index datum_next(int16_t after_index, data_array *array); // 0x4d0630, memory module
extern void datum_delete(data_array *array, datum_index handle);       // 0x4d0510, memory module
extern void cache_evict_entry(datum_index handle, struct cache *self); // 0x4d1c20, memory module
extern real random_real_range_seeded(random_seed *seed, real min, real max); // 0x4cd170, math module
extern void sound_random_detail_direction(SoundLoopingDetail *detail, real_vector3d *out); // this module, 0x54e4d0
extern uint8_t sound_looping_detail_location_proc(datum_index owner, void *callback_data, sound_location *location); // this module, 0x54dc70
extern datum_index sound_play_new(datum_index definition_index, sound_location *location, datum_index owner_index,
    sound_location_proc location_proc, void *callback_data, int32_t callback_data_size, uint32_t first_person_hint); // 0x549af0

// Releases page-cache references for every resident, unlocked permutation of `tag_id`'s pitch
// ranges. Returns 0 (caller should stop sweeping this looping_sound's tracks) if `tag_id`'s
// sound_class is not music; 1 otherwise. See file header for the extra music-class gate.
static uint8_t sound_release_unused_pages_if_music(TagID tag_id)
{
    Sound *definition;
    int32_t range_index, permutation_index;
    SoundPitchRange *pitch_range;
    SoundPermutation *permutation;
    sound_cache_entry *entry;

    definition = (Sound *)tag_instances[tag_id.index].data;
    if (definition->sound_class != soundclass_music) {
        return 0;
    }

    for (range_index = 0; range_index < definition->pitch_ranges.count; range_index++) {
        pitch_range = (SoundPitchRange *)definition->pitch_ranges.pointer + range_index;
        for (permutation_index = 0; permutation_index < (int32_t)pitch_range->permutations.count;
             permutation_index++) {
            permutation = (SoundPermutation *)pitch_range->permutations.pointer + permutation_index;
            if (permutation->samples_pointer != 0xffffffff) {
                entry = (sound_cache_entry *)((uint8_t *)sound_cache_entries->data +
                    (permutation->samples_pointer & 0xffff) * sizeof(sound_cache_entry));
                if (entry->lock_count == 0) {
                    cache_evict_entry((datum_index)permutation->samples_pointer, sound_cache);
                    permutation->samples_pointer = 0xffffffff;
                    permutation->cache_page = 0;
                }
            }
        }
    }
    return 1;
}

// Per-update pass over every live looping_sound: if it was touched this frame (update_toggle
// matches the global frame toggle), triggers any detail sounds whose next_time has arrived and
// reseeds their next_time; otherwise (not touched this frame -- finished) sweeps its
// SoundLooping definition's tracks for cache pages nothing needs any more and deletes the datum.
void sound_update_looping_states(void)
{
    datum_index handle;
    looping_sound *state;
    SoundLooping *definition;
    int32_t detail_index;
    SoundLoopingDetail *detail;
    float period;
    float random_value;
    real_vector3d direction;
    sound_location location;
    int32_t track_index;
    SoundLoopingTrack *track;

    handle = datum_next(-1, looping_sound_data);
    while (handle != 0xffffffff) {
        state = (looping_sound *)((uint8_t *)looping_sound_data->data + (handle & 0xffff) * sizeof(looping_sound));
        definition = (SoundLooping *)tag_instances[state->definition_index & 0xffff].data;

        if (state->update_toggle == sound_update_toggle) {
            if (state->state != 2) { // looping_sound.state 2: stopped
                for (detail_index = 0; detail_index < (int32_t)definition->detail_sounds.count; detail_index++) {
                    detail = (SoundLoopingDetail *)definition->detail_sounds.pointer + detail_index;
                    if (state->detail_next_time[detail_index] < sound_time &&
                        (detail->sound.tag_id.index != 0xffff || detail->sound.tag_id.id != 0xffff)) {
                        Sound *detail_tag = (Sound *)tag_instances[detail->sound.tag_id.index].data;
                        uint8_t skip_with_alternate = (detail->flags & 1) != 0 && state->alternate != 0;
                        uint8_t skip_without_alternate = (detail->flags & 2) != 0 && state->alternate == 0;
                        if (!skip_with_alternate && !skip_without_alternate) {
                            location.type = (int16_t)((state->location.type == _sound_location_none) + 1);
                            location.scale = state->location.scale;
                            location.gain = detail->gain;
                            sound_random_detail_direction(detail, &direction);
                            sound_looping_detail_location_proc(handle, &direction, &location);
                            sound_play_new(*(datum_index *)&detail->sound.tag_id, &location, handle,
                                sound_looping_detail_location_proc, &direction, sizeof(direction), 0);
                        }

                        period = definition->zero_detail_sound_period +
                            (definition->one_detail_sound_period - definition->zero_detail_sound_period) *
                                state->location.scale;
                        random_value = random_real_range_seeded(&effect_random_seed, detail->random_period_bounds[0],
                            detail->random_period_bounds[1]); // inlined LCG in the binary
                        state->detail_next_time[detail_index] = (int32_t)(random_value * period * 1000.0f +
                            (float)(int32_t)detail_tag->longest_permutation_length + (float)sound_time);
                    }
                }
            }
        } else {
            for (track_index = 0; track_index < definition->tracks.count; track_index++) {
                track = (SoundLoopingTrack *)definition->tracks.pointer + track_index;
                if ((track->start.tag_id.index != 0xffff || track->start.tag_id.id != 0xffff) &&
                    !sound_release_unused_pages_if_music(track->start.tag_id)) {
                    break;
                }
                if ((track->end.tag_id.index != 0xffff || track->end.tag_id.id != 0xffff) &&
                    !sound_release_unused_pages_if_music(track->end.tag_id)) {
                    break;
                }
                if ((track->alternate_end.tag_id.index != 0xffff || track->alternate_end.tag_id.id != 0xffff) &&
                    !sound_release_unused_pages_if_music(track->alternate_end.tag_id)) {
                    break;
                }
                if ((track->loop.tag_id.index != 0xffff || track->loop.tag_id.id != 0xffff) &&
                    !sound_release_unused_pages_if_music(track->loop.tag_id)) {
                    break;
                }
                if ((track->alternate_loop.tag_id.index != 0xffff || track->alternate_loop.tag_id.id != 0xffff) &&
                    !sound_release_unused_pages_if_music(track->alternate_loop.tag_id)) {
                    break;
                }
            }
            datum_delete(looping_sound_data, handle);
        }

        handle = datum_next((int16_t)handle, looping_sound_data);
    }
}

#if 0
Original Ghidra decompilation (0x54d270): see out/phase2/sound/01.md and
scratchpad/sound_packs/0x54d270.md for the full 1886-byte listing.

Disassembly (0x54d270..0x54d9ce, capstone; phase-4 review):

0x54d270: sub esp, 0x74
0x54d273: push edi
0x54d274: mov edi, dword ptr [0x724a50]
0x54d27a: or edx, 0xffffffff
0x54d27d: call 0x4d0630
0x54d282: cmp eax, -1
0x54d285: mov dword ptr [esp + 0x14], eax
0x54d289: je 0x54d9df
0x54d28f: push ebx
0x54d290: push ebp
0x54d291: push esi
0x54d292: mov ecx, dword ptr [edi + 0x34]
0x54d295: mov edx, dword ptr [0x87bc14]
0x54d29b: mov ebx, eax
0x54d29d: and ebx, 0xffff
0x54d2a3: imul ebx, ebx, 0xe4
0x54d2a9: add ebx, ecx
0x54d2ab: mov ecx, dword ptr [ebx + 4]
0x54d2ae: and ecx, 0xffff
0x54d2b4: shl ecx, 5
0x54d2b7: mov ebp, dword ptr [ecx + edx + 0x14]
0x54d2bb: mov cl, byte ptr [ebx + 0x4c]
0x54d2be: cmp cl, byte ptr [0x725214]
0x54d2c4: mov dword ptr [esp + 0x30], ebp
0x54d2c8: je 0x54d816
0x54d2ce: mov eax, dword ptr [ebp + 0x3c]
0x54d2d1: xor ebx, ebx
0x54d2d3: cmp eax, ebx
0x54d2d5: mov dword ptr [esp + 0x2c], ebx
0x54d2d9: jle 0x54d7f9
0x54d2df: xor eax, eax
0x54d2e1: jmp 0x54d2e7
0x54d2e3: mov ebp, dword ptr [esp + 0x30]
0x54d2e7: mov esi, dword ptr [ebp + 0x40]
0x54d2ea: lea eax, [eax + eax*4]
0x54d2ed: shl eax, 5
0x54d2f0: mov ecx, dword ptr [eax + esi + 0x3c]
0x54d2f4: add eax, esi
0x54d2f6: cmp ecx, -1
0x54d2f9: mov dword ptr [esp + 0x1c], eax
0x54d2fd: je 0x54d3f1
0x54d303: mov edx, dword ptr [0x87bc14]
0x54d309: and ecx, 0xffff
0x54d30f: shl ecx, 5
0x54d312: mov ecx, dword ptr [ecx + edx + 0x14]
0x54d316: cmp word ptr [ecx + 4], 0x20
0x54d31b: mov dword ptr [esp + 0x18], ecx
0x54d31f: jne 0x54d7f9
0x54d325: cmp dword ptr [ecx + 0x98], ebx
0x54d32b: mov dword ptr [esp + 0x14], ebx
0x54d32f: jle 0x54d3ed
0x54d335: mov dword ptr [esp + 0x28], ebx
0x54d339: lea esp, [esp]
0x54d340: mov ebp, dword ptr [ecx + 0x9c]
0x54d346: mov edx, dword ptr [esp + 0x28]
0x54d34a: mov eax, dword ptr [ebp + edx + 0x3c]
0x54d34e: add ebp, edx
0x54d350: cmp eax, ebx
0x54d352: mov dword ptr [esp + 0x10], ebx
0x54d356: jle 0x54d3cb
0x54d358: mov dword ptr [esp + 0x24], ebx
0x54d35c: lea esp, [esp]
0x54d360: mov esi, dword ptr [ebp + 0x40]
0x54d363: mov edx, dword ptr [esp + 0x24]
0x54d367: mov ebx, dword ptr [esi + edx + 0x2c]
0x54d36b: add esi, edx
0x54d36d: cmp ebx, -1
0x54d370: je 0x54d3ae
0x54d372: mov edx, dword ptr [0x6ac528]
0x54d378: mov edx, dword ptr [edx + 0x34]
0x54d37b: mov eax, ebx
0x54d37d: and eax, 0xffff
0x54d382: shl eax, 4
0x54d385: cmp byte ptr [eax + edx + 5], 0
0x54d38a: seta al
0x54d38d: test al, al
0x54d38f: jne 0x54d3ae
0x54d391: mov edi, dword ptr [0x6ac530]
0x54d397: call 0x4d1c20
0x54d39c: mov ecx, dword ptr [esp + 0x18]
0x54d3a0: mov dword ptr [esi + 0x2c], 0xffffffff
0x54d3a7: mov dword ptr [esi + 0x30], 0
0x54d3ae: mov eax, dword ptr [esp + 0x10]
0x54d3b2: mov esi, dword ptr [esp + 0x24]
0x54d3b6: mov edx, dword ptr [ebp + 0x3c]
0x54d3b9: inc eax
0x54d3ba: add esi, 0x7c
0x54d3bd: cmp eax, edx
0x54d3bf: mov dword ptr [esp + 0x10], eax
0x54d3c3: mov dword ptr [esp + 0x24], esi
0x54d3c7: jl 0x54d360
0x54d3c9: xor ebx, ebx
0x54d3cb: mov eax, dword ptr [esp + 0x14]
0x54d3cf: mov esi, dword ptr [esp + 0x28]
0x54d3d3: mov edx, dword ptr [ecx + 0x98]
0x54d3d9: inc eax
0x54d3da: add esi, 0x48
0x54d3dd: cmp eax, edx
0x54d3df: mov dword ptr [esp + 0x14], eax
0x54d3e3: mov dword ptr [esp + 0x28], esi
0x54d3e7: jl 0x54d340
0x54d3ed: mov eax, dword ptr [esp + 0x1c]
0x54d3f1: mov ecx, dword ptr [eax + 0x5c]
0x54d3f4: cmp ecx, -1
0x54d3f7: je 0x54d4e1
0x54d3fd: mov eax, dword ptr [0x87bc14]
0x54d402: and ecx, 0xffff
0x54d408: shl ecx, 5
0x54d40b: mov ecx, dword ptr [ecx + eax + 0x14]
0x54d40f: cmp word ptr [ecx + 4], 0x20
0x54d414: mov dword ptr [esp + 0x18], ecx
0x54d418: jne 0x54d7f9
0x54d41e: cmp dword ptr [ecx + 0x98], ebx
0x54d424: mov dword ptr [esp + 0x14], ebx
0x54d428: jle 0x54d4dd
0x54d42e: mov dword ptr [esp + 0x24], ebx
0x54d432: mov ebp, dword ptr [ecx + 0x9c]
0x54d438: mov edx, dword ptr [esp + 0x24]
0x54d43c: mov eax, dword ptr [ebp + edx + 0x3c]
0x54d440: add ebp, edx
0x54d442: cmp eax, ebx
0x54d444: mov dword ptr [esp + 0x10], ebx
0x54d448: jle 0x54d4bb
0x54d44a: mov dword ptr [esp + 0x28], ebx
0x54d44e: mov edi, edi
0x54d450: mov esi, dword ptr [ebp + 0x40]
0x54d453: mov edx, dword ptr [esp + 0x28]
0x54d457: mov ebx, dword ptr [esi + edx + 0x2c]
0x54d45b: add esi, edx
0x54d45d: cmp ebx, -1
0x54d460: je 0x54d49e
0x54d462: mov eax, dword ptr [0x6ac528]
0x54d467: mov eax, dword ptr [eax + 0x34]
0x54d46a: mov edx, ebx
0x54d46c: and edx, 0xffff
0x54d472: shl edx, 4
0x54d475: cmp byte ptr [edx + eax + 5], 0
0x54d47a: seta al
0x54d47d: test al, al
0x54d47f: jne 0x54d49e
0x54d481: mov edi, dword ptr [0x6ac530]
0x54d487: call 0x4d1c20
0x54d48c: mov ecx, dword ptr [esp + 0x18]
0x54d490: mov dword ptr [esi + 0x2c], 0xffffffff
0x54d497: mov dword ptr [esi + 0x30], 0
0x54d49e: mov eax, dword ptr [esp + 0x10]
0x54d4a2: mov esi, dword ptr [esp + 0x28]
0x54d4a6: mov edx, dword ptr [ebp + 0x3c]
0x54d4a9: inc eax
0x54d4aa: add esi, 0x7c
0x54d4ad: cmp eax, edx
0x54d4af: mov dword ptr [esp + 0x10], eax
0x54d4b3: mov dword ptr [esp + 0x28], esi
0x54d4b7: jl 0x54d450
0x54d4b9: xor ebx, ebx
0x54d4bb: mov eax, dword ptr [esp + 0x14]
0x54d4bf: mov esi, dword ptr [esp + 0x24]
0x54d4c3: mov edx, dword ptr [ecx + 0x98]
0x54d4c9: inc eax
0x54d4ca: add esi, 0x48
0x54d4cd: cmp eax, edx
0x54d4cf: mov dword ptr [esp + 0x14], eax
0x54d4d3: mov dword ptr [esp + 0x24], esi
0x54d4d7: jl 0x54d432
0x54d4dd: mov eax, dword ptr [esp + 0x1c]
0x54d4e1: mov ecx, dword ptr [eax + 0x9c]
0x54d4e7: cmp ecx, -1
0x54d4ea: je 0x54d5e2
0x54d4f0: mov edx, dword ptr [0x87bc14]
0x54d4f6: and ecx, 0xffff
0x54d4fc: shl ecx, 5
0x54d4ff: mov ecx, dword ptr [ecx + edx + 0x14]
0x54d503: cmp word ptr [ecx + 4], 0x20
0x54d508: mov dword ptr [esp + 0x18], ecx
0x54d50c: jne 0x54d7f9
0x54d512: mov eax, ecx
0x54d514: cmp dword ptr [eax + 0x98], ebx
0x54d51a: mov dword ptr [esp + 0x14], ebx
0x54d51e: jle 0x54d5de
0x54d524: mov dword ptr [esp + 0x24], ebx
0x54d528: jmp 0x54d530
0x54d52a: lea ebx, [ebx]
0x54d530: mov ecx, dword ptr [esp + 0x18]
0x54d534: mov ebp, dword ptr [ecx + 0x9c]
0x54d53a: mov ecx, dword ptr [esp + 0x24]
0x54d53e: mov eax, dword ptr [ebp + ecx + 0x3c]
0x54d542: add ebp, ecx
0x54d544: cmp eax, ebx
0x54d546: mov dword ptr [esp + 0x10], ebx
0x54d54a: jle 0x54d5b8
0x54d54c: mov dword ptr [esp + 0x28], ebx
0x54d550: mov esi, dword ptr [ebp + 0x40]
0x54d553: mov ecx, dword ptr [esp + 0x28]
0x54d557: mov ebx, dword ptr [esi + ecx + 0x2c]
0x54d55b: add esi, ecx
0x54d55d: cmp ebx, -1
0x54d560: je 0x54d59b
0x54d562: mov eax, dword ptr [0x6ac528]
0x54d567: mov ecx, dword ptr [eax + 0x34]
0x54d56a: mov edx, ebx
0x54d56c: and edx, 0xffff
0x54d572: shl edx, 4
0x54d575: mov al, byte ptr [edx + ecx + 5]
0x54d579: test al, al
0x54d57b: seta al
0x54d57e: test al, al
0x54d580: jne 0x54d59b
0x54d582: mov edi, dword ptr [0x6ac530]
0x54d588: call 0x4d1c20
0x54d58d: mov dword ptr [esi + 0x2c], 0xffffffff
0x54d594: mov dword ptr [esi + 0x30], 0
0x54d59b: mov eax, dword ptr [esp + 0x10]
0x54d59f: mov edx, dword ptr [esp + 0x28]
0x54d5a3: mov ecx, dword ptr [ebp + 0x3c]
0x54d5a6: inc eax
0x54d5a7: add edx, 0x7c
0x54d5aa: cmp eax, ecx
0x54d5ac: mov dword ptr [esp + 0x10], eax
0x54d5b0: mov dword ptr [esp + 0x28], edx
0x54d5b4: jl 0x54d550
0x54d5b6: xor ebx, ebx
0x54d5b8: mov edx, dword ptr [esp + 0x24]
0x54d5bc: mov eax, dword ptr [esp + 0x14]
0x54d5c0: add edx, 0x48
0x54d5c3: mov dword ptr [esp + 0x24], edx
0x54d5c7: mov edx, dword ptr [esp + 0x18]
0x54d5cb: mov ecx, dword ptr [edx + 0x98]
0x54d5d1: inc eax
0x54d5d2: cmp eax, ecx
0x54d5d4: mov dword ptr [esp + 0x14], eax
0x54d5d8: jl 0x54d530
0x54d5de: mov eax, dword ptr [esp + 0x1c]
0x54d5e2: mov eax, dword ptr [eax + 0x4c]
0x54d5e5: cmp eax, -1
0x54d5e8: je 0x54d6df
0x54d5ee: mov ecx, dword ptr [0x87bc14]
0x54d5f4: and eax, 0xffff
0x54d5f9: shl eax, 5
0x54d5fc: mov eax, dword ptr [eax + ecx + 0x14]
0x54d600: cmp word ptr [eax + 4], 0x20
0x54d605: mov dword ptr [esp + 0x18], eax
0x54d609: jne 0x54d7f9
0x54d60f: mov edx, eax
0x54d611: cmp dword ptr [edx + 0x98], ebx
0x54d617: mov dword ptr [esp + 0x14], ebx
0x54d61b: jle 0x54d6df
0x54d621: mov dword ptr [esp + 0x24], ebx
0x54d625: jmp 0x54d630
0x54d627: lea esp, [esp]
0x54d62e: mov edi, edi
0x54d630: mov eax, dword ptr [esp + 0x18]
0x54d634: mov ebp, dword ptr [eax + 0x9c]
0x54d63a: mov ecx, dword ptr [esp + 0x24]
0x54d63e: mov eax, dword ptr [ebp + ecx + 0x3c]
0x54d642: add ebp, ecx
0x54d644: cmp eax, ebx
0x54d646: mov dword ptr [esp + 0x10], ebx
0x54d64a: jle 0x54d6b9
0x54d64c: mov dword ptr [esp + 0x28], ebx
0x54d650: mov esi, dword ptr [ebp + 0x40]
0x54d653: mov ecx, dword ptr [esp + 0x28]
0x54d657: mov ebx, dword ptr [esi + ecx + 0x2c]
0x54d65b: add esi, ecx
0x54d65d: cmp ebx, -1
0x54d660: je 0x54d69c
0x54d662: mov edx, dword ptr [0x6ac528]
0x54d668: mov eax, dword ptr [edx + 0x34]
0x54d66b: mov ecx, ebx
0x54d66d: and ecx, 0xffff
0x54d673: shl ecx, 4
0x54d676: mov dl, byte ptr [ecx + eax + 5]
0x54d67a: test dl, dl
0x54d67c: seta al
0x54d67f: test al, al
0x54d681: jne 0x54d69c
0x54d683: mov edi, dword ptr [0x6ac530]
0x54d689: call 0x4d1c20
0x54d68e: mov dword ptr [esi + 0x2c], 0xffffffff
0x54d695: mov dword ptr [esi + 0x30], 0
0x54d69c: mov eax, dword ptr [esp + 0x10]
0x54d6a0: mov edx, dword ptr [esp + 0x28]
0x54d6a4: mov ecx, dword ptr [ebp + 0x3c]
0x54d6a7: inc eax
0x54d6a8: add edx, 0x7c
0x54d6ab: cmp eax, ecx
0x54d6ad: mov dword ptr [esp + 0x10], eax
0x54d6b1: mov dword ptr [esp + 0x28], edx
0x54d6b5: jl 0x54d650
0x54d6b7: xor ebx, ebx
0x54d6b9: mov eax, dword ptr [esp + 0x14]
0x54d6bd: mov esi, dword ptr [esp + 0x24]
0x54d6c1: mov ecx, dword ptr [esp + 0x18]
0x54d6c5: mov edx, dword ptr [ecx + 0x98]
0x54d6cb: inc eax
0x54d6cc: add esi, 0x48
0x54d6cf: cmp eax, edx
0x54d6d1: mov dword ptr [esp + 0x14], eax
0x54d6d5: mov dword ptr [esp + 0x24], esi
0x54d6d9: jl 0x54d630
0x54d6df: mov edx, dword ptr [esp + 0x1c]
0x54d6e3: mov eax, dword ptr [edx + 0x8c]
0x54d6e9: cmp eax, -1
0x54d6ec: je 0x54d7de
0x54d6f2: mov ecx, dword ptr [0x87bc14]
0x54d6f8: and eax, 0xffff
0x54d6fd: shl eax, 5
0x54d700: mov eax, dword ptr [eax + ecx + 0x14]
0x54d704: cmp word ptr [eax + 4], 0x20
0x54d709: mov dword ptr [esp + 0x18], eax
0x54d70d: jne 0x54d7f9
0x54d713: cmp dword ptr [eax + 0x98], ebx
0x54d719: mov dword ptr [esp + 0x14], ebx
0x54d71d: jle 0x54d7de
0x54d723: mov dword ptr [esp + 0x24], ebx
0x54d727: jmp 0x54d730
0x54d729: lea esp, [esp]
0x54d730: mov edx, dword ptr [esp + 0x18]
0x54d734: mov ebp, dword ptr [edx + 0x9c]
0x54d73a: mov ecx, dword ptr [esp + 0x24]
0x54d73e: mov eax, dword ptr [ebp + ecx + 0x3c]
0x54d742: add ebp, ecx
0x54d744: cmp eax, ebx
0x54d746: mov dword ptr [esp + 0x10], ebx
0x54d74a: jle 0x54d7b8
0x54d74c: mov dword ptr [esp + 0x28], ebx
0x54d750: mov esi, dword ptr [ebp + 0x40]
0x54d753: mov ecx, dword ptr [esp + 0x28]
0x54d757: mov ebx, dword ptr [esi + ecx + 0x2c]
0x54d75b: add esi, ecx
0x54d75d: cmp ebx, -1
0x54d760: je 0x54d79b
0x54d762: mov ecx, dword ptr [0x6ac528]
0x54d768: mov edx, dword ptr [ecx + 0x34]
0x54d76b: mov eax, ebx
0x54d76d: and eax, 0xffff
0x54d772: shl eax, 4
0x54d775: mov cl, byte ptr [eax + edx + 5]
0x54d779: test cl, cl
0x54d77b: seta al
0x54d77e: test al, al
0x54d780: jne 0x54d79b
0x54d782: mov edi, dword ptr [0x6ac530]
0x54d788: call 0x4d1c20
0x54d78d: mov dword ptr [esi + 0x2c], 0xffffffff
0x54d794: mov dword ptr [esi + 0x30], 0
0x54d79b: mov eax, dword ptr [esp + 0x10]
0x54d79f: mov edx, dword ptr [esp + 0x28]
0x54d7a3: mov ecx, dword ptr [ebp + 0x3c]
0x54d7a6: inc eax
0x54d7a7: add edx, 0x7c
0x54d7aa: cmp eax, ecx
0x54d7ac: mov dword ptr [esp + 0x10], eax
0x54d7b0: mov dword ptr [esp + 0x28], edx
0x54d7b4: jl 0x54d750
0x54d7b6: xor ebx, ebx
0x54d7b8: mov eax, dword ptr [esp + 0x14]
0x54d7bc: mov esi, dword ptr [esp + 0x24]
0x54d7c0: mov ecx, dword ptr [esp + 0x18]
0x54d7c4: mov edx, dword ptr [ecx + 0x98]
0x54d7ca: inc eax
0x54d7cb: add esi, 0x48
0x54d7ce: cmp eax, edx
0x54d7d0: mov dword ptr [esp + 0x14], eax
0x54d7d4: mov dword ptr [esp + 0x24], esi
0x54d7d8: jl 0x54d730
0x54d7de: mov eax, dword ptr [esp + 0x2c]
0x54d7e2: mov edx, dword ptr [esp + 0x30]
0x54d7e6: mov ecx, dword ptr [edx + 0x3c]
0x54d7e9: inc eax
0x54d7ea: mov dword ptr [esp + 0x2c], eax
0x54d7ee: movsx eax, ax
0x54d7f1: cmp eax, ecx
0x54d7f3: jl 0x54d2e3
0x54d7f9: mov edx, dword ptr [esp + 0x20]
0x54d7fd: mov eax, dword ptr [0x724a50]
0x54d802: call 0x4d0510
0x54d807: mov eax, dword ptr [esp + 0x20]
0x54d80b: mov edi, dword ptr [0x724a50]
0x54d811: jmp 0x54d98f
0x54d816: cmp word ptr [ebx + 0x52], 2
0x54d81b: je 0x54d98d
0x54d821: mov ecx, dword ptr [ebp + 0x48]
0x54d824: test ecx, ecx
0x54d826: mov dword ptr [esp + 0x30], 0
0x54d82e: jle 0x54d98d
0x54d834: xor esi, esi
0x54d836: mov dword ptr [esp + 0x2c], esi
0x54d83a: lea ebx, [ebx]
0x54d840: mov edx, dword ptr [ebp + 0x4c]
0x54d843: mov eax, dword ptr [ebx + esi*4 + 0x54]
0x54d847: mov ecx, dword ptr [0x72520c]
0x54d84d: mov edi, esi
0x54d84f: imul edi, edi, 0x68
0x54d852: add edi, edx
0x54d854: cmp eax, ecx
0x54d856: jge 0x54d96a
0x54d85c: mov eax, dword ptr [edi + 0xc]
0x54d85f: cmp eax, -1
0x54d862: je 0x54d96a
0x54d868: mov ecx, dword ptr [0x87bc14]
0x54d86e: and eax, 0xffff
0x54d873: shl eax, 5
0x54d876: mov edx, dword ptr [eax + ecx + 0x14]
0x54d87a: mov eax, dword ptr [ebx + 0x10]
0x54d87d: mov dword ptr [esp + 0x34], eax
0x54d881: mov eax, dword ptr [edi + 0x1c]
0x54d884: test al, 1
0x54d886: mov dword ptr [esp + 0x28], edx
0x54d88a: je 0x54d893
0x54d88c: mov cl, byte ptr [ebx + 0x4d]
0x54d88f: test cl, cl
0x54d891: jne 0x54d8fd
0x54d893: test al, 2
0x54d895: je 0x54d89e
0x54d897: mov al, byte ptr [ebx + 0x4d]
0x54d89a: test al, al
0x54d89c: je 0x54d8fd
0x54d89e: mov edx, dword ptr [edi + 0x18]
0x54d8a1: mov eax, dword ptr [esp + 0x34]
0x54d8a5: xor ecx, ecx
0x54d8a7: cmp word ptr [ebx + 0xc], cx
0x54d8ab: mov dword ptr [esp + 0x4c], edx
0x54d8af: sete cl
0x54d8b2: lea esi, [esp + 0x38]
0x54d8b6: mov edx, edi
0x54d8b8: mov dword ptr [esp + 0x48], eax
0x54d8bc: inc ecx
0x54d8bd: mov word ptr [esp + 0x44], cx
0x54d8c2: call 0x54e4d0
0x54d8c7: lea ecx, [esp + 0x44]
0x54d8cb: push ecx
0x54d8cc: mov edx, esi
0x54d8ce: mov esi, dword ptr [esp + 0x24]
0x54d8d2: push edx
0x54d8d3: push esi
0x54d8d4: call 0x54dc70
0x54d8d9: mov edx, dword ptr [edi + 0xc]
0x54d8dc: push 0
0x54d8de: push 0xc
0x54d8e0: lea eax, [esp + 0x4c]
0x54d8e4: push eax
0x54d8e5: push 0x54dc70
0x54d8ea: push esi
0x54d8eb: lea ecx, [esp + 0x64]
0x54d8ef: push ecx
0x54d8f0: push edx
0x54d8f1: call 0x549af0
0x54d8f6: mov esi, dword ptr [esp + 0x54]
0x54d8fa: add esp, 0x28
0x54d8fd: mov eax, dword ptr [0x719cd4]
0x54d902: fld dword ptr [ebp + 0x10]
0x54d905: fld dword ptr [ebp + 4]
0x54d908: imul eax, eax, 0x19660d
0x54d90e: fld dword ptr [edi + 0x14]
0x54d911: fld dword ptr [edi + 0x10]
0x54d914: fxch st(1)
0x54d916: fsub st(1)
0x54d918: add eax, 0x3c6ef35f
0x54d91d: mov ecx, eax
0x54d91f: shr ecx, 0x10
0x54d922: mov dword ptr [esp + 0x2c], ecx
0x54d926: mov edx, dword ptr [esp + 0x28]
0x54d92a: fild dword ptr [esp + 0x2c]
0x54d92e: mov dword ptr [0x719cd4], eax
0x54d933: fmul dword ptr [0x672b84]
0x54d939: fmulp st(1)
0x54d93b: fadd st(1)
0x54d93d: fxch st(3)
0x54d93f: fsub st(2)
0x54d941: fmul dword ptr [esp + 0x34]
0x54d945: fadd st(2)
0x54d947: fmulp st(3)
0x54d949: fxch st(2)
0x54d94b: fmul dword ptr [0x672ae8]
0x54d951: fiadd dword ptr [edx + 0x84]
0x54d957: fiadd dword ptr [0x72520c]
0x54d95d: call 0x6391b4
0x54d962: fstp st(1)
0x54d964: fstp st(0)
0x54d966: mov dword ptr [ebx + esi*4 + 0x54], eax
0x54d96a: mov eax, dword ptr [esp + 0x30]
0x54d96e: inc eax
0x54d96f: movsx esi, ax
0x54d972: mov dword ptr [esp + 0x30], eax
0x54d976: cmp esi, dword ptr [ebp + 0x48]
0x54d979: mov dword ptr [esp + 0x2c], esi
0x54d97d: jl 0x54d840
0x54d983: mov eax, dword ptr [esp + 0x20]
0x54d987: mov edi, dword ptr [0x724a50]
0x54d98d: xor ebx, ebx
0x54d98f: lea ecx, [eax + 1]
0x54d992: or esi, 0xffffffff
0x54d995: cmp cx, bx
0x54d998: jl 0x54d9cd
0x54d99a: mov bx, word ptr [edi + 0x2e]
0x54d99e: cmp cx, bx
0x54d9a1: jge 0x54d9cd
0x54d9a3: movsx edx, word ptr [edi + 0x22]
0x54d9a7: mov ebp, dword ptr [edi + 0x34]
0x54d9aa: movsx eax, cx
0x54d9ad: imul eax, edx
0x54d9b0: add eax, ebp
0x54d9b2: cmp word ptr [eax], 0
0x54d9b6: jne 0x54d9c2
0x54d9b8: inc ecx
0x54d9b9: add eax, edx
0x54d9bb: cmp cx, bx
0x54d9be: jl 0x54d9b2
0x54d9c0: jmp 0x54d9cd
0x54d9c2: movsx esi, word ptr [eax]
0x54d9c5: movsx eax, cx
0x54d9c8: shl esi, 0x10
0x54d9cb: or esi, eax
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
