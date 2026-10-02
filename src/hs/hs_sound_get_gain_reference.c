// hs_sound_get_gain_reference  (Ghidra: hs_sound_get_gain_reference, already named; confirmed by
// the CEA prototype's string overlap on "the sound '%s' does not exist")
// address 0x488b10, size 115 bytes
// name confidence: 0.85   rewrite confidence: 0.7
// evidence: src/cache/tag_lookup.c's established signature (tag_group in EDI, char *path);
//   out/phase4/hs_types_notes.md hs_tag_group_for_type table for the 'snd!'/'lsnd' FourCCs.
// register convention: sound name in EAX (implicit; this function takes no recognized stack
//   parameters at all in the decompile, so the name must arrive purely by register).
//   // blam-cc: EAX -> name
// UNSURE: the exact register carrying `name` is inferred (EAX, first in the blam-cc order),
// not observed -- Ghidra shows zero parameters of any kind for this function.

#include "tags.h"
#include "cache.h"
#include "memory.h"
#include "hs.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern datum_index tag_lookup(tag_group group, char *path); // cache module, 0x442550
extern void console_print_error_va(uint8_t clear_first, const char *format, ...); // 0x4c67c0, AL clear_first

extern tag_instance *tag_instances; // 0x0087bc14

// Looks up `name` as a "snd!" (sound) tag first, returning a pointer to its gain modifier
// (tag data + 0x28) if found. Otherwise looks it up as an "lsnd" (sound_looping) tag; if found
// and its permutation-like count (+0x3c) is positive, returns a pointer into its data (+0x40 + 4,
// i.e. the second element of an array there). If neither tag exists, reports a compiler error and
// returns NULL.
float *hs_sound_get_gain_reference(char *name)
{
    datum_index tag_id;
    uint8_t *sound_data;
    uint8_t *looping_data;

    tag_id = tag_lookup(0x736e6421, name); // 'snd!'
    if (tag_id != k_datum_index_none) {
        sound_data = (uint8_t *)tag_instances[(tag_id & 0xffff) & 0xffff].data;
        return (float *)(sound_data + 0x28);
    }

    tag_id = tag_lookup(0x6c736e64, name); // 'lsnd'
    if (tag_id != k_datum_index_none) {
        looping_data = (uint8_t *)tag_instances[(tag_id & 0xffff) & 0xffff].data;
        if (0 < *(int32_t *)(looping_data + 0x3c)) {
            return (float *)(*(uint32_t *)(looping_data + 0x40) + 4);
        }
    }

    console_print_error_va(0, "the sound '%s' does not exist");
    return 0;
}

#if 0
Original Ghidra decompilation (0x488b10):

int hs_sound_get_gain_reference(void)

{
  int iVar1;
  uint uVar2;

  uVar2 = tag_lookup();
  if (uVar2 != 0xffffffff) {
    return *(int *)((uVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x28;
  }
  uVar2 = tag_lookup();
  if ((uVar2 != 0xffffffff) &&
     (iVar1 = *(int *)((uVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14), 0 < *(int *)(iVar1 + 0x3c)))
  {
    return *(int *)(iVar1 + 0x40) + 4;
  }
  console_print_error_va("the sound \'%s\' does not exist");
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
