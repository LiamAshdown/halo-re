#pragma once

/* Include after the engine type headers (types/*.h carry no include guards). */

namespace halo::main {

/**
 * Asynchronous client connection by host name or address.
 * Stateless service class: the functions are static members, the state they act on lives in the engine globals.
 */
struct ClientConnection {
    static uint32_t game_client_connect_by_hostname(char *host_port_string);
    static uint8_t game_client_connect_to_address_async(char *address, char *password);
    static void game_client_connect_to_resolved_address(void);
    static uint32_t hostname_resolve_thread_proc(char *hostname);
    static char hostname_resolve_with_timeout(char *hostname);
};

}
