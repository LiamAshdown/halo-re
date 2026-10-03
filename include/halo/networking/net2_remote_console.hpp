/**
 * @file include/halo/networking/net2_remote_console.hpp
 * RCON requests, console glue, update server and registry lookups.
 *
 * The engine type headers (types/*.h) are not include-guarded: include this header after them.
 */
#pragma once

namespace halo::networking {

/**
 * RCON requests, console glue, update server and registry lookups.
 *
 * Static behaviour facade; the original C entry points forward to these members.
 */
class RemoteConsole {
public:
    /**
     * 0x4d8c50
     *
     * @address 0x4d8ed0
     */
    static int8_t chimera__on_connect(const uint32_t *target_address, network_client_globals *client,
                            const uint32_t *session_info);

    /**
     * Sends one line of rcon command output text (message type 0x37) back over the network.
     *
     * @address 0x4e50c0
     */
    static void chimera__rcon_out(char *text, int32_t unused_machine_id);

    /**
     * 0x496b50, EAX color (NULL = default) Shared get/set implementation for boolean sv_* console commands: with no argument, reports the current value; with one, accepts "0"/"false" or "1"/"true" (case-insensitive, trimmed) and stores it through `value`, then reports the new value the same way.
     *
     * @address 0x4e2990
     */
    static void console_command_bool_get_set(uint32_t argument_count, uint8_t *value, char **arguments, const char *name);

    /**
     * 0x496b50, EAX color (NULL = default) Console command: packages the password (first argument) and the remaining arguments -- the rcon command bare, every argument after it individually quoted -- into one command line, and sends it to the server via rcon_send_request. Client-only; refuses on a dedicated/listen server, on fewer than 2 arguments, on a password outside 1-8 characters, or if the rebuilt command line would exceed 64 characters.
     *
     * @address 0x4e4c00
     */
    static void rcon(int32_t argument_count, char **arguments);

    /**
     * Original engine function `rcon_send_request`.
     *
     * @address 0x4e4dc0
     */
    static void run_rcon_send_request(char *command, char *password);

    /**
     * Reads the installed game's "DistID" (distribution/channel id) DWORD value from the registry. Returns 0 if the key or value could not be read.
     *
     * @address 0x577760
     */
    static uint32_t registry_get_dist_id(void);

    /**
     * 0x006ef968 Reads the installed game's "Version" value from the Halo registry key and returns it as a string (empty if the key or value could not be read).
     *
     * @address 0x5776d0
     */
    static char * registry_get_halo_version(void);

    /**
     * 0x4ec450, outside this batch, elided args
     *
     * @address 0x4ddfb0
     */
    static char update_server_send_update(uint32_t *tick_count, char frame_time_overflow);

};

}  // namespace halo::networking
