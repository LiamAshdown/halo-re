// sound_instance_stop  (Ghidra: sound_instance_stop, already named)
// address 0x54b180, size 2003 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md "Stops a single playing-sound datum, evicting its
// cached permutation data and deleting its datum entry."; fields match types/sound.h `sound`
// (channel_index 0x8c, flags 0x04, definition_index 0x08, owner_index 0x0c, pitch_range_index
// 0x8e, permutation_index 0x90, play_state 0x02, track_index 0x94) and types/tags.h Sound
// (sound_class 0x04, promotion fields, pitch_ranges 0x98) / SoundLooping (tracks 0x3c) /
// SoundLoopingTrack (start/loop/end/alternate_loop/alternate_end TagDependency, tag_id at +0xc
// of each). The five near-identical "walk this tag's pitch ranges/permutations, evict any
// unlocked cache page" blocks are factored into one static helper (sound_release_unused_pages,
// below); Ghidra printed them as five copies of the same code with different starting tag ids.
// register convention: sound handle as the recognized parameter (param_1).
// Phase-4 review: checked line by line against the disassembly appended below; control flow,
// the class sets {0x2c..0x2f}, the sweep order (start, end, alternate_end, loop,
// alternate_loop) and the register arguments (sound_channel_release_permutations: DI = channel;
// sound_permutation_release_page: ESI = permutation) all match. One deliberate difference: on
// the no-channel path the binary decrements entries[samples_pointer & 0xffff].lock_count without
// testing samples_pointer for -1 (it only tests the computed entry address for NULL); the
// rewrite keeps the -1 guard, since the unguarded form would touch entry 0xffff.
//
// Sound.scripting_sound (tags.h +0x94, a runtime TagID-shaped slot) holds the sound instance
// started by hs sound_impulse_start (0x543e10 writes it), so it is compared with the instance
// handle as a raw 32-bit value.

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "sound.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *sound_data;            // 0x007252c0, "sounds" 0x200 x 0xb0
extern tag_instance *tag_instances;       // 0x0087bc14
extern sound_channel sound_channels[k_maximum_sound_channels]; // 0x00724a60
extern data_array *sound_cache_entries;   // 0x006ac528
extern struct cache *sound_cache;         // 0x006ac530
extern data_array *looping_sound_data;    // 0x00724a50, "looping sounds" 0x80 x 0xe4

extern void sound_permutation_release_page(SoundPermutation *permutation); // 0x443d30, cache module
extern void cache_evict_entry(datum_index handle, struct cache *self);    // 0x4d1c20, memory module
extern void *datum_get(datum_index handle, data_array *array);            // 0x4d0680, memory module
extern void datum_delete(data_array *array, datum_index handle);          // 0x4d0510, memory module
extern void sound_channel_release_permutations(int16_t channel_index);    // 0x54d0d0

// Releases the page-cache reference of every resident, unlocked permutation across every pitch
// range of the sound tag `tag_id`. Used to reclaim memory once the last sound using a looping
// track's start/loop/end/alternate sound has finished.
static void sound_release_unused_pages(TagID tag_id)
{
    Sound *definition;
    int32_t range_index, permutation_index;
    SoundPitchRange *pitch_range;
    SoundPermutation *permutation;
    sound_cache_entry *entry;

    definition = (Sound *)tag_instances[tag_id.index].data;
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
}

