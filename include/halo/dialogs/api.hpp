/**
 * @file include/halo/dialogs/api.hpp
 * Functions of the dialogs module that other modules and the data tables call (namespace halo::dialogs). The record types are
 * forward-declared, so the header is light enough for every caller and for the data tables.
 */
#pragma once

#include <stdint.h>



namespace halo::dialogs {

int32_t dialog_box_show_localized(dialog_window_proc_fn dialog_proc, void *module, const char *template_name, void *parent_window);
int32_t dialog_static_hyperlink_install(void *control);
int32_t __stdcall dialog_static_hyperlink_parent_proc(void *hwnd, uint32_t message, uint32_t wparam, int32_t lparam);
int32_t __stdcall dialog_static_hyperlink_subclass_proc(void *hwnd, uint32_t message, uint32_t wparam, int32_t lparam);
int32_t __stdcall fatal_error_dialog_proc(void *dialog, uint32_t message, uint32_t wparam, int32_t lparam);

}
