#include "halo/shell/diagnostics.hpp"
#include "halo/shell/layout.hpp"
#include "interface.h"
#include "halo/sound/api.hpp"
#include "halo/rasterizer/api.hpp"
#include "halo/core/link.hpp"
#include "halo/interface/vars.hpp"
#include "halo/shell/vars.hpp"
#include "halo/shell/api.hpp"

static auto &shell_stack_guard_page = halo::link::ref<void *>(halo::shell::vars().shell_stack_guard_page);
static auto &shell_stack_guard_old_protect = halo::link::ref<uint32_t>(halo::shell::vars().shell_stack_guard_old_protect);
static auto &crash_in_progress = halo::link::ref<int32_t>(halo::shell::vars().crash_in_progress);
static auto &report_fault = halo::link::ref<report_fault_fn>(halo::shell::vars().report_fault);
static auto &shell_window_proc_bypass = halo::link::ref<uint8_t>(halo::shell::vars().shell_window_proc_bypass);
static auto &chat_gui_root_handle = halo::link::ref<void *>(halo::ui::vars().chat_gui_root_handle);
static auto &chat_gui_find_object = halo::link::ref<chat_gui_find_object_fn>(halo::ui::vars().chat_gui_find_object);
static auto &chat_gui_release = halo::link::ref<chat_gui_release_fn>(halo::ui::vars().chat_gui_release);
static auto &keystone_release = halo::link::ref<keystone_release_fn>(halo::shell::vars().keystone_release);
static auto &chat_gui_find_object_arg = halo::link::ref<void *>(halo::ui::vars().chat_gui_find_object_arg);
static auto &chat_listbox_gui_find_object_arg = halo::link::ref<void *>(halo::ui::vars().chat_listbox_gui_find_object_arg);
static auto &shell_window = halo::link::ref<void *>(halo::shell::vars().shell_window);
static auto &exception_title = halo::link::ref<char [k_shell_exception_string_length]>(halo::shell::vars().exception_title);
static auto &exception_gathering_text = halo::link::ref<char [k_shell_exception_string_length]>(halo::shell::vars().exception_gathering_text);

