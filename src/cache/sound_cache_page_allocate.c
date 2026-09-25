// sound_cache_page_allocate  (Ghidra: FUN_004440e0; renamed, named directly in
// out/phase4/cache_types_notes.md's cache_io_completion section: "sound_cache_page_allocate
// @0x004441de..0x00444211" falls inside this function's own byte range)
// address 0x4440e0, size 344 bytes
// name confidence: 0.7   rewrite confidence: 0.55
// evidence: field offsets on SoundPermutation (format 0x28, tag_id_1 0x3c, buffer_size 0x38,
// samples 0x40) and on Sound (channel_count 0x6c, computed from types/tags.h field layout) all
// line up; the page-address computation (cache_entry::offset << cache::block_shift +
// sound_cache_base) matches types/memory.h and types/cache.h exactly; the cache_io_completion
// disassembly quoted in cache_types_notes.md (flag = &entry->loaded, entry = edi) is what fixes
// cache_io_request_new's extra, Ghidra-elided arguments.
// register convention: SoundPermutation pointer in EAX (in_EAX); priority is the recognized
// stack parameter (param_1).
//
// UNSURE: the ADPCM-style buffer_size formula for format == 1 is transcribed byte-for-byte from
// the decompile (renaming only the pointer dereferences to named fields) without being able to
// independently verify its intent; the nested casts and nested nested shifts are preserved
// exactly as Ghidra emitted them. Separately: the branchless completion-procedure selection
// (cache_io_sound_decode_thunk installed for every format except 1) means
// sound_cache_decode_permutation's own `format == 1` guard can never see a true condition when
// reached through that thunk -- an apparent inconsistency preserved as-is rather than resolved,
// since fixing it would be inventing behaviour. cache_io_request_new's completion
// record (flag/procedure/data) is built on this function's own stack and passed in ESI; Ghidra's
// decompile of this function elides it, but `lea esi,[esp+0x28]` at 0x00444209 (with the three
// dwords written at 0x004441e2, 0x004441ff and 0x0044420d) shows it directly.

// phase-4 review pass: body re-checked instruction by instruction against `objdump -d -M
// intel` of this address range; every field offset, branch and argument below now matches
// the machine code rather than only Ghidra's pseudo-C.
#include "tags.h"
#include "memory.h"
#include "cache.h"

extern tag_instance *tag_instances;      // 0x0087bc14
extern struct cache *sound_cache;        // 0x006ac530
extern data_array *sound_cache_entries;  // 0x006ac528
extern void *sound_cache_base;           // 0x006ac52c

extern datum_index cache_allocate_block(struct cache *self, uint32_t requested_bytes); // 0x4d1840
extern datum_index datum_new_at_index_with_salt(datum_index requested_handle, data_array *array); // 0x4d03d0
extern void sound_cache_dump_to_file(void); // this module, sound_cache_dump_to_file.c

extern int16_t cache_io_request_new(cache_io_completion *completion, // blam-cc: ESI
    int32_t offset, uint32_t size, void *destination, uint8_t priority,
    uint8_t data_file_index); // 0x442b20, this module

// The 15-byte thunk documented in out/phase4/cache_types_notes.md's cache_io_completion
// section; not rewritten here (Ghidra never gave it a function boundary). Recovers the
// sound_cache_entry from the completion record and tail-calls sound_cache_decode_permutation.
// the original 15-byte completion thunk; only its address is handed to the I/O system (never called from here), so
// it is referenced as that address rather than as an external function (which would route through a stub)
#define cache_io_sound_decode_thunk ((void (*)(cache_io_completion *))0x00443e00)

