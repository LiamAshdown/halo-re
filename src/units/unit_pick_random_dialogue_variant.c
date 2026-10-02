// unit_pick_random_dialogue_variant  (Ghidra: unit_pick_random_dialogue_variant)
// address 0x561a00, size 168 bytes
// name confidence: 0.35 (renamed from phase2's "unit_pick_random_permutation_index"; see
//   evidence)   rewrite confidence: 0.85 (VERIFIED 2026-09-27 against objdump 0x561a00..0x561aa7 (EBX unit tag, stack variant; -1 matches all; seed 0x719cd0).)
// evidence: types/tags.h UnitDialogueVariant (0x18 bytes, variant_number at +0x0, dialogue
//   TagDependency at +0x8, tag_id at +0x14), Unit.dialogue_variants (TagReflexive at 0x2b4/
//   0x2b8, right after Unit.new_hud_interfaces at 0x2a8). types/math.h random_seed_global.
// register convention: the owning Unit tag data pointer in an unresolved register
//   (unaff_EBX -- every caller in this batch is unit_choose_dialogue_variant, 0x561990, which
//   also never reloads a register before calling this, so it is modelled as an explicit
//   parameter), requested variant_number (or -1 for "any") in the recognized parameter.
//   // blam-cc: unaff_EBX -> unit_tag, param_1 -> variant_number
// UNSURE: functions.md's summary ("permutation table") and phase2's name do not match this
//   decompilation, which walks Unit.dialogue_variants and returns a TagID, not a permutation
//   index; renamed accordingly.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern random_seed random_seed_global; // 0x00719cd0

TagID unit_pick_random_dialogue_variant(Unit *unit_tag, int16_t variant_number) // blam-cc: see file header
{
    UnitDialogueVariant *variants = (UnitDialogueVariant *)unit_tag->dialogue_variants.pointer;
    int32_t count = (int32_t)unit_tag->dialogue_variants.count;
    int16_t matches[16]; // matches Ghidra's local_20[16]; a match count above 16 overflows this
                         // array in the original binary too (asStack_10020[32762] sits right
                         // below it on the stack and is never otherwise referenced) -- preserved
                         // as-is rather than silently enlarging the buffer
    int16_t match_count = 0;

    if (count > 0) {
        for (int16_t i = 0; i < count; i++) {
            if (variant_number == -1 || variants[i].variant_number == variant_number) {
                matches[match_count] = i;
                match_count = match_count + 1;
            }
        }
        if (match_count > 0) {
            int16_t chosen;
            if (match_count == 1) {
                chosen = matches[0];
            } else {
                random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
                chosen = matches[(int16_t)((random_seed_global >> 0x10) * (uint32_t)match_count >> 0x10)];
            }
            return variants[chosen].dialogue.tag_id;
        }
    }
    TagID none = {0xffff, 0xffff};
    return none;
}

#if 0
Original Ghidra decompilation (0x561a00):

undefined4 FUN_00561a00(short param_1)

{
  int iVar1;
  int iVar2;
  short sVar3;
  int unaff_EBX;
  short sVar4;
  short asStack_10020 [32762];
  short local_20 [16];

  iVar1 = *(int *)(unaff_EBX + 0x2b4);
  sVar4 = 0;
  sVar3 = 0;
  if (0 < iVar1) {
    iVar2 = 0;
    do {
      if ((param_1 == -1) || (*(short *)(*(int *)(unaff_EBX + 0x2b8) + iVar2 * 0x18) == param_1)) {
        local_20[sVar4] = sVar3;
        sVar4 = sVar4 + 1;
      }
      sVar3 = sVar3 + 1;
      iVar2 = (int)sVar3;
    } while (iVar2 < iVar1);
    if (0 < sVar4) {
      if (sVar4 == 1) {
        sVar3 = (short)local_20._0_4_;
      }
      else {
        random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
        sVar3 = local_20[(short)((random_seed_global >> 0x10) * (int)sVar4 >> 0x10)];
      }
      return *(undefined4 *)(*(int *)(unaff_EBX + 0x2b8) + 0x14 + sVar3 * 0x18);
    }
  }
  return 0xffffffff;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
