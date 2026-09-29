// squad_pick_random_starting_location  (Ghidra: squad_pick_random_starting_location, already named)
// address 0x437220, size 640 bytes
// name confidence: 0.55   rewrite confidence: 0.55
// evidence: phase-4 summary ("randomly selects an as-yet-unused starting location within a
//   squad's state bucket, recycling the pool once every location has been used"). It reads
//   the two encounter_squad_state masks encounter_squad_reset_starting_location_mask
//   @0x436f90 fills and refills the second one (memset 0xff) exactly the way that function
//   does. The random draw is the standard Blam LCG on random_seed_global followed by the
//   16.16 scale, the same idiom as every other random pick in this module.
// register convention: EAX -> squad_index, ECX -> encounter_index. The 0x1008-byte
//   auStack_1008 Ghidra reports is a phantom frame; the only real local state is the
//   two-dword "already taken this call" mask (local_8) and the loop scalars.
//   // blam-cc: EAX -> squad_index, ECX -> encounter_index
//
// UNSURE (high): this is the hardest function of this batch to read cleanly.
//  - Ghidra models the two masks as `puVar10[i >> 5]` (the +0x00 mask) and
//    `puVar10[(i >> 5) + 1]` (the +0x04 mask). For any squad with 32 or fewer starting
//    locations, which is what the layout allows, those are exactly
//    starting_location_mask and starting_location_free.
//  - `local_8` is a two-dword scratch mask that is written nowhere in this function; it is
//    read as "locations already excluded" and is always zero in practice. It is kept here so
//    the tests stay faithful, marked as the phantom it is.
//  - The first pass clears the chosen bit from BOTH masks (`*puVar1 &= ~bit` then
//    `puVar1[1] &= ~bit`), while the second pass clears it only from the free mask.
//  - The original returns the index in AX with garbage in the high half (the CONCAT22 in
//    Ghidra's rendering); only the low 16 bits are meaningful.
// Flagged for hook verification.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "fn_ai.h"

extern encounter_squad_state *encounter_squad_states; // 0x008802cc
extern data_array *encounter_data;                    // 0x008802c8
extern Scenario *global_scenario;                     // 0x00746f8c
extern uint32_t random_seed_global;                   // 0x00719cd0

