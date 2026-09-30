// qsort_dword_array  (Ghidra: qsort_dword_array, already named)
// address 0x449590, size 302 bytes
// name confidence: 0.85   rewrite confidence: 0.9  (reviewed line by line against objdump)
// evidence: out/phase4/cseries_types_notes.md "Register conventions (LTCG)" and "What the header
// defines": this is the MSVC CRT qsort algorithm (median-of-middle pivot, Hoare-style partition,
// an explicit int32_t[k_qsort_dword_stack_depth] pending-partition stack instead of recursion,
// falling back to qsort_dword_array_shortsort under k_qsort_dword_shortsort_cutoff elements)
// specialised to 4-byte elements and a byte-returning comparator. It is Bungie's own copy, not
// the linked CRT routine (the linked qsort is elsewhere), and it stays in cseries even though
// its address range was first grouped into the cutscene batch (see out/phase4/cutscene's
// README.md "Known gaps"). Two callers, per cseries.h and the types notes: ai 0x413d0f and
// structures 0x552d0b.
// register convention: element count in EAX; elements pointer in ECX; the comparator on the
// stack (the caller pops 4 bytes after the call).

#include "crt.h"
#include "tags.h"
#include "cseries.h"
#include "fn_cseries.h"


// blam-cc: count in EAX, elements in ECX, compare on the stack
// Sorts a flat array of count 4-byte elements in place, ordering them by compare. This is the
// non-recursive MSVC CRT quicksort: it swaps the middle element of the current partition to the
// front as the pivot, partitions the rest against it, recurses (via the explicit stack below) on
// the larger half first so the stack never holds more than k_qsort_dword_stack_depth pending
// partitions, and hands any partition of k_qsort_dword_shortsort_cutoff elements or fewer to
// qsort_dword_array_shortsort.
void qsort_dword_array(uint32_t count, int32_t *elements, qsort_dword_compare_proc compare)
{
    int32_t *low;
    int32_t *high;
    int32_t *scan_low;
    int32_t *scan_high;
    uint32_t partition_size;
    uint32_t pivot_index;
    int32_t pivot;
    int32_t temp;
    uint8_t cmp;
    int32_t stack_depth;
    int32_t *low_stack[k_qsort_dword_stack_depth];
    int32_t *high_stack[k_qsort_dword_stack_depth];

    if (count < 2) {
        return;
    }

    stack_depth = 0;
    low = elements;
    high = elements + (count - 1);

    for (;;) {
        partition_size = (uint32_t)(high - low) + 1;
        if (partition_size <= k_qsort_dword_shortsort_cutoff) {
            qsort_dword_array_shortsort(high, low, compare);
        } else {
            // swap the middle element to the front to use as the pivot
            pivot_index = partition_size >> 1;
            pivot = low[pivot_index];
            low[pivot_index] = *low;
            *low = pivot;

            scan_low = low;
            scan_high = high + 1;
            for (;;) {
                do {
                    scan_low = scan_low + 1;
                } while (scan_low <= high && compare(*scan_low, *low) == 0);

                for (;;) {
                    scan_high = scan_high - 1;
                    if (scan_high <= low) {
                        break;
                    }
                    cmp = compare(*scan_high, *low);
                    if (cmp == 0) {
                        break;
                    }
                }

                if (scan_low > scan_high) {
                    break;
                }
                temp = *scan_low;
                *scan_low = *scan_high;
                *scan_high = temp;
            }

            // scan_low and scan_high have crossed: drop the pivot into its final slot
            temp = *low;
            *low = *scan_high;
            *scan_high = temp;

            // recurse (via the stack) on the larger of the two partitions, keep iterating on
            // the smaller one directly so the explicit stack stays bounded
            // 0x449651..0x44965c compares BYTE distances, (scan_high - low) * 4 - 1 against
            // (high - scan_low) * 4, signed; with both sides multiples of 4 that is exactly
            // "left element span <= right element span", written here in element units.
            if ((scan_high - low) <= (high - scan_low)) {
                if (scan_low < high) {
                    low_stack[stack_depth] = scan_low;
                    high_stack[stack_depth] = high;
                    stack_depth = stack_depth + 1;
                }
                if (low + 1 < scan_high) {
                    high = scan_high - 1; // 0x4496ae lea ebp,[edi-0x4]
                    continue;
                }
            } else {
                if (low + 1 < scan_high) {
                    low_stack[stack_depth] = low;
                    high_stack[stack_depth] = scan_high - 1; // 0x449669 add edi,-4
                    stack_depth = stack_depth + 1;
                }
                low = scan_low;
                if (scan_low < high) {
                    continue;
                }
            }
        }

        // no partition left to iterate on directly: pop the next pending one, or finish
        stack_depth = stack_depth - 1;
        if (stack_depth < 0) {
            return;
        }
        low = low_stack[stack_depth];
        high = high_stack[stack_depth];
    }
}

