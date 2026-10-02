// dynamic_pointer_array_remove_at  (Ghidra: FUN_004ba940; named per this rewrite)
// address 0x4ba940, size 42 bytes
// name confidence: 0.35   rewrite confidence: 0.8
// evidence: out/phase4/networking_functions.md summary ("Removes the element at a given index
// from a dynamic pointer array, shifting later elements down and decrementing the count");
// shares its {data, count, capacity, pending_count} array with
// dynamic_pointer_array_find_index.c and dynamic_pointer_array_add_unique.c.
// register convention: index in EDX (in_EDX, unresolved), array pointer in ESI (unaff_ESI,
// unresolved). // blam-cc: EDX -> index, ESI -> array
// note: this function was named dynamic_pointer_array_* before the array it walks was
// pinned to the server_list global at 0x007196bc. The type it takes is therefore
// types/networking.h's server_list_globals, whose capacity/pending_count fields were
// folded in from this family's per-file TYPES-GAP typedefs during the review pass.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <string.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif


void dynamic_pointer_array_remove_at(int32_t index, server_list_globals *array)
{
    if (index < array->result_count - 1) {
        void **dst = array->list + index;

        memmove(dst, dst + 1, (uint32_t)(array->result_count - index) * 4 - 4);
    }
    array->result_count = array->result_count - 1;
}

#if 0
Original Ghidra decompilation (0x4ba940):

void FUN_004ba940(void)

{
  void *_Dst;
  int in_EDX;
  int *unaff_ESI;

  if (in_EDX < unaff_ESI[1] + -1) {
    _Dst = (void *)(*unaff_ESI + in_EDX * 4);
    _memmove(_Dst,(void *)((int)_Dst + 4),(unaff_ESI[1] - in_EDX) * 4 - 4);
  }
  unaff_ESI[1] = unaff_ESI[1] + -1;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
