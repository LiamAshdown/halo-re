// hs_object_list_collect_player_units  (Ghidra: FUN_00487630)
// address 0x487630, size 279 bytes
// name confidence: 0.3 (out/phase4/hs_functions.md: "Builds a linked reference list collecting a
//   per-entry index field from every valid slot of an object-related datum array"; the array and
//   field are not parameters here, they are the hardcoded players array and its unit field)
// rewrite confidence: 0.6
// evidence: types/hs.h object_list_header/object_list_reference (this module owns both, created by
//   object_lists_initialize @0x48b250) and the "globals this module reads but does not own" note
//   for players (0x0087a480, stride 0x200, unit handle at +0x34); src/memory/datum_new.c and
//   src/memory/datum_next.c for the two callees, whose bodies this function's tail loop matches
//   instruction for instruction (the second and later "find next player" steps are datum_next's
//   logic, re-inlined here rather than called again -- restored to a real call since the two are
//   bit-identical).
// register convention: none (void).
// UNSURE: no in-range caller was found for this address (out/functions.json reports 0 callers),
//   so the exact purpose (a "players" built-in HS function's evaluate handler, most likely, since
//   object_list is not a type hs_global_definition::type ever takes) and therefore the name are
//   both guesses. The mechanism itself -- allocate one object_list header, then chain a reference
//   node onto it for every player with a live unit -- is certain from the code.
// UNSURE: if the header allocation (the first datum_new) fails, the original still falls through
//   into the players loop and indexes object_list_header_data with the failed (0xffff-masked)
//   index; that out-of-bounds behavior on allocation failure is preserved as-is, not guarded.

#include "tags.h"
#include "memory.h"
#include "hs.h"

extern datum_index datum_new(data_array *array); // blam-cc: EDX -> array; memory module, 0x4d0480
extern datum_index datum_next(int16_t after_index, data_array *array);
    // blam-cc: DX -> after_index, EDI -> array; memory module, 0x4d0630

extern data_array *object_list_header_data;    // 0x0087a464
extern data_array *object_list_reference_data; // 0x0087a468
extern data_array *players;                    // 0x0087a480, stride 0x200, unit handle at +0x34

// hs_player_record: defined in types/hs.h (foreign-module slice; was a local TYPES-GAP copy)

// Allocates a fresh object_list header and chains a reference node onto it for the unit of every
// live player, in ascending player-slot order. Returns the new list's handle (or
// k_datum_index_none if the header itself could not be allocated -- see the UNSURE note above).
datum_index hs_object_list_collect_player_units(void)
{
    datum_index header_index;
    object_list_header *header;
    datum_index player_index;
    datum_index unit;
    datum_index reference_index;
    object_list_reference *reference;

    header_index = datum_new(object_list_header_data);
    if (header_index != k_datum_index_none) {
        header = (object_list_header *)((uint8_t *)object_list_header_data->data +
            (header_index & 0xffff) * 0x0c);
        header->count = 0;
        header->first_reference = k_datum_index_none;
    }

    player_index = datum_next(-1, players);
    while (player_index != k_datum_index_none) {
        unit = ((hs_player_record *)((uint8_t *)players->data +
            (player_index & 0xffff) * 0x200))->unit;
        if (unit != k_datum_index_none) {
            header = (object_list_header *)((uint8_t *)object_list_header_data->data +
                (header_index & 0xffff) * 0x0c);
            reference_index = datum_new(object_list_reference_data);
            if (reference_index != k_datum_index_none) {
                reference = (object_list_reference *)((uint8_t *)object_list_reference_data->data +
                    (reference_index & 0xffff) * 0x0c);
                reference->object_index = unit;
                reference->next = header->first_reference;
                header->first_reference = reference_index;
            }
            header->count = header->count + 1;
        }
        player_index = datum_next((int16_t)player_index, players);
    }
    return header_index;
}

#if 0
Original Ghidra decompilation (0x487630):

uint FUN_00487630(void)

{
  int iVar1;
  int iVar2;
  int iVar3;
  uint uVar4;
  uint uVar5;
  uint uVar6;
  short *psVar7;
  short sVar8;
  int iVar9;
  undefined8 uVar10;

  uVar10 = datum_new();
  uVar4 = (uint)uVar10;
  if (uVar4 != 0xffffffff) {
    iVar1 = *(int *)((int)((ulonglong)uVar10 >> 0x20) + 0x34) + (uVar4 & 0xffff) * 0xc;
    *(undefined2 *)(iVar1 + 6) = 0;
    *(undefined4 *)(iVar1 + 8) = 0xffffffff;
  }
  iVar1 = DAT_0087a480;
  uVar5 = FUN_004d0630();
  if (uVar5 == 0xffffffff) {
    return uVar4;
  }
  do {
    iVar9 = *(int *)((uVar5 & 0xffff) * 0x200 + 0x34 + *(int *)(iVar1 + 0x34));
    if (iVar9 != -1) {
      iVar2 = *(int *)(DAT_0087a464 + 0x34) + (uVar4 & 0xffff) * 0xc;
      uVar6 = datum_new();
      if (uVar6 != 0xffffffff) {
        iVar3 = *(int *)(DAT_0087a468 + 0x34) + (uVar6 & 0xffff) * 0xc;
        *(int *)(iVar3 + 4) = iVar9;
        *(undefined4 *)(iVar3 + 8) = *(undefined4 *)(iVar2 + 8);
        *(uint *)(iVar2 + 8) = uVar6;
      }
      *(short *)(iVar2 + 6) = *(short *)(iVar2 + 6) + 1;
    }
    iVar9 = uVar5 + 1;
    uVar5 = 0xffffffff;
    sVar8 = (short)iVar9;
    if ((-1 < sVar8) && (sVar8 < *(short *)(iVar1 + 0x2e))) {
      psVar7 = (short *)((int)sVar8 * (int)*(short *)(iVar1 + 0x22) + *(int *)(iVar1 + 0x34));
      do {
        if (*psVar7 != 0) {
          uVar5 = (int)*psVar7 << 0x10 | (int)(short)iVar9;
          break;
        }
        iVar9 = iVar9 + 1;
        psVar7 = (short *)((int)psVar7 + (int)*(short *)(iVar1 + 0x22));
      } while ((short)iVar9 < *(short *)(iVar1 + 0x2e));
    }
    if (uVar5 == 0xffffffff) {
      return uVar4;
    }
  } while( true );
}
#endif
