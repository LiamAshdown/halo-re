/**
 * Asynchronous client connection by host name or address.
 */

#include "win32.h"
#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "interface.h"
#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "halo/main/network.hpp"
#include "halo/main/layout.hpp"
#include "halo/main/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/interface/api.hpp"
#include "halo/core/link.hpp"
#include "halo/interface/vars.hpp"
#include "halo/main/vars.hpp"
#include "halo/networking/vars.hpp"
#include "halo/platform/time.hpp"
#include "halo/platform/thread.hpp"
#include "halo/platform/memory.hpp"


static auto &main_globals_data = halo::link::ref<main_globals>(halo::main::vars().main_globals_data);
static auto &connect_thread = halo::link::ref<void *>(halo::main::vars().connect_thread);
static auto &ui_split_screen = halo::link::ref<uint8_t>(halo::ui::vars().ui_split_screen);
static auto &ui_root_widget = halo::link::ref<widget_instance *[1]>(halo::ui::vars().ui_root_widget);
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
    bool resolved = false;

    port = 0;
    colon = strchr(host_port_string, ':');
    if (colon != 0) {
        port = (uint16_t)atol(colon + 1);
        *colon = '\0';
    }
    halo::networking::network_dispatch_initialize();
    if (halo::main::network_hostname_resolve_with_timeout(host_port_string) != 0) {
        host = gethostbyname(host_port_string);
        if (host != 0) {
            address_text = inet_ntoa(**(struct in_addr **)((char *)host + k_hostent_address_list_offset));
            if (port == 0) {
                strncpy(main_globals_data.connect_address, address_text, k_main_connect_address_length - 1);
                main_globals_data.connect_address[k_main_connect_address_length - 1] = 0;
                main_globals_data.connect_pending = 1;
            } else {
                sprintf(main_globals_data.connect_address, "%s:%d", address_text, (unsigned int)port);
                main_globals_data.connect_pending = 1;
            }
            resolved = true;
        }
    }
    if (!resolved) {
        halo::main::network_game_client_connect_to_address_async(0, 0);
        if (!(ui_split_screen == 1 && ui_root_widget[0] != 0 &&
              strncmp(ui_root_widget[0]->name, k_main_menu_widget_name, sizeof(k_main_menu_widget_name)) == 0)) {
            if (halo::networking::globals().join_error_code == -1) {
                halo::networking::globals().join_error_code = k_join_error_connection_failed;
            }
            main_globals_data.switch_structure_bsp_index = -1;
            main_globals_data.save_map = 0;
            main_globals_data.return_to_main_menu = 1;
        }
    }
    halo::platform::heap_free(host_port_string);
    connect_thread = 0;
    return 0;
}

}

static auto &join_ui_state = halo::link::ref<int32_t>(halo::networking::vars().join_ui_state);
namespace halo::main {

static uint8_t connect_address_failed()
{
    halo::main::network_game_client_connect_to_address_async(0, 0);
    halo::interface::display_error(k_join_error_connection_failed, -1, 1, 0);
    return 0;
}

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
    char normalized[k_main_connect_address_length];
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

    if (!halo::networking::network_address_string_is_valid(address)) {
        return connect_address_failed();
    }

    strncpy(main_globals_data.connect_password, password, 8);
    main_globals_data.connect_password[8] = 0;

    if (!halo::networking::network_address_string_normalize(address, normalized, &is_any)) {
        if (!halo::networking::network_address_parse_port(address, 0)) {
            return connect_address_failed();
        }
        length = strlen(address) + 1;
        host_copy = (char *)(halo::platform::heap_allocate(0, length));
        strcpy(host_copy, address);

        halo::interface::widget_close_all();
        halo::interface::interface_loading_screen_reset();
        join_ui_state = 3;
        halo::interface::interface_loading_screen_set_text(address);

        while (connect_thread != 0) {
            halo::platform::sleep_milliseconds(0);
        }
        connect_thread = halo::platform::thread_create(k_main_connect_thread_stack_size, (halo::platform::thread_procedure)(void *)halo::main::network_game_client_connect_by_hostname, host_copy, 0, &thread_id);
        return 1;
    }

    if (is_any != 0) {
        return connect_address_failed();
    }
    main_globals_data.connect_pending = 1;
    strncpy(main_globals_data.connect_address, normalized, k_main_connect_address_length - 1);
    main_globals_data.connect_address[k_main_connect_address_length - 1] = 0;
    return 1;
}

}

static auto &interface_loading_screen_address_a = halo::link::ref<int32_t>(halo::main::vars().interface_loading_screen_address_a);
static auto &interface_loading_screen_address_b = halo::link::ref<int32_t>(halo::main::vars().interface_loading_screen_address_b);
static auto &interface_loading_screen_progress = halo::link::ref<int32_t>(halo::networking::vars().interface_loading_screen_progress);
static auto &progress_screen_text = halo::link::ref<uint16_t [0x20]>(halo::main::vars().progress_screen_text);
static auto &progress_screen_subtext = halo::link::ref<uint16_t [0x20]>(halo::main::vars().progress_screen_subtext);
static auto &interface_loading_screen_request_id = halo::link::ref<int32_t>(halo::networking::vars().interface_loading_screen_request_id);
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
    halo::networking::network_dispatch_initialize();

    length = strlen(main_globals_data.connect_password);
    if (length > 8) {
        length = 8;
    }
    wide_password[length] = 0;
    for (i = length; i > 0; i--) {
        wide_password[i - 1] = (uint8_t)main_globals_data.connect_password[i - 1];
    }

    if (halo::networking::network_game_client_connect_to_address((wchar_t *)wide_password, main_globals_data.connect_address) == 0) {
        if (halo::networking::globals().join_error_code == -1) {
            halo::networking::globals().join_error_code = k_join_error_connection_failed;
        }
        main_globals_data.switch_structure_bsp_index = -1;
        main_globals_data.save_map = 0;
        main_globals_data.return_to_main_menu = 1;
    } else {
        halo::interface::widget_close_all();
        halo::main::main_menu_music_stop();
    }
    main_globals_data.connect_address[0] = 0;
    main_globals_data.connect_password[0] = 0;
    main_globals_data.connect_pending = 0;
}

}

static auto &hostname_resolve_result = halo::link::ref<void *>(halo::main::vars().hostname_resolve_result);
static auto &hostname_resolve_complete = halo::link::ref<int32_t>(halo::main::vars().hostname_resolve_complete);
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
    halo::platform::thread_exit(0);
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
    thread_handle = halo::platform::thread_create(k_main_hostname_thread_stack_size, (halo::platform::thread_procedure)halo::main::network_hostname_resolve_thread_proc, hostname, 0, &thread_id);
    if (thread_handle != 0) {
        wait_result = halo::platform::wait(thread_handle, k_main_hostname_resolve_timeout_ms);
        if (wait_result == win32::k_wait_timeout) {
            halo::platform::thread_terminate(thread_handle, 0);
        }
        halo::platform::handle_close(thread_handle);
        if (hostname_resolve_complete != 0) {
            return 1;
        }
    }
    hostname_resolve_result = 0;
    return 0;
}

}
