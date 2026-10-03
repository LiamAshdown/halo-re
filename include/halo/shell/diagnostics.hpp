#pragma once

#include "halo/shell/platform.hpp"

namespace halo::shell {

/**
 * Localized string loading from strings.dll: the active language first, then en-US, then the plain
 * resource lookup. initialize() loads the library, picks the language and caches the diagnostic strings.
 */
class Localization {
public:
    static int32_t load_string_resource(uint32_t id, uint16_t language, uint32_t buffer_capacity, void *module, char *buffer);
    static int32_t load_localized_string(uint32_t buffer_capacity, void *module, char *buffer, uint32_t id);
    static void initialize();
};

/**
 * The game's error dialog. A fatal error shuts the engine down and ends the process; a non fatal one
 * lets the player continue and may remember the answer per graphics device.
 */
class FatalError {
public:
    static int32_t show(uint32_t resource_id, uint32_t help_text_or_id, int32_t is_fatal);

private:
    static void load_text(uint32_t resource_id, uint32_t help_text_or_id, int32_t is_fatal);
    static void make_remembered_name(char *name, uint32_t resource_id);
    static void shut_down_engine_services();
};

/**
 * Unhandled exception reporter. The Windows implementation shows a "gathering data" window, runs dxdiag
 * and hands the crash to Dr. Watson; other platforms can plug in their own.
 */
class CrashReporter {
public:
    /**
     * Handles an unhandled exception and returns the filter disposition (0 continues the search).
     */
    virtual int32_t handle_exception(win32_exception_pointers *exception_pointers) const = 0;

    /**
     * The crash reporter of the running platform.
     */
    static const CrashReporter &current();
};

/**
 * Window procedure helper for the small dialogs the shell builds: centers the dialog on the desktop.
 */
class DialogCentering {
public:
    static int32_t __stdcall procedure(void *hwnd, uint32_t message, uint32_t wparam, int32_t lparam);
};

}