namespace halo::shell {

namespace {

/**
 * Handles and buffers of one crash report in progress.
 */
struct CrashSession {
    win32_exception_pointers *exception_pointers;
    win32_exception_record *record;
    void *dialog;
    void *template_memory;
    void *mapping;
    dw_shared_memory *shared;
    void *event_done;
    void *event_alive;
    void *mutex;
    char file_list[0x410];
};

/**
 * Windows implementation: Dr. Watson (dw15.exe) fed with a shared memory block, dxdiag.txt, debug.txt
 * and network.log.
 */
class WatsonCrashReporter final : public CrashReporter {
public:
    int32_t handle_exception(win32_exception_pointers *exception_pointers) const override;

private:
    static void reset_fpu();
    static void report_fault_and_exit(win32_exception_pointers *exception_pointers);
    static void shut_down_services();
    static void show_gathering_dialog(CrashSession *session);
    static void create_shared_block(CrashSession *session);
    static void run_dxdiag(CrashSession *session);
    static void collect_log_files(CrashSession *session);
    static void close_gathering_dialog(CrashSession *session);
    static int32_t run_watson(CrashSession *session);
};

constexpr WatsonCrashReporter k_watson_crash_reporter{};

/**
 * Resets the x87 unit to its default control word.
 */
void WatsonCrashReporter::reset_fpu()
{
    uint16_t control_word = 0x27f;
    __asm {
        finit
        fldcw control_word
    }
}

/**
 * Shared failure tail: hands the exception to Windows Error Reporting when faultrep.dll was found,
 * then exits with 1.
 */
void WatsonCrashReporter::report_fault_and_exit(win32_exception_pointers *exception_pointers)
{
    if (report_fault != 0) {
        report_fault(exception_pointers, 0);
    }
    ExitProcess(1);
}

/**
 * Tears down the subsystems that would block the report: window procedure bypass, gamma ramp,
 * deferred windowed operations, sound, the chat UI and the main window. Faults are ignored.
 */
void WatsonCrashReporter::shut_down_services()
{
    void *window;

    __try {
        shell_window_proc_bypass = 1;
        halo::rasterizer::chimera__registry_check_3();
        halo::rasterizer::rasterizer_service_deferred_windowed_ops();
        halo::sound::sound_stop_all();
        ShowCursor(1);
        if (chat_gui_root_handle != 0) {
            window = chat_gui_find_object(chat_gui_root_handle, chat_gui_find_object_arg);
            if (window != 0) {
                chat_gui_release(window);
            }
            window = chat_gui_find_object(chat_gui_root_handle, chat_listbox_gui_find_object_arg);
            if (window != 0) {
                chat_gui_release(window);
            }
            keystone_release(chat_gui_root_handle);
            chat_gui_root_handle = 0;
        }
        if (shell_window != 0) {
            ShowWindow((HWND)shell_window, 6);
        }
    } __except (1) {
    }
}

/**
 * Builds the "Gathering Exception Data..." window from an in-memory dialog template and shows it. A
 * fault here reports through Windows Error Reporting and ends the process.
 */
void WatsonCrashReporter::show_gathering_dialog(CrashSession *session)
{
    crash_dialog_template *dialog_template;
    crash_dialog_item_template *item;
    uint16_t *cursor;
    int32_t count;
    win32_msg message;

    __try {
        session->template_memory = GlobalAlloc(k_gmem_zeroinit, k_crash_dialog_template_allocation);
        dialog_template = (crash_dialog_template *)GlobalLock(session->template_memory);
        dialog_template->style = 0x80c800c0;
        dialog_template->item_count = 1;
        dialog_template->x = 0;
        dialog_template->y = 0;
        dialog_template->cx = 0xa3;
        dialog_template->cy = 0x37;
        cursor = &dialog_template->menu;
        *cursor++ = 0;
        *cursor++ = 0;
        count = MultiByteToWideChar(0, 0, exception_title, -1, (LPWSTR)cursor, k_crash_dialog_text_characters);
        cursor = cursor + count;
        *cursor++ = k_crash_dialog_font_size;
        count = MultiByteToWideChar(0, 0, "MS Sans Serif", -1, (LPWSTR)cursor, k_crash_dialog_text_characters);
        cursor = cursor + count;

        item = (crash_dialog_item_template *)((((uint32_t)cursor + 3) >> 2) << 2);
        item->style = 0x50020000;
        item->id = 0xffff;
        item->x = 0x1c;
        item->y = 0x17;
        item->cx = 0x6c;
        item->cy = 8;
        cursor = &item->class_ordinal_marker;
        *cursor++ = 0xffff;
        *cursor++ = 0x82;
        count = MultiByteToWideChar(0, 0, exception_gathering_text, -1, (LPWSTR)cursor, k_crash_dialog_text_characters);
        cursor = cursor + count + 1;
        GlobalUnlock(session->template_memory);

        session->dialog = CreateDialogIndirectParamA(GetModuleHandleA(0), (LPCDLGTEMPLATEA)session->template_memory, 0,
                                                     (DLGPROC)((void *)DialogCentering::procedure), 0);
        ShowWindow((HWND)session->dialog, 5);
        SetWindowPos((HWND)session->dialog, (HWND)-1, 0, 0, 0, 0, 3);
        while (PeekMessageA((LPMSG)&message, (HWND)session->dialog, 0, 0, 1)) {
            DispatchMessageA((const MSG *)&message);
        }
    } __except (1) {
        report_fault_and_exit(session->exception_pointers);
    }
}

/**
 * Creates the inheritable shared memory block, the two events and the mutex Dr. Watson reads, and
 * fills in the crash description. Any failure reports through Windows Error Reporting and ends the
 * process.
 */
void WatsonCrashReporter::create_shared_block(CrashSession *session)
{
    win32_security_attributes attributes;
    char module_file_name[k_shell_path_length];

    attributes.length = sizeof(attributes);
    attributes.security_descriptor = 0;
    attributes.inherit_handle = 1;
    session->mapping = CreateFileMappingA((void *)-1, (LPSECURITY_ATTRIBUTES)&attributes, 4, 0, k_dw_shared_memory_size, 0);
    if (session->mapping == 0) {
        report_fault_and_exit(session->exception_pointers);
    }
    session->shared = (dw_shared_memory *)MapViewOfFile(session->mapping, k_file_map_read_write, 0, 0, 0);
    if (session->shared == 0) {
        report_fault_and_exit(session->exception_pointers);
    }
    memset(session->shared, 0, sizeof(dw_shared_memory));

    session->event_done = CreateEventA((LPSECURITY_ATTRIBUTES)&attributes, 0, 0, 0);
    session->event_alive = CreateEventA((LPSECURITY_ATTRIBUTES)&attributes, 0, 0, 0);
    session->mutex = CreateMutexA((LPSECURITY_ATTRIBUTES)&attributes, 0, 0);
    if (!DuplicateHandle(GetCurrentProcess(), GetCurrentProcess(), GetCurrentProcess(),
                         (LPHANDLE)&session->shared->process, k_process_all_access, 1, 0)) {
        report_fault_and_exit(session->exception_pointers);
    }
    if (session->event_alive == 0 || session->event_done == 0 || session->mutex == 0) {
        report_fault_and_exit(session->exception_pointers);
    }

    session->shared->process_id = GetCurrentProcessId();
    session->shared->thread_id = GetCurrentThreadId();
    session->shared->event_alive = (uint32_t)session->event_alive;
    session->shared->event_done = (uint32_t)session->event_done;
    session->shared->mutex = (uint32_t)session->mutex;
    session->shared->size = k_dw_shared_memory_size;
    session->shared->exception_pointers = (uint32_t)session->exception_pointers;
    session->shared->exception_address = session->record->address;
    session->shared->offer_flags = k_dw_offer_flags;
    session->shared->behavior_flags = k_dw_behavior_flags;
    wcscpy((wchar_t *)session->shared->application_name, L"Halo");
    GetModuleFileNameA(0, module_file_name, sizeof(module_file_name));
    mbstowcs((wchar_t *)session->shared->module_file_name, module_file_name, strlen(module_file_name));
    memcpy(session->shared->registry_subpath, "Microsoft\\PCHealth\\ErrorReporting\\DW", 37);
    session->shared->let_run_flags = k_dw_let_run_flags;
    memcpy(session->shared->server, "watson.microsoft.com", 21);
    session->file_list[0] = 0;
}

/**
 * Runs dxdiag.exe /whql:off /t into the temp directory and waits for it, one minute per wake-up,
 * pumping the gathering window's messages. The resulting path becomes the first attachment. Faults
 * skip the rest of the step.
 */
void WatsonCrashReporter::run_dxdiag(CrashSession *session)
{
    win32_startup_info_a startup_info;
    win32_process_information process_information;
    win32_msg wait_message;
    uint32_t priority_class;
    uint32_t wait_result;
    char temp_path[k_shell_path_length];
    char dxdiag_command[0x208];

    __try {
        memset(&startup_info, 0, sizeof(startup_info));
        startup_info.size = sizeof(startup_info);
        process_information.process = 0;
        process_information.thread = 0;
        process_information.process_id = 0;
        process_information.thread_id = 0;
        GetTempPathA(sizeof(temp_path), temp_path);
        strcat(temp_path, "dxdiag.txt");
        wsprintfA(dxdiag_command, "dxdiag.exe /whql:off /t %s", temp_path);
        if (CreateProcessA(0, dxdiag_command, 0, 0, 0, k_create_default_error_mode | k_normal_priority_class, 0, 0, (LPSTARTUPINFOA)&startup_info,
                           (LPPROCESS_INFORMATION)&process_information)) {
            priority_class = GetPriorityClass(GetCurrentProcess());
            SetPriorityClass(GetCurrentProcess(), k_idle_priority_class);
            do {
                wait_result = MsgWaitForMultipleObjects(1, (void **)&process_information.process, 0, k_dw_dxdiag_timeout,
                                                        0xff);
                if (wait_result == 0) {
                    if (session->file_list[0] != 0) {
                        strcat(session->file_list, "|");
                    }
                    strcpy(session->file_list, temp_path);
                }
                while (PeekMessageA((LPMSG)&wait_message, (HWND)session->dialog, 0, 0, 1)) {
                    if (wait_message.message == k_wm_command) {
                        wait_result = win32::k_wait_timeout;
                    }
                    DispatchMessageA((const MSG *)&wait_message);
                }
            } while (wait_result != win32::k_wait_object_0 && wait_result != win32::k_wait_timeout);
            SetPriorityClass(GetCurrentProcess(), priority_class);
            CloseHandle((void *)process_information.process);
            CloseHandle((void *)process_information.thread);
        }
    } __except (1) {
    }
}

/**
 * Appends debug.txt and network.log from the current directory to the attachment list and stores the
 * lower-cased list in the shared block.
 */
void WatsonCrashReporter::collect_log_files(CrashSession *session)
{
    char current_directory[k_shell_path_length];

    if (session->file_list[0] != 0) {
        strcat(session->file_list, "|");
    }
    GetCurrentDirectoryA(sizeof(current_directory), current_directory);
    strcat(session->file_list, current_directory);
    strcat(session->file_list, "\\debug.txt");
    if (session->file_list[0] != 0) {
        strcat(session->file_list, "|");
    }
    strcat(session->file_list, current_directory);
    strcat(session->file_list, "\\network.log");
    if (session->file_list[0] != 0) {
        _strlwr(session->file_list);
        MultiByteToWideChar(0, 0, session->file_list, -1, (LPWSTR)session->shared->additional_files, 0x400);
    }
}

/**
 * Destroys the gathering window and frees its template. Faults are ignored.
 */
void WatsonCrashReporter::close_gathering_dialog(CrashSession *session)
{
    __try {
        if (session->dialog != 0) {
            DestroyWindow((HWND)session->dialog);
        }
        if (session->template_memory != 0) {
            GlobalFree(session->template_memory);
        }
    } __except (1) {
    }
}

/**
 * Starts .\Watson\dw15.exe -x -s <mapping handle> and waits for it, watching the alive and done events
 * and the mutex. Returns 2 when the shared block asks for the process to end, 1 otherwise. A fault or
 * a failed launch reports through Windows Error Reporting and ends the process.
 */
int32_t WatsonCrashReporter::run_watson(CrashSession *session)
{
    win32_startup_info_a startup_info;
    win32_process_information process_information;
    uint32_t wait_result;
    int32_t keep_waiting;
    int32_t outcome;
    char watson_command[k_shell_path_length];

    keep_waiting = 1;
    memset(&startup_info, 0, sizeof(startup_info));
    startup_info.size = sizeof(startup_info);
    process_information.process = 0;
    process_information.thread = 0;
    process_information.process_id = 0;
    process_information.thread_id = 0;
    wsprintfA(watson_command, ".\\Watson\\dw15.exe -x -s %u", (uint32_t)session->mapping);
    __try {
        if (!CreateProcessA(0, watson_command, 0, 0, 1, k_create_default_error_mode | k_normal_priority_class, 0, 0, (LPSTARTUPINFOA)&startup_info,
                            (LPPROCESS_INFORMATION)&process_information)) {
            report_fault_and_exit(session->exception_pointers);
        }
        while (keep_waiting != 0) {
            if (WaitForSingleObject(session->event_alive, k_dw_alive_timeout) == 0) {
                if (WaitForSingleObject(session->event_done, 1) == 0) {
                    keep_waiting = 0;
                }
                continue;
            }
            wait_result = WaitForSingleObject(session->mutex, k_dw_mutex_timeout);
            if (wait_result == win32::k_wait_timeout) {
                keep_waiting = 0;
                continue;
            }
            if (wait_result == win32::k_wait_abandoned) {
                keep_waiting = 0;
                ReleaseMutex(session->mutex);
                continue;
            }
            if (WaitForSingleObject(session->event_alive, 1) != 0) {
                SetEvent(session->event_done);
                keep_waiting = 0;
            } else if (WaitForSingleObject(session->event_done, 1) == 0) {
                keep_waiting = 0;
            }
            ReleaseMutex(session->mutex);
        }
        CloseHandle((void *)process_information.process);
        CloseHandle((void *)process_information.thread);
        CloseHandle(session->event_alive);
        CloseHandle(session->event_done);
        CloseHandle(session->mutex);
        outcome = (session->shared->result & 1) != 0 ? 2 : 1;
    } __except (1) {
        report_fault_and_exit(session->exception_pointers);
    }
    return outcome;
}

/**
 * Unhandled exception filter: tears the game down enough to show the gathering window, runs dxdiag,
 * then launches Dr. Watson on a shared memory block describing the crash plus dxdiag.txt, debug.txt and
 * network.log and waits for it. Returns EXCEPTION_CONTINUE_SEARCH (0) or ends the process.
 *
 * @address 0x542fa0
 */
int32_t WatsonCrashReporter::handle_exception(win32_exception_pointers *exception_pointers) const
{
    CrashSession session;
    void *faultrep_module;
    int32_t outcome;

    session.exception_pointers = exception_pointers;
    session.record = (win32_exception_record *)exception_pointers->exception_record;

    reset_fpu();

    if (shell_stack_guard_page != 0) {
        VirtualProtect(shell_stack_guard_page, 1, shell_stack_guard_old_protect, (PDWORD)&shell_stack_guard_old_protect);
        shell_stack_guard_page = 0;
    }
    if (crash_in_progress != 0) {
        TerminateProcess(GetCurrentProcess(), 1);
    }
    crash_in_progress = 1;

    faultrep_module = LoadLibraryA("faultrep.dll");
    if (faultrep_module != 0) {
        report_fault = (report_fault_fn)GetProcAddress((HMODULE)faultrep_module, "ReportFault");
    }

    shut_down_services();
    show_gathering_dialog(&session);
    create_shared_block(&session);
    run_dxdiag(&session);
    collect_log_files(&session);
    close_gathering_dialog(&session);
    outcome = run_watson(&session);

    if (outcome != 1) {
        ExitProcess(0);
    }
    CloseHandle((void *)session.shared->process);
    UnmapViewOfFile(session.shared);
    CloseHandle(session.mapping);
    return 0;
}

}

/**
 * The crash reporter of the running platform.
 */
const CrashReporter &CrashReporter::current()
{
    return k_watson_crash_reporter;
}

}