// Stops a playing-sound datum: releases its own cached permutation reference (or its driver
// channel and the reference held by whichever channel was playing it), clears it from its
// owning looping_sound's track_sounds slot if it is a track sound, and deletes its datum entry.
// If this was the last sound of a finished music loop, also sweeps every track of the owning
// SoundLooping definition for now-unused cached pages.
void sound_instance_stop(datum_index sound_handle)
{
    sound *instance;
    Sound *definition;
    int16_t channel_index;
    SoundPitchRange *pitch_range;
    SoundPermutation *permutation;
    sound_cache_entry *entry;
    looping_sound *owner;
    uint8_t is_scripted_dialog_class;

    instance = (sound *)((uint8_t *)sound_data->data + (sound_handle & 0xffff) * sizeof(sound));
    channel_index = instance->channel_index;
    definition = (Sound *)tag_instances[instance->definition_index & 0xffff].data;

    if (channel_index == -1) {
        // Never made it to a channel: release the cache reference this instance was holding on
        // its own chosen permutation, if any.
        if (instance->flags & _sound_channel_requested_bit) {
            pitch_range = (SoundPitchRange *)definition->pitch_ranges.pointer + instance->pitch_range_index;
            permutation = (SoundPermutation *)pitch_range->permutations.pointer + instance->permutation_index;

            if (permutation->samples_pointer != 0xffffffff) {
                entry = (sound_cache_entry *)((uint8_t *)sound_cache_entries->data +
                    (permutation->samples_pointer & 0xffff) * sizeof(sound_cache_entry));
                entry->lock_count -= 1;
            }

            is_scripted_dialog_class = definition->sound_class == soundclass_scripted_dialog_player ||
                definition->sound_class == soundclass_scripted_effect ||
                definition->sound_class == soundclass_scripted_dialog_other ||
                definition->sound_class == soundclass_scripted_dialog_force_unspatialized;
            if (is_scripted_dialog_class) {
                if (permutation->samples_pointer != 0xffffffff) {
                    entry = (sound_cache_entry *)((uint8_t *)sound_cache_entries->data +
                        (permutation->samples_pointer & 0xffff) * sizeof(sound_cache_entry));
                    if (entry->lock_count == 0) {
                        sound_permutation_release_page(permutation);
                    }
                } else {
                    sound_permutation_release_page(permutation);
                }
            }
        }
    } else {
        // Was on a channel: free the channel and its cached permutations.
        sound_channels[channel_index].sound_index = 0xffffffff;
        sound_channel_release_permutations(channel_index);
        instance->channel_index = -1;

        if (*(uint32_t *)&definition->scripting_sound == sound_handle ||
            (definition->sound_class != soundclass_scripted_dialog_player &&
             definition->sound_class != soundclass_scripted_effect &&
             definition->sound_class != soundclass_scripted_dialog_other &&
             definition->sound_class != soundclass_scripted_dialog_force_unspatialized)) {
            // Last instance stopping a finished music loop: sweep every track of the owning
            // SoundLooping definition for cached pages nothing needs any more.
            if (instance->play_state == _sound_play_loop_end && definition->sound_class == soundclass_music) {
                owner = (looping_sound *)((uint8_t *)looping_sound_data->data +
                    (instance->owner_index & 0xffff) * sizeof(looping_sound));
                {
                    // Separate from `definition`: the original keeps the Sound tag pointer (iVar3)
                    // untouched here and walks the *SoundLooping* tag through its own local (iVar12).
                    SoundLooping *looping_definition =
                        (SoundLooping *)tag_instances[owner->definition_index & 0xffff].data;
                    int32_t track_index;
                    SoundLoopingTrack *track;

                    for (track_index = 0; track_index < looping_definition->tracks.count; track_index++) {
                        track = (SoundLoopingTrack *)looping_definition->tracks.pointer + track_index;
                        if (track->start.tag_id.index != 0xffff || track->start.tag_id.id != 0xffff) {
                            sound_release_unused_pages(track->start.tag_id);
                        }
                        if (track->end.tag_id.index != 0xffff || track->end.tag_id.id != 0xffff) {
                            sound_release_unused_pages(track->end.tag_id);
                        }
                        if (track->alternate_end.tag_id.index != 0xffff || track->alternate_end.tag_id.id != 0xffff) {
                            sound_release_unused_pages(track->alternate_end.tag_id);
                        }
                        if (track->loop.tag_id.index != 0xffff || track->loop.tag_id.id != 0xffff) {
                            sound_release_unused_pages(track->loop.tag_id);
                        }
                        if (track->alternate_loop.tag_id.index != 0xffff || track->alternate_loop.tag_id.id != 0xffff) {
                            sound_release_unused_pages(track->alternate_loop.tag_id);
                        }
                    }
                }
            }
        } else {
            // Not the tracked scripting instance and is a scripted-dialog class: release this
            // instance's own permutation reference directly.
            pitch_range = (SoundPitchRange *)definition->pitch_ranges.pointer + instance->pitch_range_index;
            permutation = (SoundPermutation *)pitch_range->permutations.pointer + instance->permutation_index;
            if (permutation->samples_pointer != 0xffffffff) {
                entry = (sound_cache_entry *)((uint8_t *)sound_cache_entries->data +
                    (permutation->samples_pointer & 0xffff) * sizeof(sound_cache_entry));
                if (entry->lock_count != 0) {
                    goto skip_release;
                }
            }
            sound_permutation_release_page(permutation);
        }
    }
skip_release:

    if (instance->play_state != 0 && instance->owner_index != 0xffffffff) {
        owner = (looping_sound *)datum_get(instance->owner_index, looping_sound_data);
        if (owner != 0) {
            owner->active_sound_count -= 1;
            if (owner->track_sounds[instance->track_index] == sound_handle) {
                owner->track_sounds[instance->track_index] = 0xffffffff;
            }
        }
    }

    if (*(uint32_t *)&definition->scripting_sound == sound_handle) {
        is_scripted_dialog_class = definition->sound_class == soundclass_scripted_dialog_player ||
            definition->sound_class == soundclass_scripted_effect ||
            definition->sound_class == soundclass_scripted_dialog_other ||
            definition->sound_class == soundclass_scripted_dialog_force_unspatialized;
        *(uint32_t *)&definition->scripting_sound = 0xffffffff;
        if (is_scripted_dialog_class) {
            pitch_range = (SoundPitchRange *)definition->pitch_ranges.pointer + instance->pitch_range_index;
            permutation = (SoundPermutation *)pitch_range->permutations.pointer + instance->permutation_index;
            if (permutation->samples_pointer != 0xffffffff) {
                entry = (sound_cache_entry *)((uint8_t *)sound_cache_entries->data +
                    (permutation->samples_pointer & 0xffff) * sizeof(sound_cache_entry));
                if (entry->lock_count != 0) {
                    goto delete_datum;
                }
                cache_evict_entry((datum_index)permutation->samples_pointer, sound_cache);
            }
            permutation->samples_pointer = 0xffffffff;
            permutation->cache_page = 0;
        }
    }
delete_datum:
    datum_delete(sound_data, sound_handle);
}

