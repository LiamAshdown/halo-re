/**
 * @file src/networking/net2_field_codec.cpp
 * Table-backed FieldCodec and its registry.
 */
#include "message_delta_codec.h"
#include "halo/networking/field_codec.hpp"

namespace halo::networking {

/**
 * Calls the kind's compute_size procedure.
 */
int32_t TableFieldCodec::compute_size() const
{
    return message_delta_field_type_table[type->kind].compute_size(type);
}

/**
 * Calls the kind's initialize procedure (typed as returning a byte, as the engine callers read it).
 */
uint8_t TableFieldCodec::initialize() const
{
    return ((message_delta_initialize_proc)message_delta_field_type_table[type->kind].initialize)(type);
}

/**
 * Calls the kind's teardown procedure.
 */
void TableFieldCodec::teardown() const
{
    message_delta_field_type_table[type->kind].teardown(type);
}

/**
 * Calls the field type's own encode pointer.
 */
int32_t TableFieldCodec::encode(void *previous, void *current, bit_stream *stream) const
{
    return ((message_delta_codec_proc)type->encode)(type, previous, current, stream);
}

/**
 * Calls the field type's own decode pointer.
 */
int32_t TableFieldCodec::decode(void *previous, void *current, bit_stream *stream) const
{
    return ((message_delta_codec_proc)type->decode)(type, previous, current, stream);
}

/**
 * Wraps the field type in a table-backed codec.
 */
TableFieldCodec FieldCodecRegistry::get(message_delta_field_type *field_type)
{
    return TableFieldCodec(field_type);
}

/**
 * Reads the flag byte of one row of the per-kind table.
 */
uint8_t FieldCodecRegistry::kind_flag(int32_t kind)
{
    return message_delta_field_type_table[kind].kind_flag;
}

}  // namespace halo::networking
