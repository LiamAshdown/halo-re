/**
 * @file include/halo/networking/net2_autopatch.hpp
 * Auto-patch version check, download pool and updater launch.
 *
 * The engine type headers (types/*.h) are not include-guarded: include this header after them.
 */
#pragma once

namespace halo::networking {

/**
 * Auto-patch version check, download pool and updater launch.
 *
 * Static behaviour facade; the original C entry points forward to these members.
 */
class AutopatchUpdater {
public:
    /**
     * 0x5771e0, this module One-time entry point that kicks off the background thread which checks bungie.net for a game update: if the update-config directory is writable, deletes any stale "currentupdate.cfg" first, then starts the version-check thread.
     *
     * @address 0x577240
     */
    static int32_t check_for_update_start(void);

    /**
     * Writes the hardcoded current game build version string ("01.00.10.0621") into out (a 14-byte buffer, including the NUL).
     *
     * @address 0x578190
     */
    static void current_version_string_get(char *out);

    /**
     * 0x006ef93c Completion callback for an asynchronous download/read: on success, copies the received data into a heap buffer (unless the slot's close-requested flag is set) and marks the matching pool slot ready or errored.
     *
     * @address 0x576ad0
     */
    static uint32_t download_complete_callback(int32_t request_id, int32_t error, uint8_t *data, uint32_t size);

    /**
     * 0x006ef93c Retrieves the completed data pointer and size from a finished download pool slot, if ready. Returns 1 (with *out_data/*out_size filled) on success, 0 otherwise.
     *
     * @address 0x576f00
     */
    static uint8_t download_get_result(void **out_data, int32_t *out_size, int32_t slot_index);

    /**
     * 0x576b80, this module VERIFIED against disassembly 0x576c30..0x576db0 (2026-09-30): slot init (5 dwords, request id -1), the 32 entry 0x28 byte mutex scan (in_use at +0x24, name at +4), the 32 entry 8 byte thread scan, the CreateThread/SetThreadPriority/ResumeThread sequence and the cleanup match. Fixed: 0x7227c8 (autopatch_download_active_count) is a BYTE everywhere in the original (the 4-byte C store clobbered 0x7227c9..0x7227cb), and the mutex name uses the CRT _snprintf. Initializes the two-slot asynchronous download table, then inline-allocates a named mutex (mirroring mutex_create) and a suspended worker thread (mirroring network_thread_create), resuming it on success. Returns 1 once the pool is fully up, 0 if the mutex or thread could not be created (the pool is left initialized but idle either way).
     *
     * @address 0x576c30
     */
    static uint8_t download_pool_initialize(void);

    /**
     * Signals the download pool's worker thread to stop, waits for it to exit (STILL_ACTIVE == 0x103), then closes its thread and mutex handles and resets the pool table slots.
     *
     * @address 0x576db0
     */
    static uint32_t download_pool_shutdown(void);

    /**
     * Advances/cleans up the small asynchronous download slot table and returns the count of still (non-free) slots.
     *
     * @address 0x576bc0
     */
    static int32_t download_pool_tick(void);

    /**
     * 0x61c030
     *
     * @address 0x576a70
     */
    static void download_progress_callback(int32_t request, int32_t state, const char *buffer, int32_t buffer_length,
    int32_t bytes_received, int32_t total_size, void *param);

    /**
     * 0x576ad0, this module Starts an asynchronous download (or local file read, when local_file is set) into a free pool slot, using autopatch_download_complete_callback for completion, and returns the slot index (or -1 if the pool is full or the start call itself failed).
     *
     * @address 0x576e60
     */
    static int32_t download_start(void *path, int32_t local_file);

    /**
     * 0x576bc0, this module Background worker thread that repeatedly polls the download pool until told to stop.
     *
     * @address 0x576b80
     */
    static uint32_t download_worker_thread(void);

    /**
     * 0x6720a4 Finds the HTTP proxy the autopatch client should use: first the WinInet (Internet Explorer) setting, then WinHTTP auto-detection (WPAD) for http://www.bungie.net. From the proxy list it takes the "http=" entry (or the first entry), strips any leading "http://" prefixes and copies it into autopatch_proxy_server, which it always returns (empty when no proxy was found).
     *
     * @address 0x576f40
     */
    static char * get_proxy_settings(void);

    /**
     * Writes currentupdate.cfg (game mode, update URL, target version and the command line that relaunches the game), starts haloupdate.exe with this process's id, and asks the main loop to quit. Returns true once the updater process has started.
     *
     * @address 0x577310
     */
    static uint8_t launch_updater(void);

    /**
     * Detects and installs the proxy configuration used by the autopatch HTTP client, then signals it is ready. FIXED 2026-09-28: a CreateThread routine (network_initialize), __stdcall with the unused thread parameter -- the original ends ret 4 (0x5771d7).
     *
     * @address 0x5771c0
     */
    static uint32_t __stdcall proxy_initialize(void *parameter);

    /**
     * Seeds the CRT RNG from the current time (xored with a constant) and writes a random 7-letter lowercase string into the shared temp-name buffer, used as a scratch download file name.
     *
     * @address 0x575fa0
     */
    static char * temp_name_generate(void);

    /**
     * Original engine function `autopatch_version_check_completed`.
     *
     * @address 0x5777d0
     */
    static void version_check_completed(int32_t available, int32_t mandatory, const char *version_name, int32_t file_id,
    const char *download_url, void *param);

    /**
     * Thread that waits for proxy setup and then sends the current game version and distribution id to check for an update; clears the "check succeeded" flag if the version string is empty or the request could not be sent.
     *
     * @address 0x5771e0
     */
    static uint32_t version_check_request(void);

    /**
     * Returns 1 if version exactly matches either the current build string ("01.00.10.0621") or the minimum baseline ("01.00.08.0616"), 0 for any other string -- so, despite the name, this reads as a "recognized version" check rather than a true outdated/not-outdated comparison.
     *
     * @address 0x5781c0
     */
    static uint32_t version_string_is_outdated(char *version);

};

}  // namespace halo::networking