#if 0
Original Ghidra decompilation (0x54b180): see out/phase2/sound/01.md and
scratchpad/sound_packs/0x54b180.md for the full 2003-byte listing (too long to repeat here in
full); the five repeated pitch-range/permutation eviction blocks and the datum_get-shaped
owner-lookup tail were folded into sound_release_unused_pages() and datum_get() respectively, as
described in the file header.

Disassembly (0x54b180..0x54b953, capstone; phase-4 review):

0x54b180: sub esp, 0x28
0x54b183: mov eax, dword ptr [0x7252c0]
0x54b188: push ebx
0x54b189: mov edx, dword ptr [0x87bc14]
0x54b18f: push ebp
0x54b190: mov ebp, dword ptr [eax + 0x34]
0x54b193: push esi
0x54b194: mov esi, dword ptr [esp + 0x38]
0x54b198: mov ebx, esi
0x54b19a: and ebx, 0xffff
0x54b1a0: imul ebx, ebx, 0xb0
0x54b1a6: mov ecx, dword ptr [ebx + ebp + 8]
0x54b1aa: mov ax, word ptr [ebx + ebp + 0x8c]
0x54b1b2: add ebx, ebp
0x54b1b4: and ecx, 0xffff
0x54b1ba: shl ecx, 5
0x54b1bd: cmp ax, 0xffff
0x54b1c1: mov ebp, dword ptr [ecx + edx + 0x14]
0x54b1c5: push edi
0x54b1c6: mov dword ptr [esp + 0x34], ebx
0x54b1ca: mov dword ptr [esp + 0x30], ebp
0x54b1ce: je 0x54b7a6
0x54b1d4: movsx eax, ax
0x54b1d7: lea eax, [eax + eax*2]
0x54b1da: mov dword ptr [eax*8 + 0x724a60], 0xffffffff
0x54b1e5: mov di, word ptr [ebx + 0x8c]
0x54b1ec: call 0x54d0d0
0x54b1f1: mov word ptr [ebx + 0x8c], 0xffff
0x54b1fa: cmp dword ptr [ebp + 0x94], esi
0x54b200: je 0x54b267
0x54b202: mov ax, word ptr [ebp + 4]
0x54b206: cmp ax, 0x2c
0x54b20a: je 0x54b21e
0x54b20c: cmp ax, 0x2d
0x54b210: je 0x54b21e
0x54b212: cmp ax, 0x2e
0x54b216: je 0x54b21e
0x54b218: cmp ax, 0x2f
0x54b21c: jne 0x54b267
0x54b21e: movsx esi, word ptr [ebx + 0x90]
0x54b225: movsx eax, word ptr [ebx + 0x8e]
0x54b22c: imul esi, esi, 0x7c
0x54b22f: mov edx, dword ptr [ebp + 0x9c]
0x54b235: lea ecx, [eax + eax*8]
0x54b238: mov edi, dword ptr [edx + ecx*8 + 0x40]
0x54b23c: mov eax, dword ptr [esi + edi + 0x2c]
0x54b240: add esi, edi
0x54b242: cmp eax, -1
0x54b245: je 0x54b847
0x54b24b: mov ecx, dword ptr [0x6ac528]
0x54b251: mov edx, dword ptr [ecx + 0x34]
0x54b254: and eax, 0xffff
0x54b259: shl eax, 4
0x54b25c: mov cl, byte ptr [eax + edx + 5]
0x54b260: test cl, cl
0x54b262: jmp 0x54b840
0x54b267: cmp word ptr [ebx + 2], 4
0x54b26c: jne 0x54b84c
0x54b272: cmp word ptr [ebp + 4], 0x20
0x54b277: jne 0x54b84c
0x54b27d: mov eax, dword ptr [ebx + 0xc]
0x54b280: mov ecx, dword ptr [0x724a50]
0x54b286: mov edx, dword ptr [ecx + 0x34]
0x54b289: mov ecx, dword ptr [0x87bc14]
0x54b28f: and eax, 0xffff
0x54b294: imul eax, eax, 0xe4
0x54b29a: mov eax, dword ptr [eax + edx + 4]
0x54b29e: and eax, 0xffff
0x54b2a3: shl eax, 5
0x54b2a6: mov eax, dword ptr [eax + ecx + 0x14]
0x54b2aa: mov ecx, dword ptr [eax + 0x3c]
0x54b2ad: xor edi, edi
0x54b2af: cmp ecx, edi
0x54b2b1: mov dword ptr [esp + 0x2c], eax
0x54b2b5: mov dword ptr [esp + 0x28], edi
0x54b2b9: jle 0x54b84c
0x54b2bf: xor eax, eax
0x54b2c1: lea edx, [eax + eax*4]
0x54b2c4: mov eax, dword ptr [esp + 0x2c]
0x54b2c8: mov esi, dword ptr [eax + 0x40]
0x54b2cb: shl edx, 5
0x54b2ce: mov eax, dword ptr [edx + esi + 0x3c]
0x54b2d2: add edx, esi
0x54b2d4: cmp eax, -1
0x54b2d7: mov dword ptr [esp + 0x1c], edx
0x54b2db: je 0x54b3c0
0x54b2e1: mov ecx, dword ptr [0x87bc14]
0x54b2e7: and eax, 0xffff
0x54b2ec: shl eax, 5
0x54b2ef: mov ecx, dword ptr [eax + ecx + 0x14]
0x54b2f3: cmp dword ptr [ecx + 0x98], edi
0x54b2f9: mov dword ptr [esp + 0x18], ecx
0x54b2fd: mov dword ptr [esp + 0x14], edi
0x54b301: jle 0x54b3c0
0x54b307: mov dword ptr [esp + 0x24], edi
0x54b30b: jmp 0x54b310
0x54b30d: lea ecx, [ecx]
0x54b310: mov ebp, dword ptr [ecx + 0x9c]
0x54b316: mov esi, dword ptr [esp + 0x24]
0x54b31a: mov eax, dword ptr [ebp + esi + 0x3c]
0x54b31e: add ebp, esi
0x54b320: cmp eax, edi
0x54b322: mov dword ptr [esp + 0x10], edi
0x54b326: jle 0x54b39e
0x54b328: mov dword ptr [esp + 0x20], edi
0x54b32c: lea esp, [esp]
0x54b330: mov ebx, dword ptr [esp + 0x20]
0x54b334: mov esi, dword ptr [ebp + 0x40]
0x54b337: add esi, ebx
0x54b339: mov ebx, dword ptr [esi + 0x2c]
0x54b33c: cmp ebx, -1
0x54b33f: je 0x54b383
0x54b341: mov edi, dword ptr [0x6ac528]
0x54b347: mov edi, dword ptr [edi + 0x34]
0x54b34a: mov eax, ebx
0x54b34c: and eax, 0xffff
0x54b351: shl eax, 4
0x54b354: cmp byte ptr [eax + edi + 5], 0
0x54b359: seta al
0x54b35c: test al, al
0x54b35e: jne 0x54b381
0x54b360: mov edi, dword ptr [0x6ac530]
0x54b366: call 0x4d1c20
0x54b36b: mov edx, dword ptr [esp + 0x1c]
0x54b36f: mov ecx, dword ptr [esp + 0x18]
0x54b373: mov dword ptr [esi + 0x2c], 0xffffffff
0x54b37a: mov dword ptr [esi + 0x30], 0
0x54b381: xor edi, edi
0x54b383: mov eax, dword ptr [esp + 0x10]
0x54b387: mov ebx, dword ptr [esp + 0x20]
0x54b38b: mov esi, dword ptr [ebp + 0x3c]
0x54b38e: inc eax
0x54b38f: add ebx, 0x7c
0x54b392: cmp eax, esi
0x54b394: mov dword ptr [esp + 0x10], eax
0x54b398: mov dword ptr [esp + 0x20], ebx
0x54b39c: jl 0x54b330
0x54b39e: mov eax, dword ptr [esp + 0x14]
0x54b3a2: mov ebx, dword ptr [esp + 0x24]
0x54b3a6: mov esi, dword ptr [ecx + 0x98]
0x54b3ac: inc eax
0x54b3ad: add ebx, 0x48
0x54b3b0: cmp eax, esi
0x54b3b2: mov dword ptr [esp + 0x14], eax
0x54b3b6: mov dword ptr [esp + 0x24], ebx
0x54b3ba: jl 0x54b310
0x54b3c0: mov eax, dword ptr [edx + 0x5c]
0x54b3c3: cmp eax, -1
0x54b3c6: je 0x54b4ae
0x54b3cc: mov ecx, dword ptr [0x87bc14]
0x54b3d2: and eax, 0xffff
0x54b3d7: shl eax, 5
0x54b3da: mov eax, dword ptr [eax + ecx + 0x14]
0x54b3de: cmp dword ptr [eax + 0x98], edi
0x54b3e4: mov dword ptr [esp + 0x18], eax
0x54b3e8: mov dword ptr [esp + 0x14], edi
0x54b3ec: jle 0x54b4ae
0x54b3f2: mov dword ptr [esp + 0x20], edi
0x54b3f6: jmp 0x54b400
0x54b3f8: lea esp, [esp]
0x54b3ff: nop 
0x54b400: mov edx, dword ptr [esp + 0x18]
0x54b404: mov ebp, dword ptr [edx + 0x9c]
0x54b40a: mov ecx, dword ptr [esp + 0x20]
0x54b40e: mov eax, dword ptr [ebp + ecx + 0x3c]
0x54b412: add ebp, ecx
0x54b414: cmp eax, edi
0x54b416: mov dword ptr [esp + 0x10], edi
0x54b41a: jle 0x54b488
0x54b41c: mov dword ptr [esp + 0x24], edi
0x54b420: mov esi, dword ptr [ebp + 0x40]
0x54b423: mov ecx, dword ptr [esp + 0x24]
0x54b427: mov ebx, dword ptr [esi + ecx + 0x2c]
0x54b42b: add esi, ecx
0x54b42d: cmp ebx, -1
0x54b430: je 0x54b46d
0x54b432: mov ecx, dword ptr [0x6ac528]
0x54b438: mov edx, dword ptr [ecx + 0x34]
0x54b43b: mov eax, ebx
0x54b43d: and eax, 0xffff
0x54b442: shl eax, 4
0x54b445: mov cl, byte ptr [eax + edx + 5]
0x54b449: test cl, cl
0x54b44b: seta al
0x54b44e: test al, al
0x54b450: jne 0x54b46d
0x54b452: mov edi, dword ptr [0x6ac530]
0x54b458: call 0x4d1c20
0x54b45d: mov dword ptr [esi + 0x2c], 0xffffffff
0x54b464: mov dword ptr [esi + 0x30], 0
0x54b46b: xor edi, edi
0x54b46d: mov eax, dword ptr [esp + 0x10]
0x54b471: mov edx, dword ptr [esp + 0x24]
0x54b475: mov ecx, dword ptr [ebp + 0x3c]
0x54b478: inc eax
0x54b479: add edx, 0x7c
0x54b47c: cmp eax, ecx
0x54b47e: mov dword ptr [esp + 0x10], eax
0x54b482: mov dword ptr [esp + 0x24], edx
0x54b486: jl 0x54b420
0x54b488: mov eax, dword ptr [esp + 0x14]
0x54b48c: mov esi, dword ptr [esp + 0x20]
0x54b490: mov ecx, dword ptr [esp + 0x18]
0x54b494: mov edx, dword ptr [ecx + 0x98]
0x54b49a: inc eax
0x54b49b: add esi, 0x48
0x54b49e: cmp eax, edx
0x54b4a0: mov dword ptr [esp + 0x14], eax
0x54b4a4: mov dword ptr [esp + 0x20], esi
0x54b4a8: jl 0x54b400
0x54b4ae: mov edx, dword ptr [esp + 0x1c]
0x54b4b2: mov eax, dword ptr [edx + 0x9c]
0x54b4b8: cmp eax, -1
0x54b4bb: je 0x54b59e
0x54b4c1: mov ecx, dword ptr [0x87bc14]
0x54b4c7: and eax, 0xffff
0x54b4cc: shl eax, 5
0x54b4cf: mov eax, dword ptr [eax + ecx + 0x14]
0x54b4d3: cmp dword ptr [eax + 0x98], edi
0x54b4d9: mov dword ptr [esp + 0x18], eax
0x54b4dd: mov dword ptr [esp + 0x14], edi
0x54b4e1: jle 0x54b59e
0x54b4e7: mov dword ptr [esp + 0x20], edi
0x54b4eb: jmp 0x54b4f0
0x54b4ed: lea ecx, [ecx]
0x54b4f0: mov edx, dword ptr [esp + 0x18]
0x54b4f4: mov ebp, dword ptr [edx + 0x9c]
0x54b4fa: mov ecx, dword ptr [esp + 0x20]
0x54b4fe: mov eax, dword ptr [ebp + ecx + 0x3c]
0x54b502: add ebp, ecx
0x54b504: cmp eax, edi
0x54b506: mov dword ptr [esp + 0x10], edi
0x54b50a: jle 0x54b578
0x54b50c: mov dword ptr [esp + 0x24], edi
0x54b510: mov esi, dword ptr [ebp + 0x40]
0x54b513: mov ecx, dword ptr [esp + 0x24]
0x54b517: mov ebx, dword ptr [esi + ecx + 0x2c]
0x54b51b: add esi, ecx
0x54b51d: cmp ebx, -1
0x54b520: je 0x54b55d
0x54b522: mov ecx, dword ptr [0x6ac528]
0x54b528: mov edx, dword ptr [ecx + 0x34]
0x54b52b: mov eax, ebx
0x54b52d: and eax, 0xffff
0x54b532: shl eax, 4
0x54b535: mov cl, byte ptr [eax + edx + 5]
0x54b539: test cl, cl
0x54b53b: seta al
0x54b53e: test al, al
0x54b540: jne 0x54b55d
0x54b542: mov edi, dword ptr [0x6ac530]
0x54b548: call 0x4d1c20
0x54b54d: mov dword ptr [esi + 0x2c], 0xffffffff
0x54b554: mov dword ptr [esi + 0x30], 0
0x54b55b: xor edi, edi
0x54b55d: mov eax, dword ptr [esp + 0x10]
0x54b561: mov edx, dword ptr [esp + 0x24]
0x54b565: mov ecx, dword ptr [ebp + 0x3c]
0x54b568: inc eax
0x54b569: add edx, 0x7c
0x54b56c: cmp eax, ecx
0x54b56e: mov dword ptr [esp + 0x10], eax
0x54b572: mov dword ptr [esp + 0x24], edx
0x54b576: jl 0x54b510
0x54b578: mov eax, dword ptr [esp + 0x14]
0x54b57c: mov esi, dword ptr [esp + 0x20]
0x54b580: mov ecx, dword ptr [esp + 0x18]
0x54b584: mov edx, dword ptr [ecx + 0x98]
0x54b58a: inc eax
0x54b58b: add esi, 0x48
0x54b58e: cmp eax, edx
0x54b590: mov dword ptr [esp + 0x14], eax
0x54b594: mov dword ptr [esp + 0x20], esi
0x54b598: jl 0x54b4f0
0x54b59e: mov edx, dword ptr [esp + 0x1c]
0x54b5a2: mov eax, dword ptr [edx + 0x4c]
0x54b5a5: cmp eax, -1
0x54b5a8: je 0x54b68e
0x54b5ae: mov ecx, dword ptr [0x87bc14]
0x54b5b4: and eax, 0xffff
0x54b5b9: shl eax, 5
0x54b5bc: mov eax, dword ptr [eax + ecx + 0x14]
0x54b5c0: cmp dword ptr [eax + 0x98], edi
0x54b5c6: mov dword ptr [esp + 0x18], eax
0x54b5ca: mov dword ptr [esp + 0x14], edi
0x54b5ce: jle 0x54b68e
0x54b5d4: mov dword ptr [esp + 0x20], edi
0x54b5d8: jmp 0x54b5e0
0x54b5da: lea ebx, [ebx]
0x54b5e0: mov edx, dword ptr [esp + 0x18]
0x54b5e4: mov ebp, dword ptr [edx + 0x9c]
0x54b5ea: mov ecx, dword ptr [esp + 0x20]
0x54b5ee: mov eax, dword ptr [ebp + ecx + 0x3c]
0x54b5f2: add ebp, ecx
0x54b5f4: cmp eax, edi
0x54b5f6: mov dword ptr [esp + 0x10], edi
0x54b5fa: jle 0x54b668
0x54b5fc: mov dword ptr [esp + 0x24], edi
0x54b600: mov esi, dword ptr [ebp + 0x40]
0x54b603: mov ecx, dword ptr [esp + 0x24]
0x54b607: mov ebx, dword ptr [esi + ecx + 0x2c]
0x54b60b: add esi, ecx
0x54b60d: cmp ebx, -1
0x54b610: je 0x54b64d
0x54b612: mov ecx, dword ptr [0x6ac528]
0x54b618: mov edx, dword ptr [ecx + 0x34]
0x54b61b: mov eax, ebx
0x54b61d: and eax, 0xffff
0x54b622: shl eax, 4
0x54b625: mov cl, byte ptr [eax + edx + 5]
0x54b629: test cl, cl
0x54b62b: seta al
0x54b62e: test al, al
0x54b630: jne 0x54b64d
0x54b632: mov edi, dword ptr [0x6ac530]
0x54b638: call 0x4d1c20
0x54b63d: mov dword ptr [esi + 0x2c], 0xffffffff
0x54b644: mov dword ptr [esi + 0x30], 0
0x54b64b: xor edi, edi
0x54b64d: mov eax, dword ptr [esp + 0x10]
0x54b651: mov edx, dword ptr [esp + 0x24]
0x54b655: mov ecx, dword ptr [ebp + 0x3c]
0x54b658: inc eax
0x54b659: add edx, 0x7c
0x54b65c: cmp eax, ecx
0x54b65e: mov dword ptr [esp + 0x10], eax
0x54b662: mov dword ptr [esp + 0x24], edx
0x54b666: jl 0x54b600
0x54b668: mov eax, dword ptr [esp + 0x14]
0x54b66c: mov esi, dword ptr [esp + 0x20]
0x54b670: mov ecx, dword ptr [esp + 0x18]
0x54b674: mov edx, dword ptr [ecx + 0x98]
0x54b67a: inc eax
0x54b67b: add esi, 0x48
0x54b67e: cmp eax, edx
0x54b680: mov dword ptr [esp + 0x14], eax
0x54b684: mov dword ptr [esp + 0x20], esi
0x54b688: jl 0x54b5e0
0x54b68e: mov edx, dword ptr [esp + 0x1c]
0x54b692: mov eax, dword ptr [edx + 0x8c]
0x54b698: cmp eax, -1
0x54b69b: je 0x54b77e
0x54b6a1: mov ecx, dword ptr [0x87bc14]
0x54b6a7: and eax, 0xffff
0x54b6ac: shl eax, 5
0x54b6af: mov eax, dword ptr [eax + ecx + 0x14]
0x54b6b3: cmp dword ptr [eax + 0x98], edi
0x54b6b9: mov dword ptr [esp + 0x18], eax
0x54b6bd: mov dword ptr [esp + 0x14], edi
0x54b6c1: jle 0x54b77e
0x54b6c7: mov dword ptr [esp + 0x20], edi
0x54b6cb: jmp 0x54b6d0
0x54b6cd: lea ecx, [ecx]
0x54b6d0: mov edx, dword ptr [esp + 0x18]
0x54b6d4: mov ebp, dword ptr [edx + 0x9c]
0x54b6da: mov ecx, dword ptr [esp + 0x20]
0x54b6de: mov eax, dword ptr [ebp + ecx + 0x3c]
0x54b6e2: add ebp, ecx
0x54b6e4: cmp eax, edi
0x54b6e6: mov dword ptr [esp + 0x10], edi
0x54b6ea: jle 0x54b758
0x54b6ec: mov dword ptr [esp + 0x24], edi
0x54b6f0: mov esi, dword ptr [ebp + 0x40]
0x54b6f3: mov ecx, dword ptr [esp + 0x24]
0x54b6f7: mov ebx, dword ptr [esi + ecx + 0x2c]
0x54b6fb: add esi, ecx
0x54b6fd: cmp ebx, -1
0x54b700: je 0x54b73d
0x54b702: mov ecx, dword ptr [0x6ac528]
0x54b708: mov edx, dword ptr [ecx + 0x34]
0x54b70b: mov eax, ebx
0x54b70d: and eax, 0xffff
0x54b712: shl eax, 4
0x54b715: mov cl, byte ptr [eax + edx + 5]
0x54b719: test cl, cl
0x54b71b: seta al
0x54b71e: test al, al
0x54b720: jne 0x54b73d
0x54b722: mov edi, dword ptr [0x6ac530]
0x54b728: call 0x4d1c20
0x54b72d: mov dword ptr [esi + 0x2c], 0xffffffff
0x54b734: mov dword ptr [esi + 0x30], 0
0x54b73b: xor edi, edi
0x54b73d: mov eax, dword ptr [esp + 0x10]
0x54b741: mov edx, dword ptr [esp + 0x24]
0x54b745: mov ecx, dword ptr [ebp + 0x3c]
0x54b748: inc eax
0x54b749: add edx, 0x7c
0x54b74c: cmp eax, ecx
0x54b74e: mov dword ptr [esp + 0x10], eax
0x54b752: mov dword ptr [esp + 0x24], edx
0x54b756: jl 0x54b6f0
0x54b758: mov eax, dword ptr [esp + 0x14]
0x54b75c: mov esi, dword ptr [esp + 0x20]
0x54b760: mov ecx, dword ptr [esp + 0x18]
0x54b764: mov edx, dword ptr [ecx + 0x98]
0x54b76a: inc eax
0x54b76b: add esi, 0x48
0x54b76e: cmp eax, edx
0x54b770: mov dword ptr [esp + 0x14], eax
0x54b774: mov dword ptr [esp + 0x20], esi
0x54b778: jl 0x54b6d0
0x54b77e: mov eax, dword ptr [esp + 0x28]
0x54b782: mov edx, dword ptr [esp + 0x2c]
0x54b786: mov ecx, dword ptr [edx + 0x3c]
0x54b789: inc eax
0x54b78a: mov dword ptr [esp + 0x28], eax
0x54b78e: movsx eax, ax
0x54b791: cmp eax, ecx
0x54b793: jl 0x54b2c1
0x54b799: mov ebp, dword ptr [esp + 0x30]
0x54b79d: mov ebx, dword ptr [esp + 0x34]
0x54b7a1: jmp 0x54b84c
0x54b7a6: test byte ptr [ebx + 4], 2
0x54b7aa: je 0x54b84c
0x54b7b0: movsx esi, word ptr [ebx + 0x90]
0x54b7b7: movsx eax, word ptr [ebx + 0x8e]
0x54b7be: imul esi, esi, 0x7c
0x54b7c1: mov edx, dword ptr [ebp + 0x9c]
0x54b7c7: mov ecx, dword ptr [0x6ac528]
0x54b7cd: lea eax, [eax + eax*8]
0x54b7d0: mov eax, dword ptr [edx + eax*8 + 0x40]
0x54b7d4: mov eax, dword ptr [eax + esi + 0x2c]
0x54b7d8: mov edx, dword ptr [ecx + 0x34]
0x54b7db: and eax, 0xffff
0x54b7e0: shl eax, 4
0x54b7e3: add eax, edx
0x54b7e5: je 0x54b7ea
0x54b7e7: dec byte ptr [eax + 5]
0x54b7ea: mov ax, word ptr [ebp + 4]
0x54b7ee: cmp ax, 0x2c
0x54b7f2: je 0x54b806
0x54b7f4: cmp ax, 0x2d
0x54b7f8: je 0x54b806
0x54b7fa: cmp ax, 0x2e
0x54b7fe: je 0x54b806
0x54b800: cmp ax, 0x2f
0x54b804: jne 0x54b84c
0x54b806: movsx esi, word ptr [ebx + 0x90]
0x54b80d: movsx eax, word ptr [ebx + 0x8e]
0x54b814: imul esi, esi, 0x7c
0x54b817: lea edx, [eax + eax*8]
0x54b81a: mov eax, dword ptr [ebp + 0x9c]
0x54b820: mov edi, dword ptr [eax + edx*8 + 0x40]
0x54b824: mov eax, dword ptr [esi + edi + 0x2c]
0x54b828: add esi, edi
0x54b82a: cmp eax, -1
0x54b82d: je 0x54b847
0x54b82f: mov ecx, dword ptr [ecx + 0x34]
0x54b832: and eax, 0xffff
0x54b837: shl eax, 4
0x54b83a: mov dl, byte ptr [eax + ecx + 5]
0x54b83e: test dl, dl
0x54b840: seta al
0x54b843: test al, al
0x54b845: jne 0x54b84c
0x54b847: call 0x443d30
0x54b84c: cmp word ptr [ebx + 2], 0
0x54b851: je 0x54b8b8
0x54b853: mov ecx, dword ptr [ebx + 0xc]
0x54b856: cmp ecx, -1
0x54b859: je 0x54b8b8
0x54b85b: mov edx, ecx
0x54b85d: sar edx, 0x10
0x54b860: test cx, cx
0x54b863: jl 0x54b8b8
0x54b865: mov esi, dword ptr [0x724a50]
0x54b86b: cmp cx, word ptr [esi + 0x20]
0x54b86f: jge 0x54b8b8
0x54b871: movsx eax, word ptr [esi + 0x22]
0x54b875: mov edi, dword ptr [esi + 0x34]
0x54b878: movsx ecx, cx
0x54b87b: imul eax, ecx
0x54b87e: mov cx, word ptr [eax + edi]
0x54b882: add eax, edi
0x54b884: test cx, cx
0x54b887: je 0x54b8b8
0x54b889: test dx, dx
0x54b88c: je 0x54b893
0x54b88e: cmp cx, dx
0x54b891: jne 0x54b8b8
0x54b893: dec word ptr [eax + 0x50]
0x54b897: movsx edx, word ptr [ebx + 0x94]
0x54b89e: mov ecx, dword ptr [eax + edx*4 + 0xd4]
0x54b8a5: lea eax, [eax + edx*4 + 0xd4]
0x54b8ac: cmp ecx, dword ptr [esp + 0x3c]
0x54b8b0: jne 0x54b8b8
0x54b8b2: mov dword ptr [eax], 0xffffffff
0x54b8b8: mov edx, dword ptr [esp + 0x3c]
0x54b8bc: cmp dword ptr [ebp + 0x94], edx
0x54b8c2: jne 0x54b955
0x54b8c8: mov ax, word ptr [ebp + 4]
0x54b8cc: cmp ax, 0x2c
0x54b8d0: mov dword ptr [ebp + 0x94], 0xffffffff
0x54b8da: je 0x54b8ee
0x54b8dc: cmp ax, 0x2d
0x54b8e0: je 0x54b8ee
0x54b8e2: cmp ax, 0x2e
0x54b8e6: je 0x54b8ee
0x54b8e8: cmp ax, 0x2f
0x54b8ec: jne 0x54b955
0x54b8ee: movsx esi, word ptr [ebx + 0x90]
0x54b8f5: movsx eax, word ptr [ebx + 0x8e]
0x54b8fc: imul esi, esi, 0x7c
0x54b8ff: mov ecx, dword ptr [ebp + 0x9c]
0x54b905: lea eax, [eax + eax*8]
0x54b908: mov edx, dword ptr [ecx + eax*8 + 0x40]
0x54b90c: mov ebx, dword ptr [esi + edx + 0x2c]
0x54b910: add esi, edx
0x54b912: cmp ebx, -1
0x54b915: je 0x54b947
0x54b917: mov eax, dword ptr [0x6ac528]
0x54b91c: mov ecx, dword ptr [eax + 0x34]
0x54b91f: mov edx, ebx
0x54b921: and edx, 0xffff
0x54b927: shl edx, 4
0x54b92a: mov al, byte ptr [edx + ecx + 5]
0x54b92e: test al, al
0x54b930: seta al
0x54b933: test al, al
0x54b935: jne 0x54b955
0x54b937: cmp ebx, -1
0x54b93a: je 0x54b947
0x54b93c: mov edi, dword ptr [0x6ac530]
0x54b942: call 0x4d1c20
0x54b947: mov dword ptr [esi + 0x2c], 0xffffffff
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