// blam-cc: EAX -> squad_index, ECX -> encounter_index
// Hands out one of the squad's starting locations. Locations still set in
// starting_location_mask are preferred and are consumed from both masks; once that pool is
// empty the free mask is used, and when it too runs out it is refilled to all ones and the
// draw is retried. Returns -1 when the squad has no usable location at all.
int16_t squad_pick_random_starting_location(datum_index encounter_index, int16_t squad_index)
{
    encounter *enc;
    ScenarioEncounter *encounter_definition;
    ScenarioSquad *squad_definition;
    uint32_t *masks;
    uint32_t taken[2];   // phantom: never written by this function, always zero in practice
    uint32_t *slot;
    uint32_t clear_mask;
    uint32_t bit;
    uint32_t draw;
    uint32_t dword_count;
    uint32_t *fill;
    int16_t available;
    int16_t cursor;
    int16_t chosen;
    int16_t free_available;
    int32_t location_index;
    int32_t location_count;
    char any_unavailable;

    enc = &((encounter *)encounter_data->data)[encounter_index & 0xffff];
    encounter_definition = &((ScenarioEncounter *)global_scenario->encounters.pointer)
        [encounter_index & 0xffff];
    squad_definition = &((ScenarioSquad *)encounter_definition->squads.pointer)[squad_index];
    masks = &encounter_squad_states[(int16_t)(enc->first_squad + squad_index)]
        .starting_location_mask;

    available = 0;
    taken[0] = 0;
    cursor = 0;
    chosen = -1;
    taken[1] = 0;

    if (0 < squad_definition->starting_locations.count) {
        location_index = 0;
        do {
            bit = 1 << (location_index & 0x1f);
            if ((masks[location_index >> 5] & bit) != 0 &&
                (taken[location_index >> 5] & bit) == 0) {
                available = available + 1;
            }
            cursor = cursor + 1;
            location_index = (int32_t)cursor;
        } while (location_index < squad_definition->starting_locations.count);

        if (0 < available) {
            random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
            draw = (uint32_t)(((random_seed_global >> 0x10) * (int32_t)available) >> 0x10);
            cursor = 0;
            if (0 < squad_definition->starting_locations.count) {
                location_index = 0;
                do {
                    bit = 1 << (location_index & 0x1f);
                    if ((masks[location_index >> 5] & bit) != 0 &&
                        (taken[location_index >> 5] & bit) == 0) {
                        if ((int16_t)draw == 0) {
                            slot = masks + ((int32_t)cursor >> 5);
                            clear_mask = ~(1 << (cursor & 0x1f));
                            *slot = masks[(int32_t)cursor >> 5] & clear_mask;
                            slot[1] = slot[1] & clear_mask;
                            chosen = cursor;
                            if (cursor != -1) {
                                return chosen;
                            }
                            break;
                        }
                        draw = draw - 1;
                    }
                    cursor = cursor + 1;
                    location_index = (int32_t)cursor;
                } while (location_index < squad_definition->starting_locations.count);
            }
        }
    }

    for (;;) {
        cursor = chosen;
        location_count = squad_definition->starting_locations.count;
        location_index = 0;
        any_unavailable = 0;
        free_available = 0;
        if (location_count < 1) {
            return cursor;
        }
        {
            int32_t i = 0;
            uint32_t scan = 0;
            do {
                bit = 1 << (scan & 0x1f);
                if ((taken[(int32_t)scan >> 5] & bit) == 0) {
                    if ((masks[((int32_t)scan >> 5) + 1] & bit) == 0) {
                        any_unavailable = 1;
                    } else {
                        free_available = free_available + 1;
                    }
                }
                i = i + 1;
                scan = (uint32_t)(int16_t)i;
            } while ((int32_t)scan < location_count);
        }

        if (any_unavailable == 0 || free_available != 0) {
            if (0 < free_available) {
                random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
                draw = (uint32_t)(((random_seed_global >> 0x10) * (int32_t)free_available) >> 0x10);
                {
                    uint32_t pick = 0;
                    location_index = 0;
                    if (0 < squad_definition->starting_locations.count) {
                        do {
                            bit = 1 << (location_index & 0x1f);
                            if ((masks[(location_index >> 5) + 1] & bit) != 0 &&
                                (taken[location_index >> 5] & bit) == 0) {
                                if ((int16_t)draw == 0) {
                                    masks[((int32_t)(int16_t)pick >> 5) + 1] =
                                        masks[((int32_t)(int16_t)pick >> 5) + 1] &
                                        ~(1 << (pick & 0x1f));
                                    return (int16_t)pick;
                                }
                                draw = draw - 1;
                            }
                            pick = pick + 1;
                            location_index = (int32_t)(int16_t)pick;
                        } while (location_index < squad_definition->starting_locations.count);
                    }
                }
            }
            return cursor;
        }

        // every location is used up: refill the free mask and draw again
        fill = masks + 1;
        dword_count = (uint32_t)((location_count + 0x1f) >> 5);
        for (; dword_count != 0; dword_count = dword_count - 1) {
            *fill = 0xffffffff;
            fill = fill + 1;
        }
        chosen = cursor;
    }
}

#if 0
Original Ghidra decompilation (0x437220):

uint squad_pick_random_starting_location(void)