// blam-cc: permutation in EAX (in_EAX), priority as the recognized stack parameter
// Allocates a cache page for a sound permutation's sample data and kicks off an async read to
// fill it. Computes the page's requested size (a derived block-aligned size for format 1,
// otherwise the raw compressed sample size for formats 0/3, otherwise 0), asks the sound cache
// for a block of that size, and on success creates the matching sound_cache_entry datum and
// submits the read. If no cache block is available, dumps cache statistics and force-crashes via
// a null-pointer write (the original's own diagnostic-and-abort behaviour).
void sound_cache_page_allocate(SoundPermutation *permutation, uint8_t priority)
{
    int32_t requested_bytes;
    uint8_t data_file_index;
    datum_index page_datum;
    int32_t page_address;
    sound_cache_entry *entry;
    cache_io_completion completion;

    requested_bytes = 0;
    data_file_index = 0;
    if ((permutation->samples.flags & 1) != 0) {
        data_file_index = 2; // _cache_io_data_file_sounds
    }

    if (permutation->format == 1) {
        int32_t channel_factor;
        Sound *sound;

        sound = (Sound *)tag_instances[permutation->tag_id_1.index].data;
        channel_factor = (sound->channel_count == 1) + 1;
        requested_bytes =
            (channel_factor * 0x400 >> 3) *
            ((int32_t)permutation->samples.size /
            (int32_t)((channel_factor + (uint32_t)((uint16_t)(((uint16_t)((int16_t)(channel_factor * 0xfc) + 7U) >> 3) + 7) >> 3) *
                           2) * 4));
        permutation->buffer_size = requested_bytes;
    } else if (permutation->format == 3 || permutation->format == 0) {
        requested_bytes = permutation->samples.size;
    }

    page_datum = cache_allocate_block(sound_cache, (uint32_t)requested_bytes);
    if (page_datum != 0xffffffff) {
        page_address = (((cache_entry *)((uint8_t *)sound_cache->entries->data +
            (page_datum & 0xffff) * sizeof(cache_entry)))->offset << (sound_cache->block_shift & 0x1f)) +
            (int32_t)sound_cache_base;

        datum_new_at_index_with_salt(page_datum, sound_cache_entries);
        entry = (sound_cache_entry *)((uint8_t *)sound_cache_entries->data + (page_datum & 0xffff) * sizeof(sound_cache_entry));

        permutation->samples_pointer = page_datum;
        *(int32_t *)permutation->_pad_30 = page_address;
        entry->permutation = permutation;

        completion.flag = &entry->loaded;
        completion.procedure = (permutation->format != 1) ? cache_io_sound_decode_thunk : 0;
        completion.data = entry;
        entry->io_request_index = cache_io_request_new(&completion,
            permutation->samples.file_offset, permutation->samples.size, (void *)page_address,
            priority, data_file_index);
        return;
    }

    sound_cache_dump_to_file();
    *(uint8_t *)0 = 1; // intentional forced crash, preserved from the original
    return;
}

#if 0
Original Ghidra decompilation (0x4440e0):

void FUN_004440e0(undefined4 param_1)

{
  short sVar1;
  undefined2 uVar2;
  int in_EAX;
  int iVar3;
  uint uVar4;
  int extraout_EDX;
  int iVar5;
  undefined4 local_10;

  iVar3 = 0;
  local_10 = 0;
  if ((*(byte *)(in_EAX + 0x44) & 1) != 0) {
    local_10 = 2;
  }
  sVar1 = *(short *)(in_EAX + 0x28);
  if (sVar1 == 1) {
    iVar3 = (*(short *)(*(int *)((*(uint *)(in_EAX + 0x3c) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) +
                       0x6c) == 1) + 1;
    iVar3 = (iVar3 * 0x400 >> 3) *
            (*(int *)(in_EAX + 0x40) /
            (int)((iVar3 + (uint)((ushort)(((ushort)((short)(iVar3 * 0xfc) + 7U) >> 3) + 7) >> 3) *
                           2) * 4));
    *(int *)(in_EAX + 0x38) = iVar3;
  }
  else if ((sVar1 == 3) || (sVar1 == 0)) {
    iVar3 = *(int *)(in_EAX + 0x40);
  }
  uVar4 = FUN_004d1840(DAT_006ac530,iVar3);
  if (uVar4 != 0xffffffff) {
    iVar3 = (*(int *)(*(int *)(*(int *)(DAT_006ac530 + 0x3c) + 0x34) + 8 + (uVar4 & 0xffff) * 0x1c)
            << ((byte)*(undefined4 *)(DAT_006ac530 + 0x2c) & 0x1f)) + DAT_006ac52c;
    datum_new_at_index_with_salt();
    iVar5 = (uVar4 & 0xffff) * 0x10 + *(int *)(extraout_EDX + 0x34);
    *(uint *)(in_EAX + 0x2c) = uVar4;
    *(int *)(in_EAX + 0x30) = iVar3;
    *(int *)(iVar5 + 0xc) = in_EAX;
    uVar2 = cache_io_request_new
                      (*(undefined4 *)(in_EAX + 0x48),*(undefined4 *)(in_EAX + 0x40),iVar3,param_1,
                       local_10);
    *(undefined2 *)(iVar5 + 8) = uVar2;
    return;
  }
  sound_cache_dump_to_file();
  DAT_00000000 = 1;
  return;
}
#endif
