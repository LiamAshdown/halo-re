/**
 * @file include/halo/networking/net2_message_delta_aggregate.hpp
 * Array, structure and compound message-delta field codecs.
 *
 * The engine type headers (types/*.h) are not include-guarded: include this header after them.
 */
#pragma once

namespace halo::networking {

/**
 * Array, structure and compound message-delta field codecs.
 *
 * Static behaviour facade; the original C entry points forward to these members.
 */
class AggregateFieldCodec {
public:
    /**
     * 0x4cfb80, memory module Decodes an array-of-structures field. field_type->array_descriptor holds {count, element_size, element field type}. When previous is NULL every element is decoded unconditionally and the total bit count is returned. Otherwise the per-element "changed" bits live in a block of field_type->reserved_bits reserved at the head of the array and the element payloads follow it, so the loop alternates between the two regions with absolute seeks: for each element it seeks to the element's flag bit, reads it, seeks back to the payload cursor, and then either copies the previous element verbatim or calls the element type's decode. Returns the bits the changed elements consumed, plus field_type->reserved_bits when anything changed at all.
     *
     * @address 0x4e9330
     */
    static int32_t array_field_decode(message_delta_field_type *field_type, uint8_t *previous,
    uint8_t *destination, bit_stream *stream);

    /**
     * 0x4cf9a0, memory module; the signature is src/memory/bit_stream_write_bit.c's. Encodes an array-of-structures field. field_type->array_descriptor holds {count, fields[]}. When previous is NULL every element is encoded unconditionally. Otherwise the per-element "changed" bits live in a block of field_type->reserved_bits reserved at the head of the array and the element payloads follow it: for each element the encoder encodes the payload, seeks back to that element's flag bit, writes whether the payload produced any bits, and seeks forward again to the payload cursor. If nothing changed at all it rewinds to the start of the reserved block, so an unchanged array costs nothing.
     *
     * @address 0x4e95e0
     */
    static int32_t array_field_encode(message_delta_field_type *field_type, uint8_t *previous,
    uint8_t *destination, bit_stream *stream);

    /**
     * Original engine function `message_delta_compound_compute_size`.
     *
     * @address 0x4e9530
     */
    static int32_t compound_compute_size(message_delta_field_type *field_type);

    /**
     * Original engine function `message_delta_compound_decode`.
     *
     * @address 0x4e97e0
     */
    static int32_t compound_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream);

    /**
     * Original engine function `message_delta_compound_initialize`.
     *
     * @address 0x4e9580
     */
    static uint8_t compound_initialize(message_delta_field_type *field_type);

    /**
     * 0x4ed070, this module Decodes one array-typed message-delta field: reads its changed-flags/static-field payload via message_delta_decode_field_changed_flags and, on success (or on the degenerate "nothing to decode" case), accumulates the bit count into state->bits_read and marks state->more_items. On failure, rewinds the stream to where it started.
     *
     * @address 0x4ec510
     */
    static int32_t decode_array_field(void **context);

    /**
     * 0x4ed1d0, this module Decodes a compound (multi-subfield) message-delta field with no baseline/incremental branch: every subfield is written straight into `destination`. Returns 1 on success (accumulating the bit count into state->bits_read and setting state->changed), or 0 and rewinds the stream on failure.
     *
     * @address 0x4ec590
     */
    static uint8_t decode_compound_field(void **context, void *destination);

    /**
     * 0x4ed1d0, this module Variant of message_delta_decode_compound_field that also forwards a changed-branch offset and can be forced to report success (bits == 0 && force) without touching the stream, matching a field that had nothing to decode but must still be treated as present.
     *
     * @address 0x4ec600
     */
    static uint8_t decode_compound_field_forced(void **context, void *destination, int32_t changed_offset,
                                                    uint8_t force);

    /**
     * 0x4ed1d0, this module Decodes a compound message-delta field through a local 2048-byte scratch buffer instead of a caller-supplied destination, using the buffer as the changed-branch offset too when the message is incremental (state->incremental == 1). Returns 1 on success, 0 on failure (with the usual stream rewind).
     *
     * @address 0x4ec670
     */
    static uint8_t decode_compound_field_staged(void **context);

    /**
     * 0x4cfb80 Decodes an array of 4-byte values. When previous is NULL every element is read unconditionally. Otherwise the per-element changed bits sit in a block of field_type->reserved_bits reserved at the head of the array and the values follow it: for each element the decoder seeks to that element's flag bit, reads it, seeks back to the value cursor and either copies the previous dword verbatim or reads a fresh one. Returns the bits the changed values consumed, plus field_type->reserved_bits when anything changed at all.
     *
     * @address 0x4ea040
     */
    static int32_t dword_array_decode(message_delta_field_type *field_type, uint32_t *previous,
    uint32_t *destination, bit_stream *stream);

    /**
     * 0x4cf9a0, memory module; the signature is src/memory/bit_stream_write_bit.c's. Encodes an array of floats. When previous is NULL every element's raw 32 bits are written unconditionally. Otherwise the per-element changed bits live in a block of field_type->reserved_bits reserved at the head of the array and the values follow it: an element whose difference from the previous value exceeds +/-0.0001 has its raw 32 bits written and its flag set, everything else only costs the flag. Returns the bits the changed values consumed plus field_type->reserved_bits, or rewinds to the head of the reserved block and returns 0 when nothing changed.
     *
     * @address 0x4e9db0
     */
    static int32_t float_array_encode(message_delta_field_type *field_type, float *previous,
    float *values, bit_stream *stream);

    /**
     * Reads only the sub-fields flagged as changed in changed_flags[], invoking each one's decode callback with a changed-branch offset (changed_offset, or 0 when there is none) and a destination offset (destination_offset). Returns the total bits consumed, or 0 immediately if the stream is already out of room, or on the first field that fails to decode.
     *
     * @address 0x4ed1d0
     */
    static int32_t read_changed_subfields(message_delta_decode_state *state, uint8_t *changed_flags,
                                              int32_t changed_offset, int32_t destination_offset);

    /**
     * Original engine function `message_delta_scalar_array_compute_size`.
     *
     * @address 0x4ea220
     */
    static int32_t scalar_array_compute_size(message_delta_field_type *field_type);

    /**
     * Original engine function `message_delta_structure_array_compute_size`.
     *
     * @address 0x4e90b0
     */
    static int32_t structure_array_compute_size(message_delta_field_type *field_type);

    /**
     * Original engine function `message_delta_structure_array_encode`.
     *
     * @address 0x4e9130
     */
    static int32_t structure_array_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream);

    /**
     * Original engine function `message_delta_structure_array_initialize`.
     *
     * @address 0x4e90f0
     */
    static uint8_t structure_array_initialize(message_delta_field_type *field_type);

};

}  // namespace halo::networking
