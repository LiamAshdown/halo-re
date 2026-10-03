#include "halo/hs/records.hpp"
#include "halo/hs/hs3_machine.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/hs/api.hpp"


namespace halo::hs::part3 {

/**
 * Evaluates `parameter_count` fixed-position arguments of the syntax node being evaluated by `thread_index`, one
 * per call, validating each child's declared type against `expected_types[index]` before pushing it. Returns the
 * completed `int32_t[parameter_count]` results buffer once every argument has been evaluated (or the next one
 * fails its type check), or NULL if there is still more to evaluate.
 *
 * @address 0x48a850
 */
int32_t *ArgumentEvaluator::typed_arguments(uint32_t thread_index, int16_t parameter_count, int16_t *expected_types, char first) const
{
    hs_thread *thread;
    hs_stack_frame *frame;
    hs_syntax_node *node;
    int32_t *results;
    int16_t *index;
    datum_index *next_node_slot;
    int32_t *done;

    thread = halo::hs::thread_at(thread_index);
    frame = thread->stack;
    results = (int32_t *)((uint8_t *)frame + 0x0e + frame->size);
    done = results;
    frame->size = frame->size + parameter_count * 4;

    frame = thread->stack;
    index = (int16_t *)((uint8_t *)frame + 0x0e + frame->size);
    frame->size = frame->size + 2;

    frame = thread->stack;
    next_node_slot = (datum_index *)((uint8_t *)frame + 0x0e + frame->size);
    frame->size = frame->size + 4;

    if (first != 0) {
        *index = 0;
        node = halo::hs::syntax_node_at(frame->syntax_node);
        *next_node_slot = (halo::hs::syntax_node_at(node->data.first_child))->next_node;
    }

    node = halo::hs::syntax_node_at(*next_node_slot);
    if (*index < parameter_count && node->type == expected_types[*index]) {
        halo::hs::hs_thread_push(*next_node_slot, thread_index, &results[*index]);

        *next_node_slot = node->next_node;
        *index = *index + 1;
        done = 0;
    }
    return done;
}

/**
 * Generic variadic argument collector, called once per evaluated child. Accumulates up to 32 values; once the
 * child list is exhausted (or the limit is hit), writes the final count and values pointer to
 * `*out_count`/`*out_values` and returns 1. Otherwise pushes the next child for evaluation and returns 0.
 *
 * @address 0x48ad60
 */
char ArgumentEvaluator::variadic_arguments(uint32_t thread_index, int32_t value, uint32_t *out_count, int32_t **out_values) const
{
    hs_thread *thread;
    hs_stack_frame *frame;
    hs_syntax_node *node;
    int32_t *evaluated_count;
    int32_t *values;
    int16_t *argument_count;
    datum_index *next_node_slot;
    int i;

    thread = halo::hs::thread_at(thread_index);
    frame = thread->stack;
    evaluated_count = (int32_t *)((uint8_t *)frame + 0x0e + frame->size);
    frame->size = frame->size + 4;

    frame = thread->stack;
    values = (int32_t *)((uint8_t *)frame + 0x0e + frame->size);
    frame->size = frame->size + 0x80;

    frame = thread->stack;
    argument_count = (int16_t *)((uint8_t *)frame + 0x0e + frame->size);
    frame->size = frame->size + 2;

    frame = thread->stack;
    next_node_slot = (datum_index *)((uint8_t *)frame + 0x0e + frame->size);
    frame->size = frame->size + 4;

    if ((char)value != 0) {
        *argument_count = 0;
        node = halo::hs::syntax_node_at(frame->syntax_node);
        *next_node_slot = (halo::hs::syntax_node_at(node->data.first_child))->next_node;
        *evaluated_count = 0;
        for (i = 0; i < 0x20; i++) {
            values[i] = 0;
        }
    }

    if (*next_node_slot != k_datum_index_none && *argument_count < 0x20) {

        halo::hs::hs_thread_push(*next_node_slot, thread_index, &value);
        node = halo::hs::syntax_node_at(*next_node_slot);
        *next_node_slot = node->next_node;
        values[*evaluated_count] = value;
        *argument_count = *argument_count + 1;
        *evaluated_count = *evaluated_count + 1;
        return 0;
    }

    *out_count = *evaluated_count;
    *out_values = values;
    return 1;
}

}
