#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "objects.h"
#include "units.h"
#include "effects.h"
#include "rasterizer.h"
#include "interface.h"
#include "structures.h"
#include "cutscene.h"
#include "shaders.h"
#include "render.h"
#include <stdint.h>
#include "halo/render/render.hpp"
#include "halo/render/api.hpp"


/**
 * The inlined comparison of every function in this instantiation (see the file header).
 */
static int32_t rendered_particle_compare(const rendered_particle_datum *a, const rendered_particle_datum *b)
{
    int32_t difference = (int32_t)(int16_t)a->definition_index - (int32_t)(int16_t)b->definition_index;

    if (difference == 0) {
        difference = (int32_t)a->cluster_index - (int32_t)b->cluster_index;
        if (difference == 0) {
            difference = (int32_t)a->first_person - (int32_t)b->first_person;
        }
    }
    return difference;
}

/**
 * Swaps two rendered particle records.
 */
static void rendered_particle_swap(rendered_particle_datum *a, rendered_particle_datum *b)
{
    rendered_particle_datum temporary = *a;
    *a = *b;
    *b = temporary;
}

namespace halo::render::particle_sort {

/**
 * Re-establishes the heap property below `hole` for a heap of `bottom` elements, storing value.
 *
 * @address 0x00510a90
 */
void adjust_heap(rendered_particle_datum *first, int32_t hole, int32_t bottom, rendered_particle_datum value,
    int32_t predicate)
{
    int32_t top = hole;
    int32_t index;

    for (index = 2 * hole + 2; index < bottom; index = 2 * index + 2) {
        if (rendered_particle_compare(&first[index], &first[index - 1]) < 0) {
            index--;
        }
        first[hole] = first[index];
        hole = index;
    }
    if (index == bottom) {
        first[hole] = first[bottom - 1];
        hole = bottom - 1;
    }
    halo::render::sort_push_heap(first, hole, top, value, predicate);
}

/**
 * Turns the heap [first, last) into a sorted range.
 *
 * @address 0x005107e0
 */
void heap_sort(rendered_particle_datum *first, rendered_particle_datum *last, int32_t predicate)
{
    for (; last - first > 1; last--) {
        rendered_particle_datum value = last[-1];
        last[-1] = *first;
        halo::render::sort_adjust_heap(first, 0, (int32_t)(last - 1 - first), value, predicate);
    }
}

/**
 * Sorts the short range [first, last) by insertion.
 *
 * @address 0x00510830
 */
void insertion_sort(rendered_particle_datum *first, rendered_particle_datum *last, int32_t predicate)
{
    rendered_particle_datum *next;
    rendered_particle_datum *destination;
    rendered_particle_datum *candidate;

    if (first == last) {
        return;
    }
    for (next = first + 1; next != last; next++) {
        if (rendered_particle_compare(next, first) < 0) {
            if (first != next && next != next + 1) {
                halo::render::sort_rotate(first, next, next + 1);
            }
        } else {
            destination = next;
            for (candidate = next - 1; rendered_particle_compare(next, candidate) < 0; candidate--) {
                destination = candidate;
            }
            if (destination != next && next != next + 1) {
                halo::render::sort_rotate(destination, next, next + 1);
            }
        }
    }
}

/**
 * Sorts [first, last) by rendered_particle_compare. `ideal` is the remaining quicksort division
 * budget (the caller passes the element count).
 *
 * @address 0x00510410
 */
void introsort_loop(rendered_particle_datum *first, rendered_particle_datum *last, int32_t ideal, int32_t predicate)
{
    int32_t count;
    rendered_particle_range mid;

    for (count = (int32_t)(last - first); count > 32 && ideal > 0; count = (int32_t)(last - first)) {
        halo::render::sort_unguarded_partition(&mid, first, last, predicate);
        ideal = ideal / 2;
        ideal += ideal / 2;

        if (mid.first - first < last - mid.second) {
            halo::render::sort_introsort_loop(first, mid.first, ideal, predicate);
            first = mid.second;
        } else {
            halo::render::sort_introsort_loop(mid.second, last, ideal, predicate);
            last = mid.first;
        }
    }

    if (count > 32) {
        if (last - first > 1) {
            halo::render::sort_make_heap(first, last, predicate);
        }
        halo::render::sort_heap_sort(first, last, predicate);
    } else if (count > 1) {
        halo::render::sort_insertion_sort(first, last, predicate);
    }
}

/**
 * Arranges [first, last) into a heap.
 *
 * @address 0x00510980
 */
void make_heap(rendered_particle_datum *first, rendered_particle_datum *last, int32_t predicate)
{
    int32_t bottom = (int32_t)(last - first);
    int32_t hole;

    for (hole = bottom / 2; hole > 0; ) {
        hole--;
        halo::render::sort_adjust_heap(first, hole, bottom, first[hole], predicate);
    }
}

/**
 * Orders first, mid and last (a ninther of nine samples for long ranges) so that *mid is the median.
 *
 * @address 0x005108f0
 */
void median(rendered_particle_datum *first, rendered_particle_datum *mid, rendered_particle_datum *last,
    int32_t predicate)
{
    int32_t count = (int32_t)(last - first);

    if (count > 40) {
        int32_t step = (count + 1) / 8;

        halo::render::sort_median_of_three(first, first + step, first + 2 * step, predicate);
        halo::render::sort_median_of_three(mid - step, mid, mid + step, predicate);
        halo::render::sort_median_of_three(last - 2 * step, last - step, last, predicate);
        halo::render::sort_median_of_three(first + step, mid, last - step, predicate);
    } else {
        halo::render::sort_median_of_three(first, mid, last, predicate);
    }
}

/**
 * Sorts the three elements *first, *mid, *last.
 *
 * @address 0x005109d0
 */
void median_of_three(rendered_particle_datum *first, rendered_particle_datum *mid, rendered_particle_datum *last,
    int32_t predicate)
{
    if (rendered_particle_compare(mid, first) < 0) {
        rendered_particle_swap(mid, first);
    }
    if (rendered_particle_compare(last, mid) < 0) {
        rendered_particle_swap(last, mid);
    }
    if (rendered_particle_compare(mid, first) < 0) {
        rendered_particle_swap(mid, first);
    }
}

/**
 * Moves `value` up from `hole` (no higher than `top`) to its heap position.
 *
 * @address 0x00510b20
 */
void push_heap(rendered_particle_datum *first, int32_t hole, int32_t top, rendered_particle_datum value,
    int32_t predicate)
{
    int32_t index;

    for (index = (hole - 1) / 2; top < hole && rendered_particle_compare(&first[index], &value) < 0;
        index = (hole - 1) / 2) {
        first[hole] = first[index];
        hole = index;
    }
    first[hole] = value;
}

/**
 * Rotates [first, last) so that *mid becomes the first element.
 *
 * @address 0x00510ba0
 */
void rotate(rendered_particle_datum *first, rendered_particle_datum *mid, rendered_particle_datum *last)
{
    int32_t shift = (int32_t)(mid - first);
    int32_t count = (int32_t)(last - first);
    int32_t factor;

    for (factor = shift; factor != 0; ) {
        int32_t remainder = count % factor;
        count = factor;
        factor = remainder;
    }

    if (count < last - first) {
        for (; count > 0; count--) {
            rendered_particle_datum *hole = first + count;
            rendered_particle_datum *next = hole;
            rendered_particle_datum hole_value = *hole;
            rendered_particle_datum *next1 = (next + shift == last) ? first : next + shift;

            while (next1 != hole) {
                *next = *next1;
                next = next1;
                next1 = (shift < last - next1) ? next1 + shift : first + (shift - (last - next1));
            }
            *next = hole_value;
        }
    }
}

/**
 * Partitions [first, last) around a median pivot and returns the range of elements equal to it.
 *
 * @address 0x00510500
 */
rendered_particle_range *unguarded_partition(rendered_particle_range *result, rendered_particle_datum *first,
    rendered_particle_datum *last, int32_t predicate)
{
    rendered_particle_datum *mid = first + (last - first) / 2;
    rendered_particle_datum *pfirst;
    rendered_particle_datum *plast;
    rendered_particle_datum *gfirst;
    rendered_particle_datum *glast;

    halo::render::sort_median(first, mid, last - 1, predicate);
    pfirst = mid;
    plast = pfirst + 1;

    while (first < pfirst &&
        !(rendered_particle_compare(pfirst - 1, pfirst) < 0) &&
        !(rendered_particle_compare(pfirst, pfirst - 1) < 0)) {
        pfirst--;
    }
    while (plast < last &&
        !(rendered_particle_compare(plast, pfirst) < 0) &&
        !(rendered_particle_compare(pfirst, plast) < 0)) {
        plast++;
    }

    gfirst = plast;
    glast = pfirst;
    for (;;) {
        for (; gfirst < last; gfirst++) {
            if (rendered_particle_compare(pfirst, gfirst) < 0) {
                continue;
            } else if (rendered_particle_compare(gfirst, pfirst) < 0) {
                break;
            } else {
                rendered_particle_swap(plast++, gfirst);
            }
        }
        for (; first < glast; glast--) {
            if (rendered_particle_compare(glast - 1, pfirst) < 0) {
                continue;
            } else if (rendered_particle_compare(pfirst, glast - 1) < 0) {
                break;
            } else {
                rendered_particle_swap(--pfirst, glast - 1);
            }
        }
        if (glast == first && gfirst == last) {
            result->first = pfirst;
            result->second = plast;
            return result;
        }

        if (glast == first) {
            if (plast != gfirst) {
                rendered_particle_swap(pfirst, plast);
            }
            plast++;
            rendered_particle_swap(pfirst++, gfirst++);
        } else if (gfirst == last) {
            if (--glast != --pfirst) {
                rendered_particle_swap(glast, pfirst);
            }
            rendered_particle_swap(pfirst, --plast);
        } else {
            rendered_particle_swap(gfirst++, --glast);
        }
    }
}

}  // namespace halo::render::particle_sort
