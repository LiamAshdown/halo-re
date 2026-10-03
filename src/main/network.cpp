/**
 * Asynchronous client connection by host name or address.
 */

#include "crt.h"
#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "interface.h"
#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "halo/main/network.hpp"
#include "halo/main/api.hpp"


extern "C" { extern main_globals main_globals_data; }
extern "C" { extern void *connect_thread; }
extern "C" { extern uint8_t ui_split_screen; }
extern "C" { extern widget_instance *ui_root_widget[1]; }
extern "C" { extern int16_t network_join_error_code; }
extern "C" { extern void network_dispatch_initialize(void); }
namespace halo::main {

/**
 * Resolves host_port_string (a "host" or "host:port" string, GlobalAlloc'd by the caller) and
 * stages the result for the main loop to connect to: on success, formats the resolved dotted
 * address (with the port re-appended, if one was given) into main_globals_data.connect_address and
 * arms main_globals_data.connect_pending. On failure, re-enters the "no address" path of
 * network_game_client_connect_to_address_async to clear the staged connect state, then -- unless
 * the UI is already showing the main menu in split-screen -- arms the "can't connect" prompt.
 * Always frees host_port_string and clears connect_thread before returning.
 *
 * @address 0x4c83e0
 */
uint32_t ClientConnection::game_client_connect_by_hostname(char *host_port_string)
{
    uint16_t port;
    char *colon;
    void *host;
    char *address_text;

    port = 0;
    colon = strchr(host_port_string, ':');
    if (colon != 0) {
        port = (uint16_t)atol(colon + 1);
        *colon = '\0';
    }
    network_dispatch_initialize();
    if (halo::main::network_hostname_resolve_with_timeout(host_port_string) != 0) {
        host = gethostbyname(host_port_string);
        if (host != 0) {
            address_text = inet_ntoa(**(struct in_addr **)((char *)host + 0xc));
            if (port == 0) {
                strncpy(main_globals_data.connect_address, address_text, 0x1f);
                main_globals_data.connect_address[0x1f] = 0;
                main_globals_data.connect_pending = 1;
            } else {
                sprintf(main_globals_data.connect_address, "%s:%d", address_text, (unsigned int)port);
                main_globals_data.connect_pending = 1;
            }
            goto done;
        }
    }
    halo::main::network_game_client_connect_to_address_async(0, 0);
    if (ui_split_screen == 1 && ui_root_widget[0] != 0 &&
        strncmp(ui_root_widget[0]->name, "the_main_menu", 14) == 0) {
        goto done;
    }
    if (network_join_error_code == -1) {
        network_join_error_code = 0x35;
    }
    main_globals_data.switch_structure_bsp_index = -1;
    main_globals_data.save_map = 0;
    main_globals_data.return_to_main_menu = 1;
done:
    GlobalFree(host_port_string);
    connect_thread = 0;
    return 0;
}

}

extern "C" { extern int32_t join_ui_state; }
extern "C" { extern char network_address_string_is_valid(char *address_string); }
extern "C" { extern char network_address_string_normalize(char *address_string, char *out_buffer, uint8_t *out_is_any); }
extern "C" { extern char network_address_parse_port(char *address_string, int32_t *port_out); }
extern "C" { extern void widget_close_all(void); }
extern "C" { extern void interface_loading_screen_reset(void); }
extern "C" { extern void interface_loading_screen_set_text(const char *text); }
extern "C" { extern void display_error(int16_t error_string_index, int32_t player_index, uint8_t modal, uint8_t is_error); }
namespace halo::main {

/**
 * Stages a "connect to address[:port], with optional password" request. With both arguments
 * NULL, just clears the staged connect state and returns success (the "cancel" call other
 * functions in this module make before showing an error). Otherwise validates address: a
 * syntactically valid, already-numeric (or exact) address is staged directly into
 * main_globals_data.connect_address; one that looks like a resolvable "host[:port]" instead spins up
 * a worker thread running network_game_client_connect_by_hostname on a heap copy of it (waiting
 * for any previous such worker to finish first). An address that is neither returns failure
 * (the caller, per the disassembly's other call sites, follows up with display_error).
 *
 * @address 0x4c8500
 */
uint8_t ClientConnection::game_client_connect_to_address_async(char *address, char *password)
{
    char normalized[0x20];
    uint8_t is_any;
    uint32_t thread_id;
    size_t length;
    char *host_copy;

    if (address == 0 && password == 0) {
        main_globals_data.connect_address[0] = 0;
        main_globals_data.connect_password[0] = 0;
        main_globals_data.connect_pending = 0;
        return 1;
    }

    if (!network_address_string_is_valid(address)) {
        goto fail;
    }

    strncpy(main_globals_data.connect_password, password, 8);
    main_globals_data.connect_password[8] = 0;

    if (!network_address_string_normalize(address, normalized, &is_any)) {
        if (!network_address_parse_port(address, 0)) {
            goto fail;
        }
        length = strlen(address) + 1;
        host_copy = (char *)(GlobalAlloc(0, length));
        strcpy(host_copy, address);

        widget_close_all();
        interface_loading_screen_reset();
        join_ui_state = 3;
        interface_loading_screen_set_text(address);

        while (connect_thread != 0) {
            Sleep(0);
        }
        connect_thread = CreateThread(0, k_main_connect_thread_stack_size,
                                      (LPTHREAD_START_ROUTINE)((void *)halo::main::network_game_client_connect_by_hostname), host_copy, 0,
                                      (LPDWORD)(&thread_id));
        return 1;
    }

    if (is_any != 0) {
        goto fail;
    }
    main_globals_data.connect_pending = 1;
    strncpy(main_globals_data.connect_address, normalized, 0x1f);
    main_globals_data.connect_address[0x1f] = 0;
    return 1;

fail:
    halo::main::network_game_client_connect_to_address_async(0, 0);
    display_error(0x35, -1, 1, 0);
    return 0;
}

}

