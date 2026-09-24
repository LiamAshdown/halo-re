// decal_spawn_for_response  (Ghidra: FUN_0044ece0; named per its own summary in
// out/phase4/effects_functions.md: "Decides whether a decal should be spawned for the current
// collision/damage response ... and, if so, invokes the decal placement algorithm")
// address 0x44ece0, size 209 bytes
// name confidence: 0.4   rewrite confidence: 0.2 (LOW -- see UNSURE notes)
// evidence: types/effects.h decals_enabled (0x00687004) and decals_for_all_responses (0x006893f5)
// globals list; collision_test_movement_segment (0x100061) matches the "tunable check" pattern used elsewhere in
// this codebase (an in-game console tunable id).
// register convention: everything here is register-passed and elided by Ghidra --
// unaff_ESI (a tag/response index), unaff_BL (a "deterministic" flag), in_ECX (a 3-int seed
// triple) -- with no stack arguments recovered at all.
//   // blam-cc: ESI -> response_tag_index, BL -> deterministic, ECX -> seed_words
// UNSURE: `local_54` is read in the original decompile without ever being written inside this
// function -- Ghidra lost whatever set it (most likely a field collision_test_movement_segment or an earlier,
// un-shown call actually fills). Preserved as a local the compiler leaves uninitialized rather
// than invented a value for; the real value can only come from re-disassembling this function.
// UNSURE: the deterministic-seed reshuffle (`effect_random_seed = seed[2]^seed[1]^seed[0]^
// 0xdeadc0de`, saved and restored around the call) is preserved exactly despite not being able
// to confirm what `seed_words` points at.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "effects.h"

extern uint8_t decals_enabled;             // 0x00687004
extern uint8_t decals_for_all_responses;   // 0x006893f5
extern tag_instance *tag_instances;        // 0x0087bc14
extern random_seed effect_random_seed;     // 0x00719cd4

extern uint8_t collision_test_movement_segment(int32_t tunable_id); // 0x505880, outside this batch
extern void decal_place(void); // 0x44edc0, this module; UNSURE full signature, see decal_place.c

// Gates a decal spawn for the current collision/damage response: only proceeds when decals are
// enabled and either every response spawns decals or this specific response tag requests type 3,
// then (optionally reseeding the RNG deterministically first) checks the decals tunable and a
// response flag before invoking decal_place.
void decal_spawn_for_response(datum_index response_tag_index, uint8_t deterministic,
    uint32_t *seed_words)
{
    uint8_t allowed = 1;
    int16_t local_54; // UNSURE: never written in the original decompile, see file header

    if (decals_for_all_responses == 0 &&
        (deterministic != 1 ||
            *(int16_t *)((uint8_t *)tag_instances[(uint16_t)response_tag_index].data + 4) != 3)) {
        allowed = 0;
    }

    if (decals_enabled != 0 && allowed) {
        random_seed saved_seed = 0;

        if (deterministic != 0) {
            saved_seed = effect_random_seed;
            effect_random_seed = seed_words[2] ^ seed_words[1] ^ seed_words[0] ^ 0xdeadc0de;
        }

        if (collision_test_movement_segment(0x100061) != 0 && local_54 == 2 &&
            (*(uint8_t *)tag_instances[(uint16_t)response_tag_index].data & 0x10) == 0) {
            decal_place();
        }

        if (deterministic != 0) {
            effect_random_seed = saved_seed;
        }
    }
}

#if 0
Original Ghidra decompilation (0x44ece0):

void FUN_0044ece0(void)

{
  bool bVar1;
  int iVar2;
  char cVar3;
  uint *in_ECX;
  char unaff_BL;
  uint unaff_ESI;
  uint uVar4;
  short local_54;

  iVar2 = DAT_0087bc14;
  bVar1 = true;
  if ((DAT_006893f5 == '\0') &&
     ((unaff_BL != '\x01' ||
      (*(short *)(*(int *)((unaff_ESI & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 4) != 3)))) {
    bVar1 = false;
  }
  if ((DAT_00687004 != '\0') && (bVar1)) {
    uVar4 = 0;
    if (unaff_BL != '\0') {
      uVar4 = DAT_00719cd4;
      DAT_00719cd4 = in_ECX[2] ^ in_ECX[1] ^ *in_ECX ^ 0xdeadc0de;
    }
    cVar3 = FUN_00505880(0x100061);
    if (((cVar3 != '\0') && (local_54 == 2)) &&
       ((**(byte **)((unaff_ESI & 0xffff) * 0x20 + 0x14 + iVar2) & 0x10) == 0)) {
      FUN_0044edc0();
    }
    if (unaff_BL != '\0') {
      DAT_00719cd4 = uVar4;
    }
  }
  return;
}
#endif
