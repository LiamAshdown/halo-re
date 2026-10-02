// qsort_dword_array_shortsort  (Ghidra: qsort_dword_array_shortsort, already named)
// address 0x4496d0, size 78 bytes
// name confidence: 0.85   rewrite confidence: 0.9  (reviewed line by line against objdump)
// evidence: out/phase4/cseries_types_notes.md "Register conventions (LTCG)" and "What the header
// defines": this is the MSVC CRT qsort short-partition fallback (a max-selection insertion sort)
// specialised to 4-byte elements and a byte-returning comparator; qsort_dword_array (0x449590)
// calls it once the current partition shrinks to k_qsort_dword_shortsort_cutoff elements or
// fewer. Confirmed against objdump -d -M intel of bin/halo.exe over 0x4496d0..0x44971e.
// register convention: last (inclusive) element pointer in EAX; first element pointer and the
// comparator on the stack, in that order (its one caller, qsort_dword_array, pops 8 bytes after
// the call).

#include "tags.h"
#include "cseries.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// blam-cc: last in EAX, then the stack parameters first, compare (EAX, stack)
// Selection-sorts the inclusive range [first, last] of 4-byte elements: each pass finds the
// largest remaining element (by compare) and swaps it into the last slot, then shrinks the
// range by one from the top. This is the small-partition fallback for qsort_dword_array; it is
// never called on a range larger than k_qsort_dword_shortsort_cutoff elements.
void qsort_dword_array_shortsort(int32_t *last, int32_t *first, qsort_dword_compare_proc compare)
{
    int32_t *max_ptr;
    int32_t *scan_ptr;
    int32_t temp;
    uint8_t cmp;

    while (first < last) {
        max_ptr = first;
        scan_ptr = first;
        while (scan_ptr = scan_ptr + 1, scan_ptr <= last) {
            cmp = compare(*scan_ptr, *max_ptr);
            if (cmp != 0) {
                max_ptr = scan_ptr;
            }
        }
        temp = *max_ptr;
        *max_ptr = *last;
        *last = temp;
        last = last - 1;
    }
}

#if 0
Original Ghidra decompilation (0x4496d0):

void qsort_dword_array_shortsort(undefined4 *param_1,code *param_2)

{
  undefined4 uVar1;
  undefined4 *puVar2;
  undefined4 *puVar3;
  char cVar4;
  undefined4 *in_EAX;

  for (; puVar2 = param_1, puVar3 = param_1, param_1 < in_EAX; in_EAX = in_EAX + -1) {
    while (puVar3 = puVar3 + 1, puVar3 <= in_EAX) {
      cVar4 = (*param_2)(*puVar3,*puVar2);
      if (cVar4 != '\0') {
        puVar2 = puVar3;
      }
    }
    uVar1 = *puVar2;
    *puVar2 = *in_EAX;
    *in_EAX = uVar1;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
