// ai_object_list_start_user_animation_until_failure  (Ghidra: FUN_00561e60; named for this rewrite)
// address 0x561e60, size 275 bytes
// name confidence: 0.4   rewrite confidence: 0.4
// evidence: same object_list walk and unit-type filter (`(1 << type) & 3`, types/hs.h /
//   types/objects.h) as ai_object_list_set_unit_flag_800/_800000, but instead of setting a flag
//   it calls unit_start_user_animation (0x5702a0, `units`) on each qualifying member, and latches
//   to "stop trying" (bVar2, `still_succeeding` below) the first time that call returns 0 -- once
//   one member fails to start the animation, every later member in the list is skipped too
//   (walked past, but never called).
// register convention: object_list_header handle in ECX; three stack parameters, but only the
//   third reaches unit_start_user_animation (as Ghidra's own decompile shows it: the first two
//   are loaded into EDI/EAX right before the call site and then never pushed or otherwise used --
//   dead loads, matching Ghidra's own choice to declare `param_1`/`param_2` but never reference
//   them in the body). Confirmed against objdump 0x561e60..0x561f72.
//   // blam-cc: ECX -> object_list_header_handle, stack -> (graph_tag_id, animation_name, interpolate)
// UNSURE: unit_start_user_animation's own file documents `EAX -> object_index, EDI ->
//   graph_tag_id, second register -> warn_if_missing` at rewrite confidence 0.3 ("the
//   animation-name string and the graph tag id arrive in registers Ghidra could not source at
//   all"). At THIS call site the two stack pushes are the current walk unit and this function's
//   own 3rd parameter, which does not obviously line up with that convention (EAX/EDI are loaded
//   from dead values here, not from the walk state). Rather than force an unverified binding onto
//   either file, this rewrite keeps the call exactly as Ghidra shows it --
//   `unit_start_user_animation(unit_index, graph_tag_id)` -- as one more opaque call in this
//   already-low-confidence pair, per this task's priority of literal-argument preservation over
//   invented register reconstruction.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "hs.h"
#include "ai.h"

extern data_array *object_data;                // 0x008603b0
extern data_array *object_list_header_data;    // 0x0087a464
extern data_array *object_list_reference_data; // 0x0087a468

// UNSURE: opaque call, exactly the arguments Ghidra's own decompile of this call site shows; see
// file header. Not the same binding as unit_start_user_animation.c's own established extern.
extern uint8_t unit_start_user_animation(uint32_t unit_index, datum_index graph_tag, const char *animation_name,
    uint8_t interpolate); // 0x5702a0

uint8_t ai_object_list_start_user_animation_until_failure(datum_index object_list_header_handle,
    datum_index graph_tag_id, const char *animation_name, uint8_t interpolate)
    // returns AL = still_succeeding (1 for an empty list; custom_animation_list reads it at 0x47bb8d)
{
    datum_index node_index = (datum_index)k_datum_index_none;
    datum_index object_index = (datum_index)k_datum_index_none;
    uint8_t still_succeeding = 1;


    if (object_list_header_handle != (datum_index)k_datum_index_none) {
        object_list_header *header =
            (object_list_header *)((uint8_t *)object_list_header_data->data +
                                    (object_list_header_handle & 0xffff) * 0x0c);
        node_index = header->first_reference;
        if (node_index != (datum_index)k_datum_index_none) {
            object_list_reference *node =
                (object_list_reference *)((uint8_t *)object_list_reference_data->data +
                                           (node_index & 0xffff) * 0x0c);
            node_index = node->next;
            object_index = node->object_index;
        }
    }

    while (object_index != (datum_index)k_datum_index_none) {
        object_header *entry = 0;
        if (0 <= (int16_t)object_index && (int16_t)object_index < object_data->maximum_count) {
            object_header *candidate = &((object_header *)object_data->data)[(int16_t)object_index];
            if (candidate->identifier != 0 &&
                ((int16_t)(object_index >> 0x10) == 0 || candidate->identifier == (int16_t)(object_index >> 0x10))) {
                entry = candidate;
            }
        }

        if (entry != 0 && ((1 << (entry->type & 0x1f)) & 3) != 0 && entry->data != 0) {
            if (still_succeeding && unit_start_user_animation((uint32_t)object_index, graph_tag_id, animation_name,
                    interpolate) != 0) {
                // FIXED (0x561f14..0x561f22): EDI = stack arg 1 (graph), EAX = stack arg 2 (name), pushed
                // (unit, stack arg 3 interpolate); the draft took the graph from arg 3 and dropped the rest.
                still_succeeding = 1;
            } else {
                still_succeeding = 0;
            }
        }

        if (node_index == (datum_index)k_datum_index_none) {
            object_index = (datum_index)k_datum_index_none;
        } else {
            object_list_reference *node =
                (object_list_reference *)((uint8_t *)object_list_reference_data->data +
                                           (node_index & 0xffff) * 0x0c);
            object_index = node->object_index;
            node_index = node->next;
        }
    }
    return still_succeeding;
}

#if 0
Original Ghidra decompilation (0x561e60):

void FUN_00561e60(undefined4 param_1,undefined4 param_2,undefined4 param_3)

{
  uint uVar1;
  bool bVar2;
  char cVar3;
  uint in_ECX;
  uint uVar4;
  short *psVar5;
  short sVar6;
  int iVar7;
  int iVar8;

  iVar7 = -1;
  bVar2 = true;
  iVar8 = DAT_008603b0;
  if (in_ECX == 0xffffffff) {
    uVar1 = 0xffffffff;
  }
  else {
    uVar4 = *(uint *)(*(int *)(DAT_0087a464 + 0x34) + 8 + (in_ECX & 0xffff) * 0xc);
    if (uVar4 == 0xffffffff) {
      iVar7 = -1;
      uVar1 = 0xffffffff;
    }
    else {
      uVar4 = uVar4 & 0xffff;
      uVar1 = *(uint *)(*(int *)(DAT_0087a468 + 0x34) + 8 + uVar4 * 0xc);
      iVar7 = *(int *)(*(int *)(DAT_0087a468 + 0x34) + uVar4 * 0xc + 4);
    }
  }
  while (iVar7 != -1) {
    if (((iVar7 != -1) && (sVar6 = (short)iVar7, -1 < sVar6)) && (sVar6 < *(short *)(iVar8 + 0x20)))
    {
      psVar5 = (short *)((int)*(short *)(iVar8 + 0x22) * (int)sVar6 + *(int *)(iVar8 + 0x34));
      if ((((*psVar5 != 0) &&
           ((sVar6 = (short)((uint)iVar7 >> 0x10), sVar6 == 0 || (*psVar5 == sVar6)))) &&
          ((1 << (*(byte *)((int)psVar5 + 3) & 0x1f) & 3U) != 0)) && (*(int *)(psVar5 + 4) != 0)) {
        if ((bVar2) &&
           (cVar3 = unit_start_user_animation(iVar7,param_3), iVar8 = DAT_008603b0, cVar3 != '\0'))
        {
          bVar2 = true;
        }
        else {
          bVar2 = false;
        }
      }
    }
    if (uVar1 == 0xffffffff) {
      iVar7 = -1;
      uVar1 = 0xffffffff;
    }
    else {
      uVar4 = uVar1 & 0xffff;
      uVar1 = *(uint *)(*(int *)(DAT_0087a468 + 0x34) + 8 + uVar4 * 0xc);
      iVar7 = *(int *)(*(int *)(DAT_0087a468 + 0x34) + uVar4 * 0xc + 4);
    }
  }
  return;
}
#endif
