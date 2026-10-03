#pragma once

#include "halo/networking/net1_common.hpp"

namespace halo::networking {

/**
 * Behaviour group for the original `network_address_*` functions; every member is the original function body moved unchanged.
 */
class AddressText {
public:
    AddressText() = delete;

    static char parse_port(char *address_string, int32_t *port_out);
    static char string_is_valid(char *address_string);
    static char string_normalize(char *address_string, char *out_buffer, uint8_t *out_is_any);
};

/**
 * Behaviour group for the original `network_address_*` functions; every member is the original function body moved unchanged.
 */
class AddressView {
public:
    s_network_address *self;

    explicit constexpr AddressView(s_network_address *record) : self(record) {}

    char * to_string();
};

}
