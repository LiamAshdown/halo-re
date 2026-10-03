/**
 * @file include/halo/networking/net2_master_server.hpp
 * Master server connection, list refresh and GameSpy glue.
 *
 * The engine type headers (types/*.h) are not include-guarded: include this header after them.
 */
#pragma once

namespace halo::networking {

/**
 * Master server connection, list refresh and GameSpy glue.
 *
 * Static behaviour facade; the original C entry points forward to these members.
 */
class MasterServerConnection {
public:
    /**
     * Original engine function `gamespy_array_length`.
     *
     * @address 0x6175f0
     */
    static int32_t array_length(void *array);

    /**
     * Original engine function `gamespy_array_nth`.
     *
     * @address 0x61dc00
     */
    static void * array_nth(void *array, int32_t index);

    /**
     * Original engine function `gamespy_think_all`.
     *
     * @address 0x6154f0
     */
    static void think_all(void);

    /**
     * Resets the master-server request state, creates the server-list mutex, and starts the background thread that owns the master-server connection. On thread-creation failure, tears the mutex slot back down by hand and reports failure. Returns 1 on success, 0 otherwise (including when the mutex itself could not be created).
     *
     * @address 0x4b6000
     */
    static int32_t connection_start(void);

    /**
     * Polls the master-server connection thread every 20ms until it exits, pumping FUN_00549960 (roughly every 132ms) while waiting, then closes and clears both the thread handle and the server-list mutex.
     *
     * @address 0x4b6070
     */
    static void connection_wait_thread(void);

    /**
     * If the master-server connection is already established or connecting (state 1 or 2), abandons any in-flight query (elapsed sentinel 9999) and clears the refresh flag. Otherwise, if a connect attempt cannot be started (result < 1), immediately requests a fresh refresh; if it can, marks the refresh flag and arms the "waiting on connection" request bit.
     *
     * @address 0x4b66c0
     */
    static void ensure_list_connection(void);

    /**
     * Original engine function `master_server_list_refresh_request`.
     *
     * @address 0x4b6660
     */
    static void list_refresh_request(void);

    /**
     * Original engine function `master_server_process_pending_requests`.
     *
     * @address 0x4b5d70
     */
    static void process_pending_requests(void);

    /**
     * Original engine function `qr2_register_key`.
     *
     * @address 0x61bb40
     */
    static void register_key(int32_t keyid, const char *key);

    /**
     * Original engine function `sig__setup_master_server_connection_sig`.
     *
     * @address 0x4b5f80
     */
    static uint32_t __stdcall setup_master_server_connection_sig(void *parameter);

};

}  // namespace halo::networking