extern "C" { extern int32_t interface_loading_screen_address_a; }
extern "C" { extern int32_t interface_loading_screen_address_b; }
extern "C" { extern int32_t interface_loading_screen_progress; }
extern "C" { extern uint16_t progress_screen_text[0x20]; }
extern "C" { extern uint16_t progress_screen_subtext[0x20]; }
extern "C" { extern int32_t interface_loading_screen_request_id; }
extern "C" { extern uint32_t network_game_client_connect_to_address(char *address_string, uint16_t *target_string); }
namespace halo::main {

/**
 * network_game_client_connect_to_address convention)
 * Resets the interface's loading-screen state, widens main_globals_data.connect_password to a
 * uint16_t string (clamped to 8 code units), and attempts the connect. On failure, arms the
 * "can't connect" prompt exactly as the two staging functions do; on success, closes the UI and
 * stops the main menu music. Either way, clears the staged connect state before returning.
 *
 * Original register convention: EAX -> connect_address (the function's own address, matching the.
 *
 * @address 0x4c8660
 */
void ClientConnection::game_client_connect_to_resolved_address(void)
{
    uint16_t wide_password[10];
    size_t length;
    size_t i;

    interface_loading_screen_address_a = -1;
    interface_loading_screen_address_b = -1;
    join_ui_state = 0;
    interface_loading_screen_progress = 0;
    progress_screen_text[0] = 0;
    progress_screen_subtext[0] = 0;
    interface_loading_screen_request_id = -1;
    network_dispatch_initialize();

    length = strlen(main_globals_data.connect_password);
    if (length > 8) {
        length = 8;
    }
    wide_password[length] = 0;
    for (i = length; i > 0; i--) {
        wide_password[i - 1] = (uint8_t)main_globals_data.connect_password[i - 1];
    }

    if (network_game_client_connect_to_address(main_globals_data.connect_address, wide_password) == 0) {
        if (network_join_error_code == -1) {
            network_join_error_code = 0x35;
        }
        main_globals_data.switch_structure_bsp_index = -1;
        main_globals_data.save_map = 0;
        main_globals_data.return_to_main_menu = 1;
    } else {
        widget_close_all();
        halo::main::main_menu_music_stop();
    }
    main_globals_data.connect_address[0] = 0;
    main_globals_data.connect_password[0] = 0;
    main_globals_data.connect_pending = 0;
}

}

extern "C" { extern void *hostname_resolve_result; }
extern "C" { extern int32_t hostname_resolve_complete; }
namespace halo::main {

/**
 * Implements network hostname resolve thread proc.
 *
 * Original register convention: hostname as the recognized parameter (__stdcall thread proc).
 *
 * @address 0x4c8340
 */
uint32_t ClientConnection::hostname_resolve_thread_proc(char *hostname)
{
    hostname_resolve_result = gethostbyname(hostname);
    hostname_resolve_complete = 1;
    ExitThread(0);
}

}

namespace halo::main {

/**
 * Resolves hostname on a worker thread, waiting up to 10 seconds; kills the thread if it hasn't
 * finished by then. Returns 1 if the resolve completed (hostname_resolve_result was filled in),
 * 0 otherwise (also clearing hostname_resolve_result on the timeout/failure path).
 *
 * Original register convention: ECX -> hostname.
 *
 * @address 0x4c8370
 */
char ClientConnection::hostname_resolve_with_timeout(char *hostname)
{
    void *thread_handle;
    uint32_t thread_id;
    uint32_t wait_result;

    hostname_resolve_complete = 0;
    thread_handle = CreateThread(0, k_main_hostname_thread_stack_size,
                                  (LPTHREAD_START_ROUTINE)halo::main::network_hostname_resolve_thread_proc, hostname, 0,
                                  (LPDWORD)(&thread_id));
    if (thread_handle != 0) {
        wait_result = WaitForSingleObject(thread_handle, k_main_hostname_resolve_timeout_ms);
        if (wait_result == 0x102) {
            TerminateThread(thread_handle, 0);
        }
        CloseHandle(thread_handle);
        if (hostname_resolve_complete != 0) {
            return 1;
        }
    }
    hostname_resolve_result = 0;
    return 0;
}

}
