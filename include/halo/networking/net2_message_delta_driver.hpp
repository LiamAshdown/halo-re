/**
 * @file include/halo/networking/net2_message_delta_driver.hpp
 * Message encode/decode drivers and field binding lifecycle.
 *
 * The engine type headers (types/*.h) are not include-guarded: include this header after them.
 */
#pragma once

namespace halo::networking {

/**
 * Message encode/decode drivers and field binding lifecycle.
 *
 * Static behaviour facade; the original C entry points forward to these members.
 */
class DeltaMessageDriver {
public:
    /**
     * Begins decoding a message-delta message: decodes the header into *state and, on success, seeds the rest of the decode state (bits_read, stream, processed_count) for the field-decode loop that follows. On failure, rewinds the stream to where it started and marks state as stateless (baseline).
     *
     * @address 0x4ec490
     */
    static int32_t decode_begin(message_delta_decode_state *state, bit_stream *stream);

    /**
     * Decodes the per-field "changed" flags for a message's array/compound field: on a baseline (non-incremental) message every field is simply marked changed; otherwise each flag is read bit by bit. Either way, if the definition has static fields it decodes them immediately after into context[0x11], and always leaves a full 0x40-byte changed-flags snapshot in the shared scratch buffer.
     *
     * @address 0x4ed070
     */
    static int32_t decode_field_changed_flags(void **context);

    /**
     * Decodes the header of a message-delta message: the "incremental" bit, the 6-bit message type, under protocol v2 a parameters-in-progress bit and a 2-bit rolling sequence number, and (whenever the message type allows more than one item) a variable-width item count biased by one. Returns the total header bit count on success, 0 on any decode or range failure.
     *
     * @address 0x4ece70
     */
    static int32_t decode_message_header(bit_stream *stream, message_delta_decode_state *state);

    /**
     * Decodes every unconditional (non-optional) static field of a message type into the destination at `offset`, always passing "not changed" (0) to each field's decode callback. Returns the total bits consumed, or 0 on the first field that fails to decode.
     *
     * @address 0x4ed290
     */
    static int32_t decode_static_fields(int32_t message_type, bit_stream *stream, int32_t offset);

    /**
     * Runs message_delta_field_bindings_invoke over both field-binding lists (the separately allocated statics list and the definition's own inline field list) of every registered message type.
     *
     * @address 0x4ec390
     */
    static void definitions_invoke_field_bindings(void);

    /**
     * Runs message_delta_field_bindings_teardown over both field-binding lists of every registered message type and clears each definition's initialized flag, undoing message_delta_field_layout_compute_size.
     *
     * @address 0x4ec750
     */
    static void definitions_teardown_field_bindings(void);

    /**
     * Encodes all of one item's static fields (unconditionally, via each field type's own encode callback) and then every top-level field (via message_delta_encode_field), aggregating whether any field actually changed. Returns that combined changed flag in the low byte. REWRITTEN from objdump 0x4ecc00..0x4eccf5.
     *
     * @address 0x4ecc00
     */
    static uint8_t encode_all_fields(uint8_t *ctx, int32_t static_base, int32_t item, int32_t type_base);

    /**
     * Encodes one top-level message field via its type-specific callback. For an incremental message, reports the field's changed bit through the bit stream; for a stateless message, simply reports whether it encoded any bits. Accumulates the field's bit count into the context's running total and records its changed flag into the shared scratch array.
     *
     * @address 0x4ecde0
     */
    static uint8_t encode_field(int32_t changed_offset, uint8_t *ctx, int32_t field_index, int32_t type_offset);

    /**
     * REWRITTEN from objdump 0x4ec940..0x4ecb57 (EAX = output buffer, EDX = its size in bits; stack as declared). The context (0x94 bytes, zeroed) holds: +0 "started" byte, +4 message type, +8 flag, +0xc buffer, +0x10 size, +0x14 total item bits, +0x18 remaining budget, +0x1c an inline bit_stream {0, buffer, 0, 0, 0, header_bits - 1}, +0x34 header bits, +0x38 item count, +0x3c running bit offset, then per-item blocks the helpers fill (+0x40 static bits, +0x44 field bits, +0x48..
     *
     * @address 0x4ec940
     */
    static int32_t encode_message(int32_t extra_eax, int32_t extra_edx, int32_t flag, int32_t message_type,
                                      int32_t changed_offset, void **items, int32_t type_offset, int32_t count,
                                      char force_changed);

    /**
     * Writes the leading header bit(s) of a message-delta message: whether the message is incremental, then (under protocol v2) the parameters-in-progress bit, using the encode context's own scratch counters at +0x80..+0x88 as running bit-position state. REWRITTEN from objdump 0x4ecd00..0x4ecdd1. ESI is the encoder context; its bit stream is INLINE at ctx+0x1c (the draft read a pointer at +0xc).
     *
     * @address 0x4ecd00
     */
    static uint8_t encode_message_header(uint8_t *ctx);

    /**
     * Initializes per-item bit-offset state within the message-delta encode context before encoding one item's fields: when the message is flagged, precomputes the item's static-field bit range; either way, computes the item's total (static + array) bit range against the message's remaining budget. FIXED (objdump 0x4ecb60): the context arrives in EAX and the result is AL only (mov...
     *
     * @address 0x4ecb60
     */
    static uint8_t encode_prepare_item(uint8_t *ctx);

    /**
     * Builds a message-delta message carrying a single value: the one-element items array points at `value` itself, and the field is reported "changed" (changed_offset = &value) whenever value is non-zero.
     *
     * @address 0x4ec450
     */
    static int32_t encode_single_value(int32_t message_type, int32_t value, int32_t type_value, char force_changed);

    /**
     * Walks a message type's field-binding list, calling each field type's callback (the same slot message_delta_field_bindings_lazy_init calls) with the field type pointer, until every entry has been visited or an all-zero sentinel entry is reached.
     *
     * @address 0x4ec700
     */
    static void field_bindings_invoke(message_delta_static_fields *list);

    /**
     * Lazily initializes each not-yet-initialized field type used by a message type's field-binding list (a field type's init/compute-size pair only ever runs once, cached via field_type+0x64), then marks every visited binding. Returns 1 only if the whole list was walked without hitting an early sentinel and the entry immediately after it is itself all-zero.
     *
     * @address 0x4ec840
     */
    static uint8_t field_bindings_lazy_init(message_delta_static_fields *list);

    /**
     * Tears down every field type in a message type's field-binding list that message_delta_field_bindings_lazy_init marked initialized, then clears both the per-type "initialized" byte and the per-binding flag byte this pass tests.
     *
     * @address 0x4ec900
     */
    static void field_bindings_teardown(message_delta_static_fields *list);

    /**
     * Computes and caches a message type's total encoded size (header, static fields, and array fields) for later use during encode/decode, first lazily initializing every field type the definition's two field-binding lists reference.
     *
     * @address 0x4ec790
     */
    static void field_layout_compute_size(message_delta_definition *definition);

    /**
     * One-time message-delta protocol startup: reloads parameters.cfg (when the parameters protocol is enabled), marks every entry of an unresolved 28-record table, and computes each registered message type's encoded layout size.
     *
     * @address 0x4ec2f0
     */
    static void protocol_initialize(void);

};

}  // namespace halo::networking