{
  uint *puVar1;
  short sVar2;
  int in_EAX;
  int iVar3;
  int iVar4;
  uint in_ECX;
  uint uVar5;
  uint uVar6;
  short sVar7;
  int iVar8;
  short sVar9;
  undefined4 *puVar10;
  undefined4 *puVar11;
  uint uVar12;
  uint auStack_1008 [1016];
  char local_15;
  uint local_8 [2];

  iVar3 = in_EAX * 0xe8 +
          *(int *)((in_ECX & 0xffff) * 0xb0 + 0x84 + *(int *)(global_scenario + 0x430));
  puVar10 = (undefined4 *)
            ((short)(*(short *)((in_ECX & 0xffff) * 0x6c + 4 + *(int *)(DAT_008802c8 + 0x34)) +
                    (short)in_EAX) * 0x20 + DAT_008802cc);
  sVar9 = 0;
  local_8[0] = 0;
  sVar7 = 0;
  sVar2 = -1;
  local_8[1] = 0;
  if (0 < *(int *)(iVar3 + 0xd0)) {
    iVar4 = 0;
    do {
      uVar5 = 1 << ((byte)iVar4 & 0x1f);
      if (((puVar10[iVar4 >> 5] & uVar5) != 0) && ((local_8[iVar4 >> 5] & uVar5) == 0)) {
        sVar9 = sVar9 + 1;
      }
      sVar7 = sVar7 + 1;
      iVar4 = (int)sVar7;
    } while (iVar4 < *(int *)(iVar3 + 0xd0));
    if (0 < sVar9) {
      random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
      uVar5 = (random_seed_global >> 0x10) * (int)sVar9 >> 0x10;
      sVar7 = 0;
      if (0 < *(int *)(iVar3 + 0xd0)) {
        iVar4 = 0;
        do {
          uVar6 = 1 << ((byte)iVar4 & 0x1f);
          if (((puVar10[iVar4 >> 5] & uVar6) != 0) && ((local_8[iVar4 >> 5] & uVar6) == 0)) {
            if ((short)uVar5 == 0) {
              puVar1 = puVar10 + ((int)sVar7 >> 5);
              uVar5 = ~(1 << ((byte)sVar7 & 0x1f));
              *puVar1 = puVar10[(int)sVar7 >> 5] & uVar5;
              puVar1[1] = puVar1[1] & uVar5;
              sVar2 = sVar7;
              if (sVar7 != -1) goto LAB_0043746d;
              break;
            }
            uVar5 = uVar5 - 1;
          }
          sVar7 = sVar7 + 1;
          iVar4 = (int)sVar7;
        } while (iVar4 < *(int *)(iVar3 + 0xd0));
      }
    }
  }
  do {
    sVar7 = sVar2;
    iVar4 = *(int *)(iVar3 + 0xd0);
    uVar5 = 0;
    iVar8 = 0;
    local_15 = '\0';
    sVar2 = 0;
    if (iVar4 < 1) {
LAB_0043746d:
      return CONCAT22((short)(uVar5 >> 0x10),sVar7);
    }
    do {
      uVar6 = 1 << ((byte)uVar5 & 0x1f);
      if ((local_8[(int)uVar5 >> 5] & uVar6) == 0) {
        if ((puVar10[((int)uVar5 >> 5) + 1] & uVar6) == 0) {
          local_15 = '\x01';
        }
        else {
          sVar2 = sVar2 + 1;
        }
      }
      iVar8 = iVar8 + 1;
      uVar5 = (uint)(short)iVar8;
    } while ((int)uVar5 < iVar4);
    uVar5 = CONCAT31((int3)(char)((uint)iVar8 >> 8),local_15);
    if ((local_15 == '\0') || (sVar2 != 0)) {
      if (0 < sVar2) {
        random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
        uVar6 = (random_seed_global >> 0x10) * (int)sVar2 >> 0x10;
        uVar5 = 0;
        if (0 < *(int *)(iVar3 + 0xd0)) {
          iVar4 = 0;
          do {
            uVar12 = 1 << ((byte)iVar4 & 0x1f);
            if (((puVar10[(iVar4 >> 5) + 1] & uVar12) != 0) && ((local_8[iVar4 >> 5] & uVar12) == 0)
               ) {
              if ((short)uVar6 == 0) {
                puVar10[((int)(short)uVar5 >> 5) + 1] =
                     puVar10[((int)(short)uVar5 >> 5) + 1] & ~(1 << ((byte)uVar5 & 0x1f));
                return uVar5;
              }
              uVar6 = uVar6 - 1;
            }
            uVar5 = uVar5 + 1;
            iVar4 = (int)(short)uVar5;
          } while (iVar4 < *(int *)(iVar3 + 0xd0));
        }
      }
      goto LAB_0043746d;
    }
    puVar11 = puVar10;
    for (uVar5 = iVar4 + 0x1f >> 5 & 0x3fffffff; puVar11 = puVar11 + 1, uVar5 != 0;
        uVar5 = uVar5 - 1) {
      *puVar11 = 0xffffffff;
    }
    for (iVar4 = 0; sVar2 = sVar7, iVar4 != 0; iVar4 = iVar4 + -1) {
      *(undefined1 *)puVar11 = 0xff;
      puVar11 = (undefined4 *)((int)puVar11 + 1);
    }
  } while( true );
}
#endif
