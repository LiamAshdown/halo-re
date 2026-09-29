// ai_pick_weighted_candidate  (Ghidra: ai_pick_weighted_candidate; named for this rewrite)
// address 0x438480, size 242 bytes
// name confidence: 0.4   rewrite confidence: 0.5
// evidence: phase-4 summary ("selects one of up to four weighted candidate records at
//   random, proportionally to their weight values"). The record stride is 0x20 and the slot
//   layout ({handle, weight, payload, key}, 0x10 bytes) is the one
//   ai_insert_scored_candidate_pair @0x4383f0 (already rewritten) maintains: each of the four
//   buckets holds a best and a runner-up entry, and only the best of each is considered here.
//   The random draw is the standard Blam LCG plus the 1/65536 scale at 0x00672b84.
// register convention: EBX -> the candidate table, stack -> out_entry.
//   // blam-cc: EBX -> table, stack -> out_entry
//
// UNSURE: `pfVar5[-1] != -NAN` in Ghidra is the raw 32-bit test "handle != 0xffffffff" on a
// slot the decompiler typed as float; it is written here as an integer comparison.
// Ghidra also folds the return into CONCAT22 with leftover register bits: only the low 16
// bits (the chosen bucket index, or -1) are meaningful.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "fn_ai.h"

extern float k_random_scale_65536; // 0x00672b84, 1.5259022e-05 = 1/65536
extern uint32_t random_seed_global; // 0x00719cd0

// blam-cc: EBX -> table, stack -> out_entry
// Picks one of the four candidate buckets in proportion to its best entry's weight and
// copies that entry into out_entry. Returns the bucket index, or -1 when no bucket has a
// positive weight with a live handle.
int16_t ai_pick_weighted_candidate(ai_scored_candidate *table, ai_scored_candidate *out_entry)
{
    float total_weight;
    float running;
    int16_t last_valid;
    int16_t valid_count;
    int16_t i;
    int16_t chosen;
    ai_scored_candidate *entry;

    total_weight = 0.0f;
    last_valid = -1;
    valid_count = 0;

    for (i = 0; i < 4; i = i + 1) {
        entry = &table[i * 2]; // the best entry of bucket i; stride 0x20, entry size 0x10
        if (0.0f < entry->score && entry->handle != (datum_index)k_datum_index_none) {
            total_weight = total_weight + entry->score;
            valid_count = valid_count + 1;
            last_valid = i;
        }
    }

    chosen = last_valid;
    if (1 < valid_count) {
        random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
        running = 0.0f;
        for (i = 0; i < 4; i = i + 1) {
            entry = &table[i * 2];
            chosen = last_valid;
            if (0.0f < entry->score && entry->handle != (datum_index)k_datum_index_none) {
                running = running + entry->score;
                chosen = i;
                if ((float)(random_seed_global >> 0x10) * k_random_scale_65536 * total_weight <
                    running) {
                    break;
                }
            }
        }
    }

    if (chosen != -1) {
        entry = &table[chosen * 2];
        out_entry->handle = entry->handle;
        out_entry->score = entry->score;
        out_entry->payload = entry->payload;
        out_entry->key = entry->key;
    }
    return chosen;
}

#if 0
Original Ghidra decompilation (0x438480):

undefined4 FUN_00438480(undefined4 *param_1)

{
  int *piVar1;
  float fVar2;
  float fVar3;
  uint in_EAX;
  ushort uVar4;
  float *pfVar5;
  undefined4 *puVar6;
  short sVar7;
  short sVar8;
  int unaff_EBX;
  short sVar9;

  fVar3 = 0.0;
  sVar9 = -1;
  sVar8 = 0;
  sVar7 = 0;
  pfVar5 = (float *)(unaff_EBX + 4);
  do {
    in_EAX = in_EAX & 0xffff0000;
    if ((0.0 < *pfVar5) && (pfVar5[-1] != -NAN)) {
      fVar3 = fVar3 + *pfVar5;
      sVar8 = sVar8 + 1;
      sVar9 = sVar7;
    }
    sVar7 = sVar7 + 1;
    pfVar5 = pfVar5 + 8;
  } while (sVar7 < 4);
  sVar7 = sVar9;
  if (1 < sVar8) {
    random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
    sVar8 = 0;
    fVar2 = 0.0;
    do {
      piVar1 = (int *)(sVar8 * 0x20 + unaff_EBX);
      uVar4 = (ushort)((uint)(sVar8 * 0x20) >> 0x10);
      in_EAX = (uint)uVar4 << 0x10;
      if ((0.0 < (float)piVar1[1]) && (*piVar1 != -1)) {
        fVar2 = fVar2 + (float)piVar1[1];
        in_EAX = (uint)uVar4 << 0x10;
        sVar7 = sVar8;
        if ((float)(random_seed_global >> 0x10) * 1.5259022e-05 * fVar3 < fVar2) break;
      }
      sVar8 = sVar8 + 1;
      sVar7 = sVar9;
    } while (sVar8 < 4);
  }
  if (sVar7 != -1) {
    puVar6 = (undefined4 *)(sVar7 * 0x20 + unaff_EBX);
    *param_1 = *puVar6;
    param_1[1] = puVar6[1];
    param_1[2] = puVar6[2];
    in_EAX = puVar6[3];
    param_1[3] = in_EAX;
  }
  return CONCAT22((short)(in_EAX >> 0x10),sVar7);
}
#endif
