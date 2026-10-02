// ai_insert_scored_candidate_pair  (Ghidra: ai_insert_scored_candidate_pair; named for this rewrite)
// address 0x4383f0, size 130 bytes
// name confidence: 0.3   rewrite confidence: 0.4
// evidence: insertion-sorts a new (x, score, y, z) candidate into a fixed 2-element
// ascending-by-score list (record stride 0x10 bytes: handle, score, payload, key -- now
// types/ai.h ai_scored_candidate), shifting the existing
// entries down when the new one is better. Matches the phase-4 summary exactly.
// register convention: Ghidra fully resolved all five parameters.
//   // blam-cc: stack -> list_base, handle, score, payload, key

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

// Preserved exactly as decompiled: the loop does not stop after inserting at slot 0, so if
// slot 1's score (which may just have been overwritten by the shift) is *also* less than
// `score`, the same value gets inserted again at slot 1. Not simplified away, since it is
// not clear whether this is intentional or a genuine quirk of the original code.
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
uint8_t ai_insert_scored_candidate_pair(ai_scored_candidate *list, datum_index handle,
                                        float score, datum_index payload, datum_index key)
{
    uint8_t inserted = 0;
    int16_t i;

    for (i = 0; i < 2; i++) {
        if (list[i].score < score) {
            if (i < 1) {
                list[1] = list[0];
            }
            list[i].handle = handle;
            list[i].score = score;
            list[i].payload = payload;
            list[i].key = key;
            inserted = 1;
        }
    }

    return inserted;
}

#if 0
Original Ghidra decompilation (0x4383f0):

undefined1 FUN_004383f0(int param_1,float param_2,float param_3,float param_4,float param_5)

{
  undefined4 *puVar1;
  float *pfVar2;
  undefined1 uVar3;
  short sVar4;
  uint uVar5;

  uVar3 = 0;
  sVar4 = 0;
  pfVar2 = (float *)(param_1 + 4);
  do {
    if (*pfVar2 < param_3) {
      if (sVar4 < 1) {
        uVar5 = (uint)(ushort)(1 - sVar4);
        puVar1 = (undefined4 *)(param_1 + 0x10);
        do {
          uVar5 = uVar5 - 1;
          *puVar1 = puVar1[-4];
          puVar1[1] = puVar1[-3];
          puVar1[2] = puVar1[-2];
          puVar1[3] = puVar1[-1];
          puVar1 = puVar1 + -4;
        } while (uVar5 != 0);
      }
      *pfVar2 = param_3;
      pfVar2[-1] = param_2;
      pfVar2[2] = param_5;
      pfVar2[1] = param_4;
      uVar3 = 1;
    }
    sVar4 = sVar4 + 1;
    pfVar2 = pfVar2 + 4;
  } while (sVar4 < 2);
  return uVar3;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
