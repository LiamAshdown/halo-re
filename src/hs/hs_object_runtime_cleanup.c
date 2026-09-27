// hs_object_runtime_cleanup  (Ghidra: FUN_00487dd0)
// address 0x487dd0, size 275 bytes
// name confidence: 0.3 (out/phase4/hs_functions.md: "Cleans up an object-related datum table at
//   HS runtime reset, deleting orphaned children and dynamically created objects left over from
//   the previous state")
// rewrite confidence: 0.4
// evidence: types/memory.h data_iterator (exact match for phase 1's local_10/local_c+local_a/
//   local_8, whose split fields are data_iterator_next's own "int16 written, int32 read"
//   next_index quirk -- see src/memory/data_iterator_next.c); out/phase4/hs_types_notes.md
//   object parent offset (0x11c); hs_object_hierarchy_test (0x487c10, this module).
// register convention: none (void).
// `local_4`, player_data ^ 0x69746572, is the data_iterator +0x0c signature (types/memory.h,
//   R16), not a /GS canary; it is stored with the iterator.
// UNSURE: Phase 2's iterator (passed to the
//   not-yet-recovered object_iterator_next) is NOT the same data_iterator shape -- its first word
//   is initialized to -1 rather than a pointer -- so it is modeled here as a guessed
//   {type_filter; next_index; index} triple, flagged TYPES-GAP. unit_detach_from_seat, object_delete_unparented and
//   object_delete_recursive's exact semantics are not recovered either.
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original

#include "tags.h"
#include "memory.h"
#include "hs.h"
#include <stdint.h>

extern void *data_iterator_next(data_iterator *iterator);        // memory module, 0x4d05d0
extern char hs_object_hierarchy_test(datum_index object_index);  // this module, 0x487c10
extern void unit_detach_from_seat(datum_index object_index, int32_t param_2, int32_t param_3,
    int32_t param_4);                                             // units module, 0x56c640
extern void object_delete_unparented(uint32_t object_index); // 0x4f5aa0, EDI
extern void object_delete_recursive(datum_index object_index, int32_t param_2); // objects module, 0x4f59d0

// hs_object_iterator_state: defined in types/hs.h (foreign-module slice; was a local TYPES-GAP copy)

extern void *object_iterator_next(hs_object_iterator_state *iterator); // objects module, 0x4f6f20

extern data_array *players;        // 0x0087a480, stride 0x200, unit handle at +0x34
extern data_array *object_headers; // 0x008603b0, stride 0x0c, object data pointer at +0x08

// hs_object_record: defined in types/hs.h (foreign-module slice; was a local TYPES-GAP copy)

static hs_object_record *hs_object_record_get(datum_index object_index)
{
    return *(hs_object_record **)((uint8_t *)object_headers->data +
        (object_index & 0xffff) * 0x0c + 8);
}

// Phase 1: for every live player whose unit is nested somewhere under another object (its parent
// chain does not lead back to itself... more precisely, walking up via `parent` repeatedly lands
// on something other than the unit itself), resets that attachment via unit_detach_from_seat.
// Phase 2: for every top-level object (no parent) that fails hs_object_hierarchy_test, either
// deletes it (object_delete_unparented, when object+4 == 0) or, if object+4 == 3, resets it via object_delete_recursive;
// any other object+4 value is left untouched.
void hs_object_runtime_cleanup(void)
{
    data_iterator player_iter;
    void *player_element;
    datum_index unit;
    datum_index walk;
    datum_index top;
    hs_object_iterator_state object_iter;
    void *object_element;
    hs_object_record *object;
    datum_index object_index;

    player_iter.data = players;
    player_iter.next_index = 0;
    player_iter.index = (datum_index)0xffffffff;
    player_iter.signature = (uint32_t)(uintptr_t)player_iter.data ^ k_data_iterator_signature;
    player_element = data_iterator_next(&player_iter);
    while (player_element != 0) {
        unit = *(datum_index *)((uint8_t *)player_element + 0x34);
        if (unit != k_datum_index_none) {
            top = unit;
            do {
                walk = top;
                top = hs_object_record_get(walk)->parent;
            } while (top != k_datum_index_none);
            if (walk != unit) {
                unit_detach_from_seat(unit, 0, 1, 1);
            }
        }
        player_element = data_iterator_next(&player_iter);
    }

    object_iter.type_filter = -1;
    object_iter.next_index = 0;
    object_iter.index = (datum_index)0xffffffff;
    object_element = object_iterator_next(&object_iter);
    for (;;) {
        if (object_element == 0) {
            return;
        }
        object_index = object_iter.index;
        if (((hs_object_record *)object_element)->parent == k_datum_index_none &&
            hs_object_hierarchy_test(object_index) == 0) {
            object = hs_object_record_get(object_index);
            // role 0 unparents (EDI object) and then deletes like role 3 (the binary falls through)
            if (object->unknown_04 == 0) {
                object_delete_unparented(object_index);
                object_delete_recursive(object_index, 0);
            } else if (object->unknown_04 == 3) {
                object_delete_recursive(object_index, 0);
            }
        }
        object_element = object_iterator_next(&object_iter);
    }
}

#if 0
Original Ghidra decompilation (0x487dd0):

void FUN_00487dd0(void)

{
  uint uVar1;
  uint uVar2;
  char cVar3;
  int iVar4;
  uint uVar5;
  uint local_10;
  ushort local_c;
  undefined2 local_a;
  uint local_8;
  uint local_4;

  local_10 = DAT_0087a480;
  local_4 = DAT_0087a480 ^ 0x69746572;
  local_c = 0;
  local_8 = 0xffffffff;
  iVar4 = data_iterator_next();
  while (iVar4 != 0) {
    uVar1 = *(uint *)(iVar4 + 0x34);
    if (uVar1 != 0xffffffff) {
      uVar2 = uVar1;
      do {
        uVar5 = uVar2;
        uVar2 = *(uint *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar5 & 0xffff) * 0xc) +
                         0x11c);
      } while (uVar2 != 0xffffffff);
      if (uVar5 != uVar1) {
        FUN_0056c640(uVar1,0,1,1);
      }
    }
    iVar4 = data_iterator_next();
  }
  local_4 = 0x86868686;
  local_10 = 0xffffffff;
  local_c = local_c & 0xff00;
  local_a = 0;
  local_8 = 0xffffffff;
  iVar4 = object_iterator_next(&local_10);
  uVar1 = local_8;
  do {
    if (iVar4 == 0) {
      return;
    }
    local_8 = uVar1;
    if ((*(int *)(iVar4 + 0x11c) == -1) && (cVar3 = FUN_00487c10(uVar1), cVar3 == '\0')) {
      iVar4 = *(int *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar1 & 0xffff) * 0xc) + 4);
      if (iVar4 == 0) {
        FUN_004f5aa0();
      }
      else if (iVar4 != 3) goto LAB_00487ecf;
      FUN_004f59d0(uVar1,0);
    }
LAB_00487ecf:
    iVar4 = object_iterator_next(&local_10);
    uVar1 = local_8;
  } while( true );
}
#endif
