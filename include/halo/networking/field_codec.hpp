/**
 * @file include/halo/networking/field_codec.hpp
 * Strategy interface over the message-delta field types.
 *
 * The engine keeps one callback row per field-type kind (compute size, initialize, teardown) in the table at
 * 0x0069a2f0 and gives every field type its own encode/decode pointers; this header is the typed face of that
 * dispatch. Include it after types/networking.h (the engine type headers are not include-guarded).
 */
#pragma once

namespace halo::networking {

/**
 * Strategy for one message-delta field type: how many bits it occupies, how it is bound, and how a value is encoded
 * to or decoded from a bit stream. `previous` is null when everything must be sent.
 *
 * Instances are cheap values created by FieldCodecRegistry and never stored in game or network state.
 */
class FieldCodec {
public:
    virtual int32_t compute_size() const = 0;
    virtual uint8_t initialize() const = 0;
    virtual void teardown() const = 0;
    virtual int32_t encode(void *previous, void *current, bit_stream *stream) const = 0;
    virtual int32_t decode(void *previous, void *current, bit_stream *stream) const = 0;

protected:
    ~FieldCodec() = default;
};

/**
 * FieldCodec backed by the engine's per-kind callback table and the field type's own encode/decode pointers. Each
 * hook calls the stored procedure directly, exactly like the original table dispatch.
 */
class TableFieldCodec final : public FieldCodec {
public:
    explicit constexpr TableFieldCodec(message_delta_field_type *field_type) : type(field_type) {}

    int32_t compute_size() const override;
    uint8_t initialize() const override;
    void teardown() const override;
    int32_t encode(void *previous, void *current, bit_stream *stream) const override;
    int32_t decode(void *previous, void *current, bit_stream *stream) const override;

private:
    message_delta_field_type *type;
};

/**
 * Resolves the FieldCodec for a field type (the kind index into the engine's field-type table) and exposes the
 * per-kind flag byte that the compound and index codecs test.
 */
class FieldCodecRegistry {
public:
    static TableFieldCodec get(message_delta_field_type *field_type);
    static uint8_t kind_flag(int32_t kind);
};

}  // namespace halo::networking
