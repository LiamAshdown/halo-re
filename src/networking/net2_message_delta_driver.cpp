/**
 * @file src/networking/net2_message_delta_driver.cpp
 * Message encode/decode drivers and field binding lifecycle.
 */
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "halo/networking/net2_message_delta_driver.hpp"

extern "C" {
extern message_delta_definition * message_delta_definitions[56];
extern uint8_t message_delta_field_changed_flags[0x40];
extern uint32_t bit_stream_read_bit(uint8_t *out_bit, bit_stream *stream);
extern uint8_t message_delta_item_count_bits[];
extern uint8_t message_delta_parameters_enabled;
extern int32_t message_delta_parameters_protocol_sequence;
extern int32_t bit_stream_read_bits_chunked(int32_t total_bit_count, uint32_t *buffer, bit_stream *stream);
extern uint8_t bit_stream_write_bit(int32_t bit_value, bit_stream *stream);
extern int32_t bit_stream_write_bits_chunked(bit_stream *stream, const uint32_t *values, int32_t total_bit_count);
extern uint8_t message_delta_parameters_sending;
extern message_delta_field_type_vtable message_delta_field_type_table[];
extern uint8_t message_delta_unknown_table_0069a304[28][0x18];
extern void message_delta_parameters_protocol_reload_from_config_file(void);
int32_t message_delta_decode_begin(message_delta_decode_state *state, bit_stream *stream);
int32_t message_delta_decode_field_changed_flags(void **context);
int32_t message_delta_decode_message_header(bit_stream *stream, message_delta_decode_state *state);
int32_t message_delta_decode_static_fields(int32_t message_type, bit_stream *stream, int32_t offset);
void message_delta_definitions_invoke_field_bindings(void);
void message_delta_definitions_teardown_field_bindings(void);
uint8_t message_delta_encode_all_fields(uint8_t *ctx, int32_t static_base, int32_t item, int32_t type_base);
uint8_t message_delta_encode_field(int32_t changed_offset, uint8_t *ctx, int32_t field_index, int32_t type_offset);
int32_t message_delta_encode_message(int32_t extra_eax, int32_t extra_edx, int32_t flag, int32_t message_type,
                                      int32_t changed_offset, void **items, int32_t type_offset, int32_t count,
                                      char force_changed);
uint8_t message_delta_encode_message_header(uint8_t *ctx);
uint8_t message_delta_encode_prepare_item(uint8_t *ctx);
int32_t message_delta_encode_single_value(int32_t message_type, int32_t value, int32_t type_value, char force_changed);
void message_delta_field_bindings_invoke(message_delta_static_fields *list);
uint8_t message_delta_field_bindings_lazy_init(message_delta_static_fields *list);
void message_delta_field_bindings_teardown(message_delta_static_fields *list);
void message_delta_field_layout_compute_size(message_delta_definition *definition);
void message_delta_protocol_initialize(void);
}

typedef int32_t (*message_delta_field_decode_fn)(void *field_type, int32_t changed, int32_t offset, bit_stream *stream);
typedef int32_t (*message_delta_field_encode_fn)(void *field_type, int32_t changed, int32_t offset, void *stream_or_ctx);

