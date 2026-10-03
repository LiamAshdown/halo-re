#include "halo/cseries/cseries.hpp"

#include "crt.h"
#include "tags.h"
#include "halo/cseries/api.hpp"

namespace halo::cseries {

/**
 * Sorts count 4 byte elements in place with the engine's non-recursive quicksort: the middle element
 * becomes the pivot, the larger partition is pushed on an explicit stack, and partitions of eight
 * elements or fewer go to shortsort.
 *
 * @address 0x449590
 */
void dword_sort::sort(uint32_t count, int32_t *elements, qsort_dword_compare_proc compare)
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
            halo::cseries::dword_sort::shortsort(high, low, compare);
        } else {

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

            temp = *low;
            *low = *scan_high;
            *scan_high = temp;

            if ((scan_high - low) <= (high - scan_low)) {
                if (scan_low < high) {
                    low_stack[stack_depth] = scan_low;
                    high_stack[stack_depth] = high;
                    stack_depth = stack_depth + 1;
                }
                if (low + 1 < scan_high) {
                    high = scan_high - 1;
                    continue;
                }
            } else {
                if (low + 1 < scan_high) {
                    low_stack[stack_depth] = low;
                    high_stack[stack_depth] = scan_high - 1;
                    stack_depth = stack_depth + 1;
                }
                low = scan_low;
                if (scan_low < high) {
                    continue;
                }
            }
        }

        stack_depth = stack_depth - 1;
        if (stack_depth < 0) {
            return;
        }
        low = low_stack[stack_depth];
        high = high_stack[stack_depth];
    }
}

/**
 * Selection-sorts the inclusive range [first, last]: each pass moves the largest remaining element, by
 * compare, to the end and shrinks the range.
 *
 * @address 0x4496d0
 */
void dword_sort::shortsort(int32_t *last, int32_t *first, qsort_dword_compare_proc compare)
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

} // namespace halo::cseries
