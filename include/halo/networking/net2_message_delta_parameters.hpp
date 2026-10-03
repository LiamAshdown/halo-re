/**
 * @file include/halo/networking/net2_message_delta_parameters.hpp
 * Parameters-protocol registration, config file and update packets.
 *
 * The engine type headers (types/*.h) are not include-guarded: include this header after them.
 */
#pragma once

namespace halo::networking {

/**
 * Parameters-protocol registration, config file and update packets.
 *
 * Static behaviour facade; the original C entry points forward to these members.
 */
class ParametersProtocol {
public:
    /**
     * 0x4ebd50, this module Tears down every message type's field bindings and, if the dynamic-parameters protocol is enabled, writes the current formatted parameter values out to parameters.cfg and frees the registration table.
     *
     * @address 0x4ec330
     */
    static void dump_to_config_file(void);

    /**
     * Finds a registered dynamic parameter by exact name match. When found and out_value is non-NULL, writes the parameter's stored value pointer through it. Returns 1 if found, 0 otherwise.
     *
     * @address 0x4ec110
     */
    static uint8_t find_registered(char *name, void **out_value);

    /**
     * Serializes a decoded array of parameter values back into "name value\n" text lines (in registration order), used after message_delta_parameters_protocol_receive_update decodes an incoming update.
     *
     * @address 0x4ec230
     */
    static void format_received_values(int32_t *values);

    /**
     * Serializes every registered dynamic parameter's current name/value pair as a "name value\n" line into the shared config text buffer.
     *
     * @address 0x4ec050
     */
    static void format_registered_values(void);

    /**
     * 0x006b86c0 Frees every GlobalAlloc'd name string in the dynamic-parameters registration table (only while the parameters protocol is enabled) and resets the registration count to zero.
     *
     * @address 0x4ebd50
     */
    static void free_registered(void);

    /**
     * 0x006b86c0 Flattens every registered dynamic parameter's current 32-bit value into a contiguous array for network transmission.
     *
     * @address 0x4ec1a0
     */
    static void pack_values(int32_t *out_values);

    /**
     * Looks up `name`'s text value in the loaded parameters.cfg buffer and scans it into *out_value with the given scanf format. Returns 1 on a successful scan, 0 if the name was not found or the scan failed.
     *
     * @address 0x4ec0d0
     */
    static int32_t parse_value_from_config(char *name, char *format, void *out_value);

    /**
     * 0x4ec230, this module Handles an incoming dynamic-parameters protocol message: for a baseline (non-incremental) message, decodes the sequence byte plus the packed value array and, on success, formats the values back into text and latches the new sequence number; for an incremental message, drains the field through the staged/scratch decoder without applying it.
     *
     * @address 0x4ec000
     */
    static void receive_update(void **context);

    /**
     * 0x4ec0d0, this module Registers a named dynamic tunable parameter (type 1 == int, otherwise float). When scope is non-NULL the stored name is "<scope>::<name>"; otherwise it is a duplicate of name. If the name is not already registered, appends a new (name, type, value) row. Either way, seeds *value from the previously-loaded parameters.cfg text via the appropriate scanf format.
     *
     * @address 0x4ebe00
     */
    static void run_register(char *scope, char *name, int32_t type, void *value);

    /**
     * 0x00860b40, shared parameters.cfg text Reads the whole of parameters.cfg into the shared config text buffer, NUL-terminated, so message_delta_parameters_protocol_parse_value_from_config can scan values out of it later.
     *
     * @address 0x4ebda0
     */
    static void reload_from_config_file(void);

    /**
     * Builds and broadcasts a message-delta message (type 0x22) carrying the current dynamic parameter values, advancing a rolling 0..3 sequence number once the broadcast succeeds.
     *
     * @address 0x4ebf50
     */
    static void send_update(void);

};

}  // namespace halo::networking