namespace halo::networking {

int32_t DeltaMessageDriver::decode_begin(message_delta_decode_state *state, bit_stream *stream)
{
    int32_t initial_offset;
    int32_t header_bits;
    uint32_t target;

    initial_offset = (int32_t)(stream->bit_cursor + stream->byte_cursor * 8) - (int32_t)stream->first_bit;
    header_bits = message_delta_decode_message_header(stream, state);
    if (0 < header_bits) {
        state->bits_read = header_bits;
        state->start_bit_offset = initial_offset;
        state->stream = stream;
        state->processed_count = 1;
        return 1;
    }
    state->incremental = 0;
    target = (uint32_t)stream->first_bit + (uint32_t)initial_offset;
    if ((initial_offset >= 0 || target <= stream->first_bit) &&
        (initial_offset <= 0 || stream->first_bit <= target) &&
        ((stream->first_bit <= target && target <= stream->last_bit) || target == stream->last_bit + 1)) {
        stream->bit_cursor = target & 7;
        stream->byte_cursor = target >> 3;
    }
    return 0;
}

int32_t DeltaMessageDriver::decode_field_changed_flags(void **context)
{
    message_delta_decode_state *state;
    message_delta_definition *definition;
    bit_stream *stream;
    int32_t field_count;
    uint8_t *changed_flags;
    int32_t bits_consumed;
    uint8_t ok;
    int32_t i;

    state = (message_delta_decode_state *)context[0];
    definition = message_delta_definitions[state->message_type];
    field_count = definition->field_count;
    stream = (bit_stream *)state->stream;
    changed_flags = (uint8_t *)context + 4;
    bits_consumed = 0;

    if (state->incremental == 0) {
        if (definition->statics->count < 1) {
            ok = 1;
        } else {
            uint32_t last = (uint32_t)(stream->bit_cursor + stream->byte_cursor * 8) - 1 +
                             definition->statics->size_bits;
            ok = (uint8_t)!(last < stream->first_bit || stream->last_bit < last);
        }
        if (ok) {
            for (i = 0; i < field_count; i++) {
                changed_flags[i] = 1;
            }
        }
    } else {
        uint32_t last = (uint32_t)(stream->bit_cursor + stream->byte_cursor * 8) - 1 +
                         definition->header_and_static_bits;
        ok = (uint8_t)!(last < stream->first_bit || stream->last_bit < last);
        if (ok) {
            for (i = 0; i < field_count; i++) {
                if (bit_stream_read_bit(&changed_flags[i], stream) != 1) {
                    ok = 0;
                    break;
                }
                bits_consumed = bits_consumed + 1;
            }
        }
    }

    if (ok && 0 < definition->statics->count) {
        int32_t static_bits = message_delta_decode_static_fields(
            state->message_type, stream, (int32_t)(int32_t)context[0x11]);
        if (static_bits < 1) {
            ok = 0;
        } else {
            bits_consumed = bits_consumed + static_bits;
        }
    }

    for (i = field_count; i < 0x40; i++) {
        changed_flags[i] = 0;
    }
    if (!ok) {
        bits_consumed = 0;
    }
    for (i = 0; i < 0x40; i++) {
        message_delta_field_changed_flags[i] = changed_flags[i];
    }
    return bits_consumed;
}

int32_t DeltaMessageDriver::decode_message_header(bit_stream *stream, message_delta_decode_state *state)
{
    uint32_t position;
    int32_t header_bits;
    uint8_t incremental_read_ok;
    uint8_t message_type_read_ok;
    uint8_t sequence_bit;
    uint8_t item_count_ok;
    int32_t sequence_value;
    uint8_t *raw_state = (uint8_t *)state;

    header_bits = 7;
    if (message_delta_parameters_enabled == 1) {
        header_bits = 10;
    }
    position = stream->bit_cursor + stream->byte_cursor * 8 + 3 + header_bits;
    if (position < stream->first_bit || stream->last_bit < position) {
        return 0;
    }

    state->incremental = 0;
    state->message_type = 0;
    state->item_count = 0;
    sequence_bit = 0;
    sequence_value = 0;

    incremental_read_ok = (uint8_t)bit_stream_read_bit((uint8_t *)state, stream);
    *(int32_t *)(raw_state + 0x2c) = 1;
    message_type_read_ok = incremental_read_ok != 0 && state->incremental >= 0 && state->incremental < 2;

    message_type_read_ok = (uint8_t)(bit_stream_read_bits_chunked(6, (uint32_t *)&state->message_type, stream) != 0) && message_type_read_ok;
    header_bits = 7;
    *(int32_t *)(raw_state + 0x28) = 6;

    {
        uint8_t range_ok = state->message_type >= 0 && state->message_type <= 0x37 && message_type_read_ok;

        if (message_delta_parameters_enabled == 1) {
            incremental_read_ok = (uint8_t)bit_stream_read_bit(&sequence_bit, stream);
            message_type_read_ok = incremental_read_ok != 0 && range_ok;
            incremental_read_ok = (uint8_t)(bit_stream_read_bits_chunked(2, (uint32_t *)&sequence_value, stream) != 0);
            range_ok = incremental_read_ok != 0 && message_type_read_ok;
            header_bits = 10;
            *(int32_t *)(raw_state + 0x24) = 3;
        } else {
            *(int32_t *)(raw_state + 0x24) = 0;
        }

        if (range_ok) {
            message_delta_definition *definition = message_delta_definitions[state->message_type];
            int32_t maximum_items = definition->maximum_items;
            if (maximum_items < 2) {
                state->item_count = 1;
                *(int32_t *)(raw_state + 0x20) = 0;
            } else {
                int32_t item_bits = message_delta_item_count_bits[maximum_items];
                item_count_ok = (uint8_t)(bit_stream_read_bits_chunked(item_bits, (uint32_t *)&state->item_count, stream) != 0);
                header_bits += item_bits;
                state->item_count += 1;
                *(int32_t *)(raw_state + 0x20) = item_bits;
                range_ok = state->item_count >= 1 && state->item_count <= maximum_items && item_count_ok;
            }
        }

        if ((message_delta_parameters_enabled != 1 || sequence_value == message_delta_parameters_protocol_sequence ||
             sequence_bit != 0) && range_ok) {
            return header_bits;
        }
    }
    return 0;
}

int32_t DeltaMessageDriver::decode_static_fields(int32_t message_type, bit_stream *stream, int32_t offset)
{
    message_delta_definition *definition;
    message_delta_static_fields *statics;
    int32_t total_bits;
    int32_t i;
    message_delta_field_binding *binding;
    int32_t field_bits;

    definition = message_delta_definitions[message_type];
    statics = definition->statics;
    total_bits = 0;
    if (0 < statics->count) {
        for (i = 0; i < statics->count; i++) {
            message_delta_field_decode_fn decode;

            binding = &statics->fields[i];
            decode = *(message_delta_field_decode_fn *)((uint8_t *)binding->field_type + 0x54);
            field_bits = decode(binding->field_type, 0, binding->destination_offset + offset, stream);
            if (field_bits < 1) {
                return 0;
            }
            total_bits = total_bits + field_bits;
        }
    }
    return total_bits;
}

void DeltaMessageDriver::definitions_invoke_field_bindings(void)
{
    int32_t i;
    message_delta_definition *definition;

    for (i = 0; i < k_network_message_definition_count; i++) {
        definition = message_delta_definitions[i];
        message_delta_field_bindings_invoke(definition->statics);
        message_delta_field_bindings_invoke((message_delta_static_fields *)&definition->field_count);
    }
}

void DeltaMessageDriver::definitions_teardown_field_bindings(void)
{
    int32_t i;
    message_delta_definition *definition;

    for (i = 0; i < k_network_message_definition_count; i++) {
        definition = message_delta_definitions[i];
        message_delta_field_bindings_teardown(definition->statics);
        message_delta_field_bindings_teardown((message_delta_static_fields *)&definition->field_count);
        definition->initialized = 0;
    }
}

uint8_t DeltaMessageDriver::encode_all_fields(uint8_t *ctx, int32_t static_base, int32_t item, int32_t type_base)
{
    #define CTXD(off) (*(int32_t *)(ctx + (off)))
    message_delta_definition *definition = message_delta_definitions[CTXD(4)];
    message_delta_static_fields *statics = definition->statics;
    int32_t field_count;
    int32_t i;
    uint8_t ok;

    if (0 < statics->count) {
        int32_t count = statics->count;
        ok = 1;
        for (i = 0; i < count; i++) {
            message_delta_field_binding *binding =
                &message_delta_definitions[CTXD(4)]->statics->fields[i];
            message_delta_field_encode_fn encode =
                *(message_delta_field_encode_fn *)((uint8_t *)binding->field_type + 0x50);
            int32_t field_bits = encode(binding->field_type, 0, static_base + binding->destination_offset, ctx + 0x64);
            if (field_bits > 0) {
                CTXD(0x40) = CTXD(0x40) + field_bits;
                ok = ok ? 1 : 0;
            } else {
                ok = 0;
            }
        }
        if (!ok) {
            return 0;
        }
    }

    field_count = definition->field_count;
    for (i = 0; i < 0x10; i++) {
        ((int32_t *)message_delta_field_changed_flags)[i] = 0;
    }
    ok = (uint8_t)(CTXD(8) != 1);
    for (i = 0; i < field_count; i++) {
        uint8_t changed = message_delta_encode_field(type_base, ctx, i, item);
        if (CTXD(8) == 1) {
            ok = (ok || changed) ? 1 : 0;
        } else {
            ok = (ok && changed) ? 1 : 0;
        }
    }
    return ok;
    #undef CTXD
}

uint8_t DeltaMessageDriver::encode_field(int32_t changed_offset, uint8_t *ctx, int32_t field_index, int32_t type_offset)
{
    #define CTXD(off) (*(int32_t *)(ctx + (off)))
    message_delta_definition *definition;
    message_delta_field_binding *binding;
    int32_t src_offset;
    int32_t field_bits;
    uint8_t changed;

    definition = message_delta_definitions[CTXD(4)];
    binding = &definition->fields[field_index];
    src_offset = (changed_offset == 0) ? 0 : (binding->source_offset + changed_offset);
    {
        message_delta_field_encode_fn encode =
            *(message_delta_field_encode_fn *)((uint8_t *)binding->field_type + 0x50);
        field_bits = encode(binding->field_type, src_offset, binding->destination_offset + type_offset, ctx + 100);
    }

    changed = 0;
    if (CTXD(8) == 1) {
        if (bit_stream_write_bit(field_bits != 0, 0) != 0) {
            changed = 1;
        }
    } else if (0 < field_bits) {
        changed = 1;
    }
    CTXD(0x44) = CTXD(0x44) + field_bits;
    message_delta_field_changed_flags[field_index] = (uint8_t)(field_bits != 0);
    return changed;
    #undef CTXD
}

int32_t DeltaMessageDriver::encode_message(int32_t extra_eax, int32_t extra_edx, int32_t flag, int32_t message_type,
                                      int32_t changed_offset, void **items, int32_t type_offset, int32_t count,
                                      char force_changed)
{
    uint32_t ctx_storage[0x98 / 4];
    uint8_t *ctx = (uint8_t *)ctx_storage;
    #define CTXD(off) (*(int32_t *)(ctx + (off)))
    message_delta_definition *definition;
    int32_t header_bits;
    int32_t i;

    for (i = 0; i < 0x98 / 4; i++) {
        ctx_storage[i] = 0;
    }
    header_bits = message_delta_definitions[message_type]->header_bits;
    CTXD(4) = message_type;
    CTXD(8) = flag;
    CTXD(0xc) = extra_eax;
    CTXD(0x10) = extra_edx;
    CTXD(0x18) = extra_edx - header_bits;
    CTXD(0x20) = extra_eax;
    CTXD(0x30) = header_bits - 1;
    CTXD(0x34) = header_bits;
    CTXD(0x3c) = header_bits;
    ctx[0] = 1;
    message_delta_encode_message_header(ctx);

    if (0 < count) {
        void **cursor = items;
        int32_t remaining = count;
        do {
            int32_t baseline = (changed_offset == 0) ? 0
                : *(int32_t *)((uint8_t *)cursor + (changed_offset - (int32_t)items));
            void *item = *cursor;
            int32_t type = (flag == 0) ? 0
                : *(int32_t *)((uint8_t *)cursor + (type_offset - (int32_t)items));

            message_delta_encode_prepare_item(ctx);
            message_delta_encode_all_fields(ctx, baseline, (int32_t)item, type);
            if (0 < CTXD(0x44) || force_changed != 0) {
                int32_t bits = CTXD(0x44) + CTXD(0x40);
                CTXD(0x14) = CTXD(0x14) + bits;
                CTXD(0x18) = CTXD(0x18) - bits;
                CTXD(0x3c) = CTXD(0x3c) + bits;
                CTXD(0x38) = CTXD(0x38) + 1;
            }
            if (CTXD(8) == 1) {
                CTXD(0x48) = -1;
                CTXD(0x4c) = 0; CTXD(0x50) = 0; CTXD(0x54) = 0; CTXD(0x58) = 0; CTXD(0x5c) = 0; CTXD(0x60) = 0;
            }
            CTXD(0x64) = -1;
            CTXD(0x68) = 0; CTXD(0x6c) = 0; CTXD(0x70) = 0; CTXD(0x74) = 0; CTXD(0x78) = 0; CTXD(0x7c) = 0;
            cursor = cursor + 1;
            remaining = remaining - 1;
        } while (remaining != 0);
    }

    definition = message_delta_definitions[CTXD(4)];
    if (CTXD(0x14) <= 0) {
        return 0;
    }
    if (1 < definition->maximum_items) {
        int32_t count_bits = message_delta_item_count_bits[definition->maximum_items];
        uint32_t value = (uint32_t)(CTXD(0x38) - 1);
        bit_stream_write_bits_chunked((bit_stream *)(ctx + 0x1c), &value, count_bits);
        CTXD(0x88) = count_bits;
    }
    return definition->header_bits + CTXD(0x14);
    #undef CTXD
}

uint8_t DeltaMessageDriver::encode_message_header(uint8_t *ctx)
{
    uint8_t (*const bit_stream_write_bit)(uint8_t bit, bit_stream *stream) = reinterpret_cast<uint8_t (*)(uint8_t bit, bit_stream *stream)>(&::bit_stream_write_bit);
    #define CTXD(off) (*(int32_t *)(ctx + (off)))
    bit_stream *stream = (bit_stream *)(ctx + 0x1c);
    uint32_t parameters = (uint32_t)message_delta_parameters_protocol_sequence;
    uint8_t ok;
    int32_t written;

    ok = bit_stream_write_bit((uint8_t)CTXD(8), stream) != 0;
    CTXD(0x80) = 1;
    CTXD(0x84) = CTXD(0x84) + 6;
    written = bit_stream_write_bits_chunked(stream, (const uint32_t *)(ctx + 4), CTXD(0x84));
    ok = (written != 0 && ok) ? 1 : 0;
    if (message_delta_parameters_enabled != 1) {
        return ok;
    }
    CTXD(0x88) = 2;
    ok = (bit_stream_write_bit(message_delta_parameters_sending, stream) != 0 && ok) ? 1 : 0;
    written = bit_stream_write_bits_chunked(stream, &parameters, CTXD(0x88));
    CTXD(0x88) = CTXD(0x88) + 1;
    return (written != 0 && ok) ? 1 : 0;
    #undef CTXD
}

uint8_t DeltaMessageDriver::encode_prepare_item(uint8_t *ctx)
{
    #define CTXW(off) (*(int32_t *)(ctx + (off)))

    CTXW(0x40) = 0;
    CTXW(0x44) = 0;
    if (CTXW(8) == 1) {
        uint32_t bit_offset = (uint32_t)CTXW(0x3c);
        int32_t field_bits = message_delta_definitions[CTXW(4)]->field_bits;
        CTXW(0x58) = bit_offset & 7;
        CTXW(0x50) = (int32_t)bit_offset;
        CTXW(0x5c) = (int32_t)(bit_offset - 1) + field_bits;
        CTXW(0x54) = (int32_t)(bit_offset >> 3);
        CTXW(0x48) = 0;
        CTXW(0x4c) = CTXW(0xc);
        CTXW(0x60) = field_bits;
        CTXW(0x40) = CTXW(0x40) + field_bits;
    } else {
        CTXW(0x48) = 0;
        CTXW(0x4c) = 0;
        CTXW(0x50) = 0;
        CTXW(0x54) = 0;
        CTXW(0x58) = 0;
        CTXW(0x5c) = 0;
        CTXW(0x60) = 0;
    }
    {
        uint32_t total_offset = (uint32_t)(CTXW(0x3c) + CTXW(0x40));
        int32_t remaining = CTXW(0x18) - CTXW(0x40);
        CTXW(0x74) = total_offset & 7;
        CTXW(0x64) = 0;
        CTXW(0x6c) = (int32_t)total_offset;
        CTXW(0x68) = CTXW(0xc);
        CTXW(0x70) = (int32_t)(total_offset >> 3);
        CTXW(0x78) = (remaining - 1) + (int32_t)total_offset;
        CTXW(0x7c) = remaining;
    }
    #undef CTXW
    return 1;
}

int32_t DeltaMessageDriver::encode_single_value(int32_t message_type, int32_t value, int32_t type_value, char force_changed)
{
    int32_t (*const message_delta_encode_message)(int32_t flag, int32_t message_type, int32_t changed_offset,
                                             void **items, int32_t type_offset, int32_t count,
                                             char force_changed) = reinterpret_cast<int32_t (*)(int32_t flag, int32_t message_type, int32_t changed_offset,
                                             void **items, int32_t type_offset, int32_t count,
                                             char force_changed)>(&::message_delta_encode_message);
    struct {
        int32_t value;
        int32_t type_value;
    } item;
    void *items[1];

    item.value = value;
    item.type_value = type_value;
    items[0] = &item;
    return message_delta_encode_message(1, message_type, value != 0 ? (int32_t)(int32_t)&item : 0,
                                         items, (int32_t)(int32_t)&item.type_value, 1, force_changed);
}

void DeltaMessageDriver::field_bindings_invoke(message_delta_static_fields *list)
{
    int32_t i;
    message_delta_field_binding *binding;

    if (0 < list->count) {
        binding = list->fields;
        for (i = 0; i < list->count; i++, binding++) {
            if (binding->destination_offset == 0 && binding->source_offset == 0 && binding->field_type == 0) {
                return;
            }
            message_delta_field_type_table[*(int32_t *)binding->field_type].initialize(binding->field_type);
        }
    }
}

uint8_t DeltaMessageDriver::field_bindings_lazy_init(message_delta_static_fields *list)
{
    int32_t count;
    int32_t i;
    int32_t processed;
    message_delta_field_binding *binding;
    void *field_type;
    uint8_t *binding_flag;
    uint8_t *type_flag;

    count = list->count;
    if (count >= 0x41) {
        return 0;
    }
    i = 0;
    processed = 0;
    if (0 < count) {
        binding = list->fields;
        for (; i < count; i++, binding++) {
            if (binding->destination_offset == 0 && binding->source_offset == 0 && binding->field_type == 0) {
                break;
            }
            binding_flag = &binding->initialized;
            if (*binding_flag == 0) {
                field_type = binding->field_type;
                type_flag = (uint8_t *)field_type + 0x64;
                if (*type_flag == 0) {
                    message_delta_field_type_table[*(int32_t *)field_type].initialize((message_delta_field_type *)field_type);
                    *(int32_t *)((uint8_t *)field_type + 0x5c) =
                        message_delta_field_type_table[*(int32_t *)field_type].compute_size((message_delta_field_type *)field_type);
                    *type_flag = 1;
                }
                *binding_flag = 1;
            }
            processed = processed + 1;
        }
    }
    binding = &list->fields[i];
    if (processed == count &&
        binding->destination_offset == 0 && binding->source_offset == 0 && binding->field_type == 0) {
        return 1;
    }
    return 0;
}

void DeltaMessageDriver::field_bindings_teardown(message_delta_static_fields *list)
{
    int32_t i;
    message_delta_field_binding *binding;
    uint8_t *binding_flag;
    uint8_t *type_flag;

    if (0 < list->count) {
        binding = list->fields;
        for (i = 0; i < list->count; i++, binding++) {
            binding_flag = &binding->initialized;
            if (*binding_flag == 1) {
                type_flag = (uint8_t *)binding->field_type + 0x64;
                if (*type_flag == 1) {
                    message_delta_field_type_table[*(int32_t *)binding->field_type].teardown(binding->field_type);
                    *type_flag = 0;
                }
                *binding_flag = 0;
            }
        }
    }
}

void DeltaMessageDriver::field_layout_compute_size(message_delta_definition *definition)
{
    int32_t fields_bit_sum;
    int32_t statics_bit_sum;
    int32_t i;
    int32_t header_bits;
    int32_t item_count_extra_bits;
    int32_t header_and_static_bits;
    int32_t item_bits;

    message_delta_field_bindings_lazy_init(definition->statics);
    message_delta_field_bindings_lazy_init((message_delta_static_fields *)&definition->field_count);

    fields_bit_sum = 0;
    for (i = 0; i < definition->field_count; i++) {
        fields_bit_sum += *(int32_t *)((uint8_t *)definition->fields[i].field_type + 0x5c);
    }

    statics_bit_sum = 0;
    for (i = 0; i < definition->statics->count; i++) {
        statics_bit_sum += *(int32_t *)((uint8_t *)definition->statics->fields[i].field_type + 0x5c);
    }

    header_bits = 7;
    if (message_delta_parameters_enabled == 1) {
        header_bits = 10;
    }
    item_count_extra_bits = 0;
    if (1 < definition->maximum_items) {
        item_count_extra_bits = message_delta_item_count_bits[definition->maximum_items];
    }

    header_and_static_bits = definition->field_count + statics_bit_sum;
    item_bits = header_and_static_bits + fields_bit_sum;
    header_bits = item_count_extra_bits + header_bits;

    definition->field_bits = fields_bit_sum;
    definition->statics->size_bits = statics_bit_sum;
    definition->item_bits = item_bits;
    definition->header_and_static_bits = header_and_static_bits;
    definition->header_bits = header_bits;
    definition->maximum_bits = definition->maximum_items * item_bits + header_bits;
    definition->initialized = 1;
}

void DeltaMessageDriver::protocol_initialize(void)
{
    int32_t i;

    if (message_delta_parameters_enabled == 1) {
        message_delta_parameters_protocol_reload_from_config_file();
    }
    for (i = 0; i < 28; i++) {
        message_delta_unknown_table_0069a304[i][0] = 1;
    }
    for (i = 0; i < k_network_message_definition_count; i++) {
        message_delta_field_layout_compute_size(message_delta_definitions[i]);
    }
}

}  // namespace halo::networking

