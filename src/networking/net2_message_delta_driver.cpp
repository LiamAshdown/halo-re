/**
 * @file src/networking/net2_message_delta_driver.cpp
 * Message encode/decode drivers and field binding lifecycle.
 */
#include <string.h>
#include "tags.h"
#include "halo/networking/delta_message_types.hpp"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "halo/networking/message_delta_context.hpp"
#include "halo/networking/net2_message_delta_driver.hpp"
#include "halo/networking/field_codec.hpp"
#include "halo/memory/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/core/link.hpp"
#include "halo/networking/vars.hpp"

static auto &message_delta_definitions = halo::link::ref<message_delta_definition * [56]>(halo::networking::vars().message_delta_definitions);
static auto &message_delta_field_changed_flags = halo::link::ref<uint8_t [0x40]>(halo::networking::vars().message_delta_field_changed_flags);
static auto &message_delta_item_count_bits = halo::link::ref<uint8_t []>(halo::networking::vars().message_delta_item_count_bits);
static auto &message_delta_parameters_enabled = halo::link::ref<uint8_t>(halo::networking::vars().message_delta_parameters_enabled);
static auto &message_delta_parameters_protocol_sequence = halo::link::ref<int32_t>(halo::networking::vars().message_delta_parameters_protocol_sequence);
static auto &message_delta_parameters_sending = halo::link::ref<uint8_t>(halo::networking::vars().message_delta_parameters_sending);
static auto &message_delta_unknown_table_0069a304 = halo::link::ref<uint8_t [28][0x18]>(halo::networking::vars().message_delta_unknown_table_0069a304);

typedef int32_t (*message_delta_field_decode_fn)(void *field_type, int32_t changed, int32_t offset, bit_stream *stream);
typedef int32_t (*message_delta_field_encode_fn)(void *field_type, int32_t changed, int32_t offset, void *stream_or_ctx);

