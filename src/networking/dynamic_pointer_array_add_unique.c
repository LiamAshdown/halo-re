// dynamic_pointer_array_add_unique  (Ghidra: FUN_004ba8a0; named per this rewrite)
// address 0x4ba8a0, size 150 bytes
// name confidence: 0.35   rewrite confidence: 0.55
// evidence: out/phase4/networking_functions.md summary ("Adds a value to a dynamically-grown,
// GlobalAlloc-backed pointer array only if it is not already present, used to accumulate
// unique server-browser query results"); shares its {data, count, capacity, pending_count} array
// with dynamic_pointer_array_find_index.c and dynamic_pointer_array_remove_at.c.
// register convention: value in EDI (unaff_EDI, unresolved), array pointer in ESI (unaff_ESI,
// unresolved). // blam-cc: EDI -> value, ESI -> array
// note: this function was named dynamic_pointer_array_* before the array it walks was
// pinned to the server_list global at 0x007196bc. The type it takes is therefore
// types/networking.h's server_list_globals, whose capacity/pending_count fields were
// folded in from this family's per-file TYPES-GAP typedefs during the review pass.
// UNSURE: `server_browser_server_passes_filter` (0x4b7080) is called with an empty argument
// list in Ghidra's decompile, the same elision pattern documented throughout this codebase;
// reconstructed here as taking `value` (the same server record this function goes on to
// deduplicate and store), which is the only value in scope that makes semantic sense, and is
// itself outside this batch's address range so its own register convention is unconfirmed.
// UNSURE: both early returns are `return uVar1 & 0xffffff00;` in the original, where uVar1 is a
// live loop-index register, not a real boolean -- the same "callers only read the low byte"
// idiom documented in src/memory/bit_stream_write_bit.c. Modeled here as a clean `return 0;`
// since the actual return type is really a bool.
// UNSURE: the `if (index != -1) ... else break;` inside the dedup loop is unreachable in
// practice (index only ever counts up from 0), but is preserved verbatim for fidelity.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern void *GlobalAlloc(uint32_t flags, uint32_t bytes);
extern void *GlobalFree(void *memory); // Win32
extern void *GlobalReAlloc(void *memory, uint32_t bytes, uint32_t flags);

extern uint8_t server_browser_server_passes_filter(void *server_record); // 0x4b7080, this module


int32_t dynamic_pointer_array_add_unique(void *value, server_list_globals *array)
{
    int32_t index;

    if (!server_browser_server_passes_filter(value)) {
        return 0; // UNSURE, see file header
    }

    if (value != 0 && 0 < array->result_count) {
        void **element = array->list;

        index = 0;
        do {
            if (*element == value) {
                if (index != -1) { // UNSURE, always true in practice; see file header
                    return 0;
                }
                break;
            }
            index = index + 1;
            element = element + 1;
        } while (index < array->result_count);
    }

    if (array->result_count == array->capacity) {
        int32_t new_capacity = array->capacity + 0x20;
        uint32_t new_bytes = (uint32_t)new_capacity * 4;
        void *new_data = array->list;

        if (new_data == 0) {
            new_data = GlobalAlloc(0, new_bytes);
        } else if (new_bytes == 0) {
            GlobalFree(new_data);
            new_data = 0;
        } else {
            new_data = GlobalReAlloc(new_data, new_bytes, 2);
        }
        array->capacity = new_capacity;
        array->list = new_data;
    }

    array->list[array->result_count] = value;
    array->pending_count = array->pending_count + 1;
    array->result_count = array->result_count + 1;
    return 1;
}

#if 0
Original Ghidra decompilation (0x4ba8a0):

uint FUN_004ba8a0(void)

{
  SIZE_T dwBytes;
  uint uVar1;
  int iVar2;
  HGLOBAL pvVar3;
  int *piVar4;
  int *unaff_ESI;
  int unaff_EDI;

  uVar1 = server_browser_server_passes_filter();
  if ((char)uVar1 == '\0') {
LAB_004ba932:
    return uVar1 & 0xffffff00;
  }
  if ((unaff_EDI != 0) && (uVar1 = 0, 0 < unaff_ESI[1])) {
    piVar4 = (int *)*unaff_ESI;
    do {
      if (*piVar4 == unaff_EDI) {
        if (uVar1 != 0xffffffff) goto LAB_004ba932;
        break;
      }
      uVar1 = uVar1 + 1;
      piVar4 = piVar4 + 1;
    } while ((int)uVar1 < unaff_ESI[1]);
  }
  if (unaff_ESI[1] == unaff_ESI[2]) {
    iVar2 = unaff_ESI[2] + 0x20;
    unaff_ESI[2] = iVar2;
    dwBytes = iVar2 * 4;
    pvVar3 = (HGLOBAL)*unaff_ESI;
    if (pvVar3 == (HGLOBAL)0x0) {
      pvVar3 = GlobalAlloc(0,dwBytes);
    }
    else if (dwBytes == 0) {
      GlobalFree(pvVar3);
      pvVar3 = (HGLOBAL)0x0;
    }
    else {
      pvVar3 = GlobalReAlloc(pvVar3,dwBytes,2);
    }
    *unaff_ESI = (int)pvVar3;
  }
  *(int *)(*unaff_ESI + unaff_ESI[1] * 4) = unaff_EDI;
  iVar2 = unaff_ESI[3];
  unaff_ESI[3] = iVar2 + 1;
  unaff_ESI[1] = unaff_ESI[1] + 1;
  return CONCAT31((int3)((uint)(iVar2 + 1) >> 8),1);
}
#endif
