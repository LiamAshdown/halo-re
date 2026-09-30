// sound_cache_decode_permutation  (Ghidra: FUN_00443d60; renamed from the cache_functions.md
// summary "Decodes a compressed sound sample into a cached destination buffer, resizing the
// shared scratch buffer as needed")
// address 0x443d60, size 158 bytes
// name confidence: 0.4   rewrite confidence: 0.45
// evidence: out/phase4/cache_types_notes.md's cache_io_completion section: the 15-byte thunk at
// 0x00443e00 walks completion->data (a sound_cache_entry*) to entry->permutation and tail-jumps
// here, which is why the register-passed pointer is a SoundPermutation* landing in EAX; field
// offsets 0x28 (format), 0x30 (_pad_30), 0x38 (buffer_size), 0x40 (samples) match
// types/tags.h SoundPermutation exactly. The decode target buffer is DAT_006f17ec /
// sound_decode_buffer, named in types/cache.h's globals list.
// register convention: SoundPermutation pointer in EAX (in_EAX).
//
// UNSURE: sound_decode_dispatch (0x0054e830) is far outside this module's address range (it belongs to
// the sound decode / math module) and is not rewritten here. Its exact contract is not
// established by this module; it is called with exactly the two arguments Ghidra shows
// (the value stored at permutation->_pad_30, and the address of permutation->samples), and its
// return value gates the copy from sound_decode_buffer into that same _pad_30 pointer. Whether
// the first argument is consumed as a decode destination, a decoder context, or something else
// is not determined here.

#include "win32.h"
#include "tags.h"
#include "cache.h"
#include "fn_sound.h"
#include "fn_cache.h"

extern int32_t sound_decode_buffer_size; // 0x006f17f0
extern void *sound_decode_buffer;        // 0x006f17ec


// UNSURE: see file header note above. The second argument is a plain dereference of
// permutation->samples's first field (structurally TagDataOffset::size), not the struct's
// address -- Ghidra shows `*(undefined4 *)(in_EAX + 0x40)`, a value read, not a pointer.
extern tag_instance *tag_instances; // 0x0087bc14


// blam-cc: SoundPermutation pointer in EAX (in_EAX)
// If the permutation's format is 1 (xbox_adpcm is the one format the cache_io_completion
// procedure thunk skips this decoder for -- see types/cache.h cache_io_completion), grows
// sound_decode_buffer to at least buffer_size, decodes the permutation's compressed samples
// into it via sound_decode_dispatch, and on success copies buffer_size bytes from the scratch buffer into
// the resident page pointer stored at permutation->_pad_30.
void sound_cache_decode_permutation(SoundPermutation *permutation)
{
    uint32_t buffer_size;
    void *decode_context;
    uint8_t *source;
    uint8_t *destination;
    uint32_t words;
    uint32_t tail_bytes;

    if (permutation->format == 1) {
        buffer_size = permutation->buffer_size;

        if (sound_decode_buffer_size < (int32_t)buffer_size) {
            if (sound_decode_buffer != (void *)0) {
                GlobalFree(sound_decode_buffer);
            }
            sound_decode_buffer_size = buffer_size;
            sound_decode_buffer = GlobalAlloc(0, buffer_size);
        }

        // 0x443d6e..0x443dca: ECX = channel count (1, or 2 when the owning sound tag's +0x6c is 1), EBX = the
        // scratch buffer, stack = the cached compressed samples (+0x30) and their size (samples.size, +0x40).
        // FIXED: the draft passed only the two stack values.
        {
            Sound *sound_tag = (Sound *)tag_instances[*(datum_index *)&permutation->tag_id_1 & 0xffff].data;
            int16_t channel_count = (int16_t)(1 + (sound_tag->channel_count == 1));
            decode_context = permutation->cache_page;
            if (sound_decode_dispatch(channel_count, sound_decode_buffer, decode_context,
                                      (int32_t)permutation->samples.size) != 0) {
                return;
            }
        }
        {
            source = (uint8_t *)sound_decode_buffer;
            destination = (uint8_t *)permutation->cache_page;

            for (words = buffer_size >> 2; words != 0; words--) {
                *(uint32_t *)destination = *(uint32_t *)source;
                source += 4;
                destination += 4;
            }
            for (tail_bytes = buffer_size & 3; tail_bytes != 0; tail_bytes--) {
                *destination = *source;
                source += 1;
                destination += 1;
            }
        }
    }
    return;
}

#if 0
Original Ghidra decompilation (0x443d60):

void FUN_00443d60(void)

{
  SIZE_T dwBytes;
  int in_EAX;
  int iVar1;
  uint uVar2;
  uint uVar3;
  undefined4 *puVar4;
  undefined4 *puVar5;

  if (*(short *)(in_EAX + 0x28) == 1) {
    dwBytes = *(SIZE_T *)(in_EAX + 0x38);
    if ((int)DAT_006f17f0 < (int)dwBytes) {
      if (DAT_006f17ec != (undefined4 *)0x0) {
        GlobalFree(DAT_006f17ec);
      }
      DAT_006f17f0 = dwBytes;
      DAT_006f17ec = GlobalAlloc(0,dwBytes);
    }
    iVar1 = FUN_0054e830(*(undefined4 *)(in_EAX + 0x30),*(undefined4 *)(in_EAX + 0x40));
    if (iVar1 == 0) {
      uVar3 = *(uint *)(in_EAX + 0x38);
      puVar4 = DAT_006f17ec;
      puVar5 = *(undefined4 **)(in_EAX + 0x30);
      for (uVar2 = uVar3 >> 2; uVar2 != 0; uVar2 = uVar2 - 1) {
        *puVar5 = *puVar4;
        puVar4 = puVar4 + 1;
        puVar5 = puVar5 + 1;
      }
      for (uVar3 = uVar3 & 3; uVar3 != 0; uVar3 = uVar3 - 1) {
        *(undefined1 *)puVar5 = *(undefined1 *)puVar4;
        puVar4 = (undefined4 *)((int)puVar4 + 1);
        puVar5 = (undefined4 *)((int)puVar5 + 1);
      }
    }
  }
  return;
}
#endif