extern "C" {
int32_t message_delta_decode_begin(message_delta_decode_state *state, bit_stream *stream)
{
    return halo::networking::DeltaMessageDriver::decode_begin(state, stream);
}

int32_t message_delta_decode_field_changed_flags(void **context)
{
    return halo::networking::DeltaMessageDriver::decode_field_changed_flags(context);
}

int32_t message_delta_decode_message_header(bit_stream *stream, message_delta_decode_state *state)
{
    return halo::networking::DeltaMessageDriver::decode_message_header(stream, state);
}

int32_t message_delta_decode_static_fields(int32_t message_type, bit_stream *stream, int32_t offset)
{
    return halo::networking::DeltaMessageDriver::decode_static_fields(message_type, stream, offset);
}

void message_delta_definitions_invoke_field_bindings(void)
{
    halo::networking::DeltaMessageDriver::definitions_invoke_field_bindings();
}

void message_delta_definitions_teardown_field_bindings(void)
{
    halo::networking::DeltaMessageDriver::definitions_teardown_field_bindings();
}

uint8_t message_delta_encode_all_fields(uint8_t *ctx, int32_t static_base, int32_t item, int32_t type_base)
{
    return halo::networking::DeltaMessageDriver::encode_all_fields(ctx, static_base, item, type_base);
}

uint8_t message_delta_encode_field(int32_t changed_offset, uint8_t *ctx, int32_t field_index, int32_t type_offset)
{
    return halo::networking::DeltaMessageDriver::encode_field(changed_offset, ctx, field_index, type_offset);
}

int32_t message_delta_encode_message(int32_t extra_eax, int32_t extra_edx, int32_t flag, int32_t message_type,
                                      int32_t changed_offset, void **items, int32_t type_offset, int32_t count,
                                      char force_changed)
{
    return halo::networking::DeltaMessageDriver::encode_message(extra_eax, extra_edx, flag, message_type, changed_offset, items, type_offset, count, force_changed);
}

uint8_t message_delta_encode_message_header(uint8_t *ctx)
{
    return halo::networking::DeltaMessageDriver::encode_message_header(ctx);
}

uint8_t message_delta_encode_prepare_item(uint8_t *ctx)
{
    return halo::networking::DeltaMessageDriver::encode_prepare_item(ctx);
}

int32_t message_delta_encode_single_value(int32_t message_type, int32_t value, int32_t type_value, char force_changed)
{
    return halo::networking::DeltaMessageDriver::encode_single_value(message_type, value, type_value, force_changed);
}

void message_delta_field_bindings_invoke(message_delta_static_fields *list)
{
    halo::networking::DeltaMessageDriver::field_bindings_invoke(list);
}

uint8_t message_delta_field_bindings_lazy_init(message_delta_static_fields *list)
{
    return halo::networking::DeltaMessageDriver::field_bindings_lazy_init(list);
}

void message_delta_field_bindings_teardown(message_delta_static_fields *list)
{
    halo::networking::DeltaMessageDriver::field_bindings_teardown(list);
}

void message_delta_field_layout_compute_size(message_delta_definition *definition)
{
    halo::networking::DeltaMessageDriver::field_layout_compute_size(definition);
}

void message_delta_protocol_initialize(void)
{
    halo::networking::DeltaMessageDriver::protocol_initialize();
}

}
