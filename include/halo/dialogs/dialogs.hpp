#pragma once

#include "win32.h"
#include "tags.h"
#include "dialogs.h"
#include "memory.h"
#include "interface.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

namespace halo::dialogs {
namespace {

/**
 * Turns a static text control into an underlined, clickable hyperlink by subclassing the
 * control and its parent dialog. The two window procedures are the static callbacks installed
 * by install(); window is the control handle.
 */
class StaticHyperlink {
public:
    explicit StaticHyperlink(void * value) : window(value) {}
    int32_t install();
    static int32_t __stdcall parent_proc(void *hwnd, uint32_t message, uint32_t wparam, int32_t lparam);
    static int32_t __stdcall subclass_proc(void *hwnd, uint32_t message, uint32_t wparam, int32_t lparam);

    void * window;
};

/**
 * Loads and shows a modal dialog resource in the current UI language, falling back to en-US
 * and then to the plain lookup.
 */
class DialogLoader {
public:
    static int32_t show_localized(dialog_window_proc_fn dialog_proc, void *module, const char *template_name, void *parent_window);
};

/**
 * Window procedure of the shell fatal error dialog; the hyperlink control it hosts is
 * installed through StaticHyperlink.
 */
class FatalErrorDialog {
public:
    static int32_t __stdcall proc(void *dialog, uint32_t message, uint32_t wparam, int32_t lparam);
};

}
}