namespace halo::networking {

int32_t DeltaMessageDriver::decode_begin(message_delta_decode_state *state, bit_stream *stream)
{
    int32_t initial_offset;
    int32_t header_bits;
    uint32_t target;

    initial_offset = (int32_t)(stream->bit_cursor + stream->byte_cursor * 8) - (int32_t)stream->first_bit;
    header_bits = halo::networking::message_delta_decode_message_header(stream, state);
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

    state = halo::networking::delta_context(context)->state;
    definition = message_delta_definitions[state->message_type];
    field_count = definition->field_count;
    stream = (bit_stream *)state->stream;
    changed_flags = halo::networking::delta_context(context)->changed;
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
                if (halo::memory::bit_stream_read_bit(&changed_flags[i], stream) != 1) {
                    ok = 0;
                    break;
                }
                bits_consumed = bits_consumed + 1;
            }
        }
    }

    if (ok && 0 < definition->statics->count) {
        int32_t static_bits = halo::networking::message_delta_decode_static_fields(
            state->message_type, stream, (int32_t)(int32_t)halo::networking::delta_context(context)->target);
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

    incremental_read_ok = (uint8_t)halo::memory::bit_stream_read_bit((uint8_t *)state, stream);
    state->incremental_bits = 1;
    message_type_read_ok = incremental_read_ok != 0 && state->incremental >= 0 && state->incremental < 2;

    message_type_read_ok = (uint8_t)(halo::memory::bit_stream_read_bits_chunked(6, (uint32_t *)&state->message_type, stream) != 0) && message_type_read_ok;
    header_bits = 7;
    state->message_type_bits = 6;

    {
        uint8_t range_ok = state->message_type >= 0 && state->message_type <= 0x37 && message_type_read_ok;

        if (message_delta_parameters_enabled == 1) {
            incremental_read_ok = (uint8_t)halo::memory::bit_stream_read_bit(&sequence_bit, stream);
            message_type_read_ok = incremental_read_ok != 0 && range_ok;
            incremental_read_ok = (uint8_t)(halo::memory::bit_stream_read_bits_chunked(2, (uint32_t *)&sequence_value, stream) != 0);
            range_ok = incremental_read_ok != 0 && message_type_read_ok;
            header_bits = 10;
            state->parameter_bits = 3;
        } else {
            state->parameter_bits = 0;
        }

        if (range_ok) {
            message_delta_definition *definition = message_delta_definitions[state->message_type];
            int32_t maximum_items = definition->maximum_items;
            if (maximum_items < 2) {
                state->item_count = 1;
                state->item_count_bits = 0;
            } else {
                int32_t item_bits = message_delta_item_count_bits[maximum_items];
                item_count_ok = (uint8_t)(halo::memory::bit_stream_read_bits_chunked(item_bits, (uint32_t *)&state->item_count, stream) != 0);
                header_bits += item_bits;
                state->item_count += 1;
                state->item_count_bits = item_bits;
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
        halo::networking::message_delta_field_bindings_invoke(definition->statics);
        halo::networking::message_delta_field_bindings_invoke((message_delta_static_fields *)&definition->field_count);
    }
}

void DeltaMessageDriver::definitions_teardown_field_bindings(void)
{
    int32_t i;
    message_delta_definition *definition;

    for (i = 0; i < k_network_message_definition_count; i++) {
        definition = message_delta_definitions[i];
        halo::networking::message_delta_field_bindings_teardown(definition->statics);
        halo::networking::message_delta_field_bindings_teardown((message_delta_static_fields *)&definition->field_count);
        definition->initialized = 0;
    }
}

uint8_t DeltaMessageDriver::encode_all_fields(message_delta_encode_context *ctx, int32_t static_base, int32_t item, int32_t type_base)
{
    message_delta_definition *definition = message_delta_definitions[ctx->message_type];
    message_delta_static_fields *statics = definition->statics;
    int32_t field_count;
    int32_t i;
    uint8_t ok;

    if (0 < statics->count) {
        int32_t count = statics->count;
        ok = 1;
        for (i = 0; i < count; i++) {
            message_delta_field_binding *binding =
                &message_delta_definitions[ctx->message_type]->statics->fields[i];
            message_delta_field_encode_fn encode =
                *(message_delta_field_encode_fn *)((uint8_t *)binding->field_type + 0x50);
            int32_t field_bits = encode(binding->field_type, 0, static_base + binding->destination_offset, &ctx->item_stream);
            if (field_bits > 0) {
                ctx->static_bits = ctx->static_bits + field_bits;
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
    ok = (uint8_t)(ctx->mode != 1);
    for (i = 0; i < field_count; i++) {
        uint8_t changed = halo::networking::message_delta_encode_field(type_base, ctx, i, item);
        if (ctx->mode == 1) {
            ok = (ok || changed) ? 1 : 0;
        } else {
            ok = (ok && changed) ? 1 : 0;
        }
    }
    return ok;
}

uint8_t DeltaMessageDriver::encode_field(int32_t changed_offset, message_delta_encode_context *ctx, int32_t field_index, int32_t type_offset)
{
    message_delta_definition *definition;
    message_delta_field_binding *binding;
    int32_t src_offset;
    int32_t field_bits;
    uint8_t changed;

    definition = message_delta_definitions[ctx->message_type];
    binding = &definition->fields[field_index];
    src_offset = (changed_offset == 0) ? 0 : (binding->source_offset + changed_offset);
    {
        message_delta_field_encode_fn encode =
            *(message_delta_field_encode_fn *)((uint8_t *)binding->field_type + 0x50);
        field_bits = encode(binding->field_type, src_offset, binding->destination_offset + type_offset, &ctx->item_stream);
    }

    changed = 0;
    if (ctx->mode == 1) {
        if (halo::memory::bit_stream_write_bit(field_bits != 0, 0) != 0) {
            changed = 1;
        }
    } else if (0 < field_bits) {
        changed = 1;
    }
    ctx->field_bits = ctx->field_bits + field_bits;
    message_delta_field_changed_flags[field_index] = (uint8_t)(field_bits != 0);
    return changed;
}

int32_t DeltaMessageDriver::encode_message(int32_t extra_eax, int32_t extra_edx, int32_t flag, int32_t message_type,
                                      int32_t changed_offset, void **items, int32_t type_offset, int32_t count,
                                      char force_changed)
{
    message_delta_encode_context context;
    message_delta_encode_context *ctx = &context;
    message_delta_definition *definition;
    int32_t header_bits;
    int32_t i;

    memset(ctx, 0, sizeof(*ctx));
    header_bits = message_delta_definitions[message_type]->header_bits;
    ctx->message_type = message_type;
    ctx->mode = flag;
    ctx->buffer = reinterpret_cast<uint8_t *>(static_cast<uintptr_t>(extra_eax));
    ctx->buffer_bits = extra_edx;
    ctx->remaining_bits = extra_edx - header_bits;
    ctx->stream.data = ctx->buffer;
    ctx->stream.last_bit = header_bits - 1;
    ctx->header_bits = header_bits;
    ctx->bit_offset = header_bits;
    ctx->active = 1;
    halo::networking::message_delta_encode_message_header(ctx);

    if (0 < count) {
        void **cursor = items;
        int32_t remaining = count;
        do {
            int32_t baseline = (changed_offset == 0) ? 0
                : *(int32_t *)((uint8_t *)cursor + (changed_offset - (int32_t)items));
            void *item = *cursor;
            int32_t type = (flag == 0) ? 0
                : *(int32_t *)((uint8_t *)cursor + (type_offset - (int32_t)items));

            halo::networking::message_delta_encode_prepare_item(ctx);
            halo::networking::message_delta_encode_all_fields(ctx, baseline, (int32_t)item, type);
            if (0 < ctx->field_bits || force_changed != 0) {
                int32_t bits = ctx->field_bits + ctx->static_bits;
                ctx->item_bits = ctx->item_bits + bits;
                ctx->remaining_bits = ctx->remaining_bits - bits;
                ctx->bit_offset = ctx->bit_offset + bits;
                ctx->item_count = ctx->item_count + 1;
            }
            if (ctx->mode == 1) {
                ctx->baseline_stream.unknown_00 = -1;
                ctx->baseline_stream.data = 0; ctx->baseline_stream.first_bit = 0; ctx->baseline_stream.byte_cursor = 0; ctx->baseline_stream.bit_cursor = 0; ctx->baseline_stream.last_bit = 0; ctx->baseline_bits = 0;
            }
            ctx->item_stream.unknown_00 = -1;
            ctx->item_stream.data = 0; ctx->item_stream.first_bit = 0; ctx->item_stream.byte_cursor = 0; ctx->item_stream.bit_cursor = 0; ctx->item_stream.last_bit = 0; ctx->item_stream_bits = 0;
            cursor = cursor + 1;
            remaining = remaining - 1;
        } while (remaining != 0);
    }

    definition = message_delta_definitions[ctx->message_type];
    if (ctx->item_bits <= 0) {
        return 0;
    }
    if (1 < definition->maximum_items) {
        int32_t count_bits = message_delta_item_count_bits[definition->maximum_items];
        uint32_t value = (uint32_t)(ctx->item_count - 1);
        halo::memory::bit_stream_write_bits_chunked(&ctx->stream, &value, count_bits);
        ctx->parameter_bits = count_bits;
    }
    return definition->header_bits + ctx->item_bits;
}

uint8_t DeltaMessageDriver::encode_message_header(message_delta_encode_context *ctx)
{
    bit_stream *stream = &ctx->stream;
    uint32_t parameters = (uint32_t)message_delta_parameters_protocol_sequence;
    uint8_t ok;
    int32_t written;

    ok = halo::memory::bit_stream_write_bit((uint8_t)ctx->mode, stream) != 0;
    ctx->header_written = 1;
    ctx->type_bits = ctx->type_bits + 6;
    written = halo::memory::bit_stream_write_bits_chunked(stream, reinterpret_cast<const uint32_t *>(&ctx->message_type), ctx->type_bits);
    ok = (written != 0 && ok) ? 1 : 0;
    if (message_delta_parameters_enabled != 1) {
        return ok;
    }
    ctx->parameter_bits = 2;
    ok = (halo::memory::bit_stream_write_bit(message_delta_parameters_sending, stream) != 0 && ok) ? 1 : 0;
    written = halo::memory::bit_stream_write_bits_chunked(stream, &parameters, ctx->parameter_bits);
    ctx->parameter_bits = ctx->parameter_bits + 1;
    return (written != 0 && ok) ? 1 : 0;
}

uint8_t DeltaMessageDriver::encode_prepare_item(message_delta_encode_context *ctx)
{

    ctx->static_bits = 0;
    ctx->field_bits = 0;
    if (ctx->mode == 1) {
        uint32_t bit_offset = (uint32_t)ctx->bit_offset;
        int32_t field_bits = message_delta_definitions[ctx->message_type]->field_bits;
        ctx->baseline_stream.bit_cursor = bit_offset & 7;
        ctx->baseline_stream.first_bit = (int32_t)bit_offset;
        ctx->baseline_stream.last_bit = (int32_t)(bit_offset - 1) + field_bits;
        ctx->baseline_stream.byte_cursor = (int32_t)(bit_offset >> 3);
        ctx->baseline_stream.unknown_00 = 0;
        ctx->baseline_stream.data = ctx->buffer;
        ctx->baseline_bits = field_bits;
        ctx->static_bits = ctx->static_bits + field_bits;
    } else {
        ctx->baseline_stream.unknown_00 = 0;
        ctx->baseline_stream.data = 0;
        ctx->baseline_stream.first_bit = 0;
        ctx->baseline_stream.byte_cursor = 0;
        ctx->baseline_stream.bit_cursor = 0;
        ctx->baseline_stream.last_bit = 0;
        ctx->baseline_bits = 0;
    }
    {
        uint32_t total_offset = (uint32_t)(ctx->bit_offset + ctx->static_bits);
        int32_t remaining = ctx->remaining_bits - ctx->static_bits;
        ctx->item_stream.bit_cursor = total_offset & 7;
        ctx->item_stream.unknown_00 = 0;
        ctx->item_stream.first_bit = (int32_t)total_offset;
        ctx->item_stream.data = ctx->buffer;
        ctx->item_stream.byte_cursor = (int32_t)(total_offset >> 3);
        ctx->item_stream.last_bit = (remaining - 1) + (int32_t)total_offset;
        ctx->item_stream_bits = remaining;
    }
    return 1;
}

int32_t DeltaMessageDriver::encode_single_value(int32_t message_type, void *changed_value, void *item, void *type_value,
                                                int32_t buffer, int32_t bit_budget, char force_changed)
{
    void *changed_slot = changed_value;
    void *type_slot = type_value;
    void *items[1];

    items[0] = item;
    return halo::networking::message_delta_encode_message(buffer, bit_budget, 1, message_type,
                                         changed_value != 0 ? (int32_t)(int32_t)&changed_slot : 0,
                                         items, (int32_t)(int32_t)&type_slot, 1, force_changed);
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
            FieldCodecRegistry::get(binding->field_type).initialize();
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
                    FieldCodecRegistry::get((message_delta_field_type *)field_type).initialize();
                    *(int32_t *)((uint8_t *)field_type + 0x5c) =
                        FieldCodecRegistry::get((message_delta_field_type *)field_type).compute_size();
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
                    FieldCodecRegistry::get(binding->field_type).teardown();
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

    halo::networking::message_delta_field_bindings_lazy_init(definition->statics);
    halo::networking::message_delta_field_bindings_lazy_init((message_delta_static_fields *)&definition->field_count);

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
        halo::networking::message_delta_parameters_protocol_reload_from_config_file();
    }
    for (i = 0; i < 28; i++) {
        message_delta_unknown_table_0069a304[i][0] = 1;
    }
    for (i = 0; i < k_network_message_definition_count; i++) {
        halo::networking::message_delta_field_layout_compute_size(message_delta_definitions[i]);
    }
}

}  // namespace halo::networking

namespace halo::networking {
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

uint8_t message_delta_encode_all_fields(message_delta_encode_context *ctx, int32_t static_base, int32_t item, int32_t type_base)
{
    return halo::networking::DeltaMessageDriver::encode_all_fields(ctx, static_base, item, type_base);
}

uint8_t message_delta_encode_field(int32_t changed_offset, message_delta_encode_context *ctx, int32_t field_index, int32_t type_offset)
{
    return halo::networking::DeltaMessageDriver::encode_field(changed_offset, ctx, field_index, type_offset);
}

int32_t message_delta_encode_message(int32_t extra_eax, int32_t extra_edx, int32_t flag, int32_t message_type,
                                      int32_t changed_offset, void **items, int32_t type_offset, int32_t count,
                                      char force_changed)
{
    return halo::networking::DeltaMessageDriver::encode_message(extra_eax, extra_edx, flag, message_type, changed_offset, items, type_offset, count, force_changed);
}

uint8_t message_delta_encode_message_header(message_delta_encode_context *ctx)
{
    return halo::networking::DeltaMessageDriver::encode_message_header(ctx);
}

uint8_t message_delta_encode_prepare_item(message_delta_encode_context *ctx)
{
    return halo::networking::DeltaMessageDriver::encode_prepare_item(ctx);
}

int32_t message_delta_encode_single_value(int32_t message_type, void *changed_value, void *item, void *type_value, int32_t buffer, int32_t bit_budget, char force_changed)
{
    return halo::networking::DeltaMessageDriver::encode_single_value(message_type, changed_value, item, type_value, buffer, bit_budget, force_changed);
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
