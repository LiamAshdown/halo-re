/**
 * @file src/networking/net2_field_codec.cpp
 * Table-backed FieldCodec and its registry.
 */
#include "message_delta_codec.h"
#include "halo/networking/field_codec.hpp"
#include "halo/networking/api.hpp"
#include "halo/cseries/api.hpp"
#include <stdio.h>
#if defined(__has_feature)
#if __has_feature(address_sanitizer)
#include <sanitizer/asan_interface.h>
#define HALO_TABLE_GUARD 1
#endif
#endif

void message_delta_table_guard(bool open)
{
#if defined(HALO_TABLE_GUARD)
    uint8_t *rows = reinterpret_cast<uint8_t *>(&message_delta_field_type_table[5]);
    static bool armed;

    if (!armed) {
        armed = true;
        fprintf(stderr, "web: guarding message-delta rows 5-6 at %p (kind 5 id %d, kind 6 id %d)\n", (void *)rows,
            *reinterpret_cast<int32_t *>(rows), *reinterpret_cast<int32_t *>(rows + 0x18));
    }
    if (open) {
        ASAN_UNPOISON_MEMORY_REGION(rows, 0x30);
    } else {
        ASAN_POISON_MEMORY_REGION(rows, 0x30);
    }
#else
    (void)open;
#endif
}

namespace halo::networking {

/**
 * Calls the kind's compute_size procedure.
 */
int32_t TableFieldCodec::compute_size() const
{
    message_delta_table_guard(true);
    int32_t (*proc)(message_delta_field_type *) = message_delta_field_type_table[type->kind].compute_size;
    message_delta_table_guard(false);
    return proc(type);
}

/**
 * Calls the kind's initialize procedure (typed as returning a byte, as the engine callers read it).
 */
uint8_t TableFieldCodec::initialize() const
{
    message_delta_table_guard(true);
    message_delta_initialize_proc proc = (message_delta_initialize_proc)message_delta_field_type_table[type->kind].initialize;
    message_delta_table_guard(false);
    return proc(type);
}

/**
 * Calls the kind's teardown procedure.
 */
void TableFieldCodec::teardown() const
{
    message_delta_table_guard(true);
    void (*proc)(message_delta_field_type *) = message_delta_field_type_table[type->kind].teardown;
#if defined(__EMSCRIPTEN__)
    // Web diagnostic: a teardown once called through a bad table entry (wasm "table index is out of bounds"). Every
    // kind's teardown is function_do_nothing except kind 13's, so anything else names a corrupt field type.
    if (proc != reinterpret_cast<void (*)(message_delta_field_type *)>(&halo::cseries::function_do_nothing) &&
        proc != &halo::networking::message_delta_index_teardown) {
        fprintf(stderr, "web: bad message-delta field type %p kind=%d name=%.32s teardown=%p initialized=%d\n",
            (void *)type, type->kind, type->name, (void *)proc, type->initialized);
        static bool dumped;
        if (!dumped) {
            dumped = true;
            for (int kind = 0; kind < 28; kind++) {
                const uint32_t *entry = reinterpret_cast<const uint32_t *>(&message_delta_field_type_table[kind]);
                fprintf(stderr, "web:   kind %2d: %08x %08x %08x %08x %08x %08x\n", kind, entry[0], entry[1], entry[2],
                    entry[3], entry[4], entry[5]);
            }
        }
        proc = nullptr;
    }
#endif
    message_delta_table_guard(false);
    if (proc != nullptr) {
        proc(type);
    }
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
    message_delta_table_guard(true);
    uint8_t flag = message_delta_field_type_table[kind].kind_flag;
    message_delta_table_guard(false);
    return flag;
}

}  // namespace halo::networking