#if 0
Original Ghidra decompilation (0x449590):

void qsort_dword_array(code *param_1)

{
  undefined4 uVar1;
  char cVar2;
  uint in_EAX;
  uint uVar3;
  undefined4 *in_ECX;
  undefined4 *puVar4;
  undefined4 *puVar5;
  undefined4 *puVar6;
  undefined4 *puVar7;
  int local_f4;
  undefined4 auStack_f0 [30];
  undefined4 auStack_78 [30];

  if (in_EAX < 2) {
    return;
  }
  local_f4 = 0;
  puVar5 = in_ECX + (in_EAX - 1);
LAB_004495b1:
  uVar3 = ((int)puVar5 - (int)in_ECX >> 2) + 1;
  if (8 < uVar3) {
    uVar3 = uVar3 >> 1;
    uVar1 = in_ECX[uVar3];
    in_ECX[uVar3] = *in_ECX;
    *in_ECX = uVar1;
    puVar6 = puVar5 + 1;
    puVar4 = in_ECX;
LAB_00449600:
    puVar4 = puVar4 + 1;
    if (puVar4 <= puVar5) goto code_r0x00449607;
    goto LAB_00449620;
  }
  qsort_dword_array_shortsort(in_ECX,param_1);
  goto LAB_004495d1;
code_r0x00449607:
  cVar2 = (*param_1)(*puVar4,*in_ECX);
  if (cVar2 == '\0') goto LAB_00449600;
LAB_00449620:
  do {
    puVar7 = puVar6;
    puVar6 = puVar7 + -1;
    if (puVar6 <= in_ECX) break;
    cVar2 = (*param_1)(*puVar6,*in_ECX);
  } while (cVar2 != '\0');
  if (puVar4 <= puVar6) {
    uVar1 = *puVar4;
    *puVar4 = *puVar6;
    *puVar6 = uVar1;
    goto LAB_00449600;
  }
  uVar1 = *in_ECX;
  *in_ECX = *puVar6;
  *puVar6 = uVar1;
  if ((int)puVar6 + (-1 - (int)in_ECX) < (int)puVar5 - (int)puVar4) {
    if (puVar4 < puVar5) {
      auStack_f0[local_f4] = puVar4;
      auStack_78[local_f4] = puVar5;
      local_f4 = local_f4 + 1;
    }
    if (in_ECX + 1 < puVar6) {
      puVar5 = puVar7 + -2;
      goto LAB_004495b1;
    }
  }
  else {
    if (in_ECX + 1 < puVar6) {
      auStack_f0[local_f4] = in_ECX;
      auStack_78[local_f4] = puVar7 + -2;
      local_f4 = local_f4 + 1;
    }
    in_ECX = puVar4;
    if (puVar4 < puVar5) goto LAB_004495b1;
  }
LAB_004495d1:
  local_f4 = local_f4 + -1;
  if (local_f4 < 0) {
    return;
  }
  puVar5 = (undefined4 *)auStack_78[local_f4];
  in_ECX = (undefined4 *)auStack_f0[local_f4];
  goto LAB_004495b1;
}
#endif
