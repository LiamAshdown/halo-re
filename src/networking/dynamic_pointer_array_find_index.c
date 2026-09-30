// dynamic_pointer_array_find_index  (Ghidra: FUN_004ba870; named per this rewrite)
// address 0x4ba870, size 37 bytes
// name confidence: 0.35   rewrite confidence: 0.7
// evidence: out/phase4/networking_functions.md summary ("Generic linear search returning the
// index of a value within a {pointer,count} array, or -1 if not found"); the same {data, count,
// capacity, pending_count} array is grown by dynamic_pointer_array_add_unique (0x4ba8a0) and
// shrunk by dynamic_pointer_array_remove_at (0x4ba940), which pin the field layout.
// register convention: array pointer in EDX (in_EDX, unresolved), search value in EDI
// (unaff_EDI, unresolved). // blam-cc: EDX -> array, EDI -> value
// note: this function was named dynamic_pointer_array_* before the array it walks was
// pinned to the server_list global at 0x007196bc. The type it takes is therefore
// types/networking.h's server_list_globals, whose capacity/pending_count fields were
// folded in from this family's per-file TYPES-GAP typedefs during the review pass.
// The array it walks is the server-browser query-result list: server_list.list, the pointer
// block server_list_mutex_try_lock hands out.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "fn_networking.h"


// Linear search for `value` in `array`, returning its index or -1 if it is not present (or the
// array is empty, or value is NULL).
int32_t dynamic_pointer_array_find_index(server_list_globals *array, void *value)
{
    int32_t index = -1;

    if (value != 0 && 0 < array->result_count) {
        void **element = array->list;

        index = 0;
        while (*element != value) {
            index = index + 1;
            element = element + 1;
            if (array->result_count <= index) {
                return -1;
            }
        }
    }
    return index;
}

#if 0
Original Ghidra decompilation (0x4ba870):

int FUN_004ba870(void)

{
  int iVar1;
  int iVar2;
  undefined4 *in_EDX;
  int *piVar3;
  int unaff_EDI;

  iVar1 = -1;
  iVar2 = iVar1;
  if (unaff_EDI != 0) {
    if (0 < (int)in_EDX[1]) {
      piVar3 = (int *)*in_EDX;
      iVar2 = 0;
      while (*piVar3 != unaff_EDI) {
        iVar2 = iVar2 + 1;
        piVar3 = piVar3 + 1;
        if ((int)in_EDX[1] <= iVar2) {
          return iVar1;
        }
      }
    }
  }
  return iVar2;
}
#endif
