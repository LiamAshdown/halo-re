// exception_filter_crash_reporter  (Ghidra: exception_filter_crash_reporter, already named)
// address 0x542fa0, size 2581 bytes (plus the handler tails at 0x5439b5..0x543a2a)
// name confidence: 0.65  rewrite confidence: 0.65
// evidence: out/phase4/shell_functions.md summary and types/shell.h dw_shared_memory /
//   crash_dialog_template. It is the filter expression of shell_winmain's outer __except
//   (0x5419c1: push [ebp-0x14], the EXCEPTION_POINTERS, call 0x542fa0, ret). Strings:
//   "faultrep.dll" / "ReportFault", "dxdiag.exe /whql:off /t %s", ".\\Watson\\dw15.exe -x -s %u".
// Rewritten against objdump 0x542fa0..0x543a2a. Facts the decompile lost:
//   - Both wsprintfA calls have their arguments (0x543520: the dxdiag.txt path; 0x543853: the
//     file mapping handle), and ReportFault is called as ReportFault(exception_pointers, 0)
//     (push 0; push [ebp+8] before every call eax).
//   - The body has five __try / __except(EXCEPTION_EXECUTE_HANDLER) blocks, scope table
//     0x00672e18 (every filter is mov eax,1; ret):
//       state 0 (0x543066..0x543101) the subsystem shutdown calls; the handler just continues;
//       state 1 (0x543115..0x54329f) the "Gathering Exception Data" dialog; handler 0x543a0f
//         reports the fault and exits with 1;
//       state 2 (0x5434af..0x54365e) running dxdiag.exe; the handler skips the rest of it;
//       state 3 (0x5437e3..0x543808) destroying the dialog; the handler continues;
//       state 4 (0x54386d..0x543978) running dw15.exe and waiting for it; handler 0x5439d4
//         reports the fault and exits with 1.
//     They are written with the CRASH_TRY / CRASH_EXCEPT macros below (real __try / __except
//     under MSVC).
//   - Entry runs finit and fldcw 0x27f (0x542fce..0x542fd8) before anything else.
//   - The events / mutex failure test jumps to 0x5439f0, which is the same report-and-exit tail.
// UNSURE (not expressible in C): for EXCEPTION_STACK_OVERFLOW (0xc00000fd) the function moves
//   its own frame 0x10000 bytes up the stack (0x54302a..0x54303f: rep movsd of 0x4000 dwords from
//   esp to esp + 0x10000, then ebp and esp += 0x10000), onto stack the overflowing code already
//   committed, so the rest of the filter has room to run. It is marked in the body.
// Original quirks reproduced as observed:
//   - When dxdiag.exe finishes, the file list gets "|" appended and is then overwritten by a
//     strcpy of the dxdiag.txt path (0x5435d6 copies to the start of the buffer). The list is empty
//     at that point, so the result is just the path.
//   - The current directory is read once; debug.txt and network.log are both appended to it.
//   - The shared block's result bit 0 set means ExitProcess(0); clear means close everything and
//     return EXCEPTION_CONTINUE_SEARCH (0).
// register convention: __stdcall, one stack argument (EXCEPTION_POINTERS *), ret 4.

#include "crt.h"
#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"
#include <string.h>
#include "interface.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

#if defined(_MSC_VER)
#define CRASH_TRY __try
#define CRASH_EXCEPT __except (1)
#else
#define CRASH_TRY
#define CRASH_EXCEPT if (0)
#endif



extern void chimera__registry_check_3(void);                                   // 0x5226c0 (rasterizer), restores the gamma ramp
extern void rasterizer_service_deferred_windowed_ops(void);                    // 0x5180d0 (rasterizer)
extern void sound_stop_all(void);                                              // 0x54adb0 (sound)
extern int32_t __stdcall dialog_center_on_screen(void *hwnd, uint32_t message, uint32_t wparam,
                                                 int32_t lparam);              // 0x542f00

extern void *shell_stack_guard_page;                                           // 0x00721f08
extern uint32_t shell_stack_guard_old_protect;                                 // 0x00721f0c
extern int32_t crash_in_progress;                                              // 0x00722bd0
extern report_fault_fn report_fault;                                           // 0x00721f10
extern uint8_t shell_window_proc_bypass;                                       // 0x00721e8d
extern void *chat_gui_root_handle;                                                    // 0x00721ea4
extern chat_gui_find_object_fn chat_gui_find_object;                            // 0x00721eb8
extern chat_gui_release_fn chat_gui_release;                            // 0x00721ec8
extern keystone_release_fn keystone_release;                                   // 0x00721eac
extern void *chat_gui_find_object_arg;                                         // 0x0069c698 -> L"KeystoneEditbox"
extern void *chat_listbox_gui_find_object_arg;                                 // 0x0069c69c -> L"KeystoneChatLog"
extern void *shell_window;                                                     // 0x007461c4
extern char exception_title[k_shell_exception_string_length];                  // 0x006f0770 "Exception!"
extern char exception_gathering_text[k_shell_exception_string_length];         // 0x006f0670 "Gathering Exception Data..."

static void crash_reset_fpu(void)
{
    uint16_t control_word = 0x27f;
#if defined(__GNUC__)
    __asm__ volatile ("finit\n\tfldcw %0" : : "m"(control_word));
#else
    __asm {
        finit
        fldcw control_word
    }
#endif
}

// The shared failure tail: hand the exception to Windows Error Reporting when faultrep.dll was
// found, then exit with 1.
static void crash_report_fault_and_exit(win32_exception_pointers *exception_pointers)
{
    if (report_fault != 0) {
        report_fault(exception_pointers, 0);
    }
    ExitProcess(1);
}

// Unhandled exception filter: tears the game down enough to show a small "Gathering Exception
// Data..." window, runs dxdiag.exe to produce dxdiag.txt, then launches Dr. Watson
// (.\Watson\dw15.exe) on an inheritable shared memory block describing the crash plus
// dxdiag.txt, debug.txt and network.log, and waits for it. Returns EXCEPTION_CONTINUE_SEARCH (0)
// or ends the process.
int32_t __stdcall exception_filter_crash_reporter(win32_exception_pointers *exception_pointers)
{
    win32_exception_record *record = (win32_exception_record *)exception_pointers->exception_record;
    void *window;
    void *faultrep_module;
    void *template_memory;
    crash_dialog_template *dialog_template;
    crash_dialog_item_template *item;
    uint16_t *cursor;
    int32_t count;
    void *dialog;
    win32_msg message;
    win32_msg wait_message;
    win32_security_attributes attributes;
    void *mapping;
    dw_shared_memory *shared;
    void *event_done;
    void *event_alive;
    void *mutex;
    win32_startup_info_a startup_info;
    win32_process_information process_information;
    uint32_t priority_class;
    uint32_t wait_result;
    int32_t keep_waiting;
    int32_t outcome;
    char module_file_name[k_shell_path_length];
    char current_directory[k_shell_path_length];
    char watson_command[k_shell_path_length];
    char file_list[0x410];
    char temp_path[k_shell_path_length];
    char dxdiag_command[0x208];

    crash_reset_fpu();

    if (shell_stack_guard_page != 0) {
        VirtualProtect(shell_stack_guard_page, 1, shell_stack_guard_old_protect, (PDWORD)&shell_stack_guard_old_protect);
        shell_stack_guard_page = 0;
    }
    if (crash_in_progress != 0) {
        TerminateProcess(GetCurrentProcess(), 1);
    }
    crash_in_progress = 1;

    if (record->code == 0xc00000fd /* EXCEPTION_STACK_OVERFLOW */) {
        // UNSURE, not expressible in C: the original copies its 0x10000 byte frame to esp + 0x10000
        // and moves ebp / esp there before continuing (see the file header).
    }

    faultrep_module = LoadLibraryA("faultrep.dll");
    if (faultrep_module != 0) {
        report_fault = (report_fault_fn)GetProcAddress((HMODULE)faultrep_module, "ReportFault");
    }

    CRASH_TRY {
        shell_window_proc_bypass = 1;
        chimera__registry_check_3();
        rasterizer_service_deferred_windowed_ops();
        sound_stop_all();
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
            ShowWindow((HWND)shell_window, 6 /* SW_MINIMIZE */);
        }
    } CRASH_EXCEPT {
    }

    // the "Gathering Exception Data..." window, built as an in-memory dialog template
    CRASH_TRY {
        template_memory = GlobalAlloc(0x40 /* GMEM_ZEROINIT */, k_crash_dialog_template_allocation);
        dialog_template = (crash_dialog_template *)GlobalLock(template_memory);
        dialog_template->style = 0x80c800c0;
        dialog_template->item_count = 1;
        dialog_template->x = 0;
        dialog_template->y = 0;
        dialog_template->cx = 0xa3;
        dialog_template->cy = 0x37;
        cursor = &dialog_template->menu;
        *cursor++ = 0;                              // no menu
        *cursor++ = 0;                              // default dialog class
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
        *cursor++ = 0x82;                           // STATIC
        count = MultiByteToWideChar(0, 0, exception_gathering_text, -1, (LPWSTR)cursor, k_crash_dialog_text_characters);
        cursor = cursor + count + 1;                // the creation data word, left zero
        GlobalUnlock(template_memory);

        dialog = CreateDialogIndirectParamA(GetModuleHandleA(0), (LPCDLGTEMPLATEA)template_memory, 0,
                                            (DLGPROC)((void *)dialog_center_on_screen), 0);
        ShowWindow((HWND)dialog, 5 /* SW_SHOW */);
        SetWindowPos((HWND)dialog, (HWND)-1 /* HWND_TOPMOST */, 0, 0, 0, 0, 3 /* SWP_NOSIZE | SWP_NOMOVE */);
        while (PeekMessageA((LPMSG)&message, (HWND)dialog, 0, 0, 1 /* PM_REMOVE */)) {
            DispatchMessageA((const MSG *)&message);
        }
    } CRASH_EXCEPT {
        crash_report_fault_and_exit(exception_pointers);
    }

    // the Dr. Watson shared memory block
    attributes.length = sizeof(attributes);
    attributes.security_descriptor = 0;
    attributes.inherit_handle = 1;
    mapping = CreateFileMappingA((void *)-1, (LPSECURITY_ATTRIBUTES)&attributes, 4 /* PAGE_READWRITE */, 0, k_dw_shared_memory_size, 0);
    if (mapping == 0) {
        crash_report_fault_and_exit(exception_pointers);
    }
    shared = (dw_shared_memory *)MapViewOfFile(mapping, 6 /* FILE_MAP_READ | FILE_MAP_WRITE */, 0, 0, 0);
    if (shared == 0) {
        crash_report_fault_and_exit(exception_pointers);
    }
    memset(shared, 0, sizeof(dw_shared_memory));

    event_done = CreateEventA((LPSECURITY_ATTRIBUTES)&attributes, 0, 0, 0);
    event_alive = CreateEventA((LPSECURITY_ATTRIBUTES)&attributes, 0, 0, 0);
    mutex = CreateMutexA((LPSECURITY_ATTRIBUTES)&attributes, 0, 0);
    if (!DuplicateHandle(GetCurrentProcess(), GetCurrentProcess(), GetCurrentProcess(), (LPHANDLE)&shared->process,
                         0x1f0fff /* PROCESS_ALL_ACCESS */, 1, 0)) {
        crash_report_fault_and_exit(exception_pointers);
    }
    if (event_alive == 0 || event_done == 0 || mutex == 0) {
        crash_report_fault_and_exit(exception_pointers);
    }

    shared->process_id = GetCurrentProcessId();
    shared->thread_id = GetCurrentThreadId();
    shared->event_alive = (uint32_t)event_alive;
    shared->event_done = (uint32_t)event_done;
    shared->mutex = (uint32_t)mutex;
    shared->size = k_dw_shared_memory_size;
    shared->exception_pointers = (uint32_t)exception_pointers;
    shared->exception_address = record->address;
    shared->offer_flags = k_dw_offer_flags;
    shared->behavior_flags = k_dw_behavior_flags;
    wcscpy((wchar_t *)shared->application_name, L"Halo");
    GetModuleFileNameA(0, module_file_name, sizeof(module_file_name));
    mbstowcs((wchar_t *)shared->module_file_name, module_file_name, strlen(module_file_name));
    memcpy(shared->registry_subpath, "Microsoft\\PCHealth\\ErrorReporting\\DW", 37);
    shared->let_run_flags = k_dw_let_run_flags;
    memcpy(shared->server, "watson.microsoft.com", 21);
    file_list[0] = 0;

    // dxdiag.exe /whql:off /t <temp>dxdiag.txt, waited for up to a minute per wake-up
    CRASH_TRY {
        memset(&startup_info, 0, sizeof(startup_info));
        startup_info.size = sizeof(startup_info);
        process_information.process = 0;
        process_information.thread = 0;
        process_information.process_id = 0;
        process_information.thread_id = 0;
        GetTempPathA(sizeof(temp_path), temp_path);
        strcat(temp_path, "dxdiag.txt");
        wsprintfA(dxdiag_command, "dxdiag.exe /whql:off /t %s", temp_path);
        if (CreateProcessA(0, dxdiag_command, 0, 0, 0,
                           0x4000020 /* CREATE_DEFAULT_ERROR_MODE | NORMAL_PRIORITY_CLASS */, 0, 0,
                           (LPSTARTUPINFOA)&startup_info, (LPPROCESS_INFORMATION)&process_information)) {
            priority_class = GetPriorityClass(GetCurrentProcess());
            SetPriorityClass(GetCurrentProcess(), 0x40 /* IDLE_PRIORITY_CLASS */);
            do {
                wait_result = MsgWaitForMultipleObjects(1, (void **)&process_information.process, 0, k_dw_dxdiag_timeout,
                                                        0xff /* QS_ALLINPUT */);
                if (wait_result == 0) {
                    if (file_list[0] != 0) {
                        strcat(file_list, "|");
                    }
                    strcpy(file_list, temp_path);   // overwrites the "|", see the header
                }
                while (PeekMessageA((LPMSG)&wait_message, (HWND)dialog, 0, 0, 1)) {
                    if (wait_message.message == 0x111 /* WM_COMMAND */) {
                        wait_result = 0x102;        // WAIT_TIMEOUT: stop waiting
                    }
                    DispatchMessageA((const MSG *)&wait_message);
                }
            } while (wait_result != 0 && wait_result != 0x102);
            SetPriorityClass(GetCurrentProcess(), priority_class);
            CloseHandle((void *)process_information.process);
            CloseHandle((void *)process_information.thread);
        }
    } CRASH_EXCEPT {
    }

    // the other two attachments, from the current directory
    if (file_list[0] != 0) {
        strcat(file_list, "|");
    }
    GetCurrentDirectoryA(sizeof(current_directory), current_directory);
    strcat(file_list, current_directory);
    strcat(file_list, "\\debug.txt");
    if (file_list[0] != 0) {
        strcat(file_list, "|");
    }
    strcat(file_list, current_directory);
    strcat(file_list, "\\network.log");
    if (file_list[0] != 0) {
        _strlwr(file_list);
        MultiByteToWideChar(0, 0, file_list, -1, (LPWSTR)shared->additional_files, 0x400);
    }

    CRASH_TRY {
        if (dialog != 0) {
            DestroyWindow((HWND)dialog);
        }
        if (template_memory != 0) {
            GlobalFree(template_memory);
        }
    } CRASH_EXCEPT {
    }

    // .\Watson\dw15.exe -x -s <mapping handle>, inheriting the mapping, events and mutex
    keep_waiting = 1;
    memset(&startup_info, 0, sizeof(startup_info));
    startup_info.size = sizeof(startup_info);
    process_information.process = 0;
    process_information.thread = 0;
    process_information.process_id = 0;
    process_information.thread_id = 0;
    wsprintfA(watson_command, ".\\Watson\\dw15.exe -x -s %u", (uint32_t)mapping);
    CRASH_TRY {
        if (!CreateProcessA(0, watson_command, 0, 0, 1, 0x4000020, 0, 0, (LPSTARTUPINFOA)&startup_info, (LPPROCESS_INFORMATION)&process_information)) {
            crash_report_fault_and_exit(exception_pointers);
        }
        while (keep_waiting != 0) {
            if (WaitForSingleObject(event_alive, k_dw_alive_timeout) == 0) {
                if (WaitForSingleObject(event_done, 1) == 0) {
                    keep_waiting = 0;
                }
                continue;
            }
            // Dr. Watson stopped signalling that it is alive: take the mutex and decide
            wait_result = WaitForSingleObject(mutex, k_dw_mutex_timeout);
            if (wait_result == 0x102 /* WAIT_TIMEOUT */) {
                keep_waiting = 0;
                continue;
            }
            if (wait_result == 0x80 /* WAIT_ABANDONED */) {
                keep_waiting = 0;
                ReleaseMutex(mutex);
                continue;
            }
            if (WaitForSingleObject(event_alive, 1) != 0) {
                SetEvent(event_done);
                keep_waiting = 0;
            } else if (WaitForSingleObject(event_done, 1) == 0) {
                keep_waiting = 0;
            }
            ReleaseMutex(mutex);
        }
        CloseHandle((void *)process_information.process);
        CloseHandle((void *)process_information.thread);
        CloseHandle(event_alive);
        CloseHandle(event_done);
        CloseHandle(mutex);
        outcome = (shared->result & 1) != 0 ? 2 : 1;
    } CRASH_EXCEPT {
        crash_report_fault_and_exit(exception_pointers);
    }

    if (outcome != 1) {
        ExitProcess(0);
    }
    CloseHandle((void *)shared->process);
    UnmapViewOfFile(shared);
    CloseHandle(mapping);
    return 0; // EXCEPTION_CONTINUE_SEARCH
}

#if 0
Original Ghidra decompilation (0x542fa0), abridged where it only repeats inlined string code:

undefined4 exception_filter_crash_reporter(undefined4 *param_1)

{
  ...
  piVar2 = (int *)*param_1;
  ExceptionList = &pvStack_14;
  if (DAT_00721f08 != (LPVOID)0x0) {
    ExceptionList = &pvStack_14;
    VirtualProtect(DAT_00721f08,1,DAT_00721f0c,&DAT_00721f0c);
    DAT_00721f08 = (LPVOID)0x0;
  }
  if (DAT_00722bd0 != 0) {
    uExitCode = 1;
    pvVar3 = GetCurrentProcess();
    TerminateProcess(pvVar3,uExitCode);
  }
  DAT_00722bd0 = 1;
  if (*piVar2 == -0x3fffff03) {
    puVar5 = (undefined4 *)&stack0xfffff4c8;
    puVar17 = (undefined4 *)&stack0x0000f4c8;
    for (iVar10 = 0x4000; iVar10 != 0; iVar10 = iVar10 + -1) {
      *puVar17 = *puVar5;
      puVar5 = puVar5 + 1;
      puVar17 = puVar17 + 1;
    }
    puVar12 = &stack0x0000fffc;
  }
  pHVar4 = LoadLibraryA("faultrep.dll");
  if (pHVar4 != (HMODULE)0x0) {
    DAT_00721f10 = GetProcAddress(pHVar4,"ReportFault");
  }
  *(undefined4 *)(puVar12 + -4) = 0;
  DAT_00721e8d = 1;
  chimera__registry_check_3();
  FUN_005180d0();
  FUN_0054adb0();
  ShowCursor(1);
  if (DAT_00721ea4 != 0) {
    iVar10 = (*DAT_00721eb8)();
    *(int *)(puVar12 + -0x6c) = iVar10;
    if (iVar10 != 0) {
      (*DAT_00721ec8)();
    }
    iVar10 = (*DAT_00721eb8)();
    *(int *)(puVar12 + -0x6c) = iVar10;
    if (iVar10 != 0) {
      (*DAT_00721ec8)();
    }
    (*DAT_00721eac)();
    DAT_00721ea4 = 0;
  }
  if (DAT_007461c4 != (HWND)0x0) {
    ShowWindow(DAT_007461c4,6);
  }
  *(undefined4 *)(puVar12 + -4) = 0xffffffff;
  *(undefined4 *)(puVar12 + -4) = 1;
  lpTemplate = GlobalAlloc(0x40,0x400);
  *(LPCDLGTEMPLATEA *)(puVar12 + -0x58) = lpTemplate;
  puVar5 = GlobalLock(lpTemplate);
  *(undefined4 **)(puVar12 + -0x7c) = puVar5;
  *puVar5 = 0x80c800c0;
  *(undefined2 *)(puVar5 + 2) = 1;
  *(undefined2 *)((int)puVar5 + 10) = 0;
  *(undefined2 *)(puVar5 + 3) = 0;
  *(undefined2 *)((int)puVar5 + 0xe) = 0xa3;
  *(undefined2 *)(puVar5 + 4) = 0x37;
  *(undefined2 *)((int)puVar5 + 0x12) = 0;
  *(undefined2 *)(puVar5 + 5) = 0;
  pWVar13 = (LPWSTR)((int)puVar5 + 0x16);
  iVar10 = MultiByteToWideChar(0,0,&DAT_006f0770,-1,pWVar13,0x32);
  pWVar13 = pWVar13 + iVar10;
  *pWVar13 = L'\b';
  pWVar13 = pWVar13 + 1;
  iVar10 = MultiByteToWideChar(0,0,"MS Sans Serif",-1,pWVar13,0x32);
  pWVar13 = pWVar13 + iVar10;
  uVar6 = (int)pWVar13 + 3U >> 2;
  puVar5 = (undefined4 *)(uVar6 * 4);
  *puVar5 = 0x50020000;
  *(undefined2 *)(puVar5 + 4) = 0xffff;
  *(undefined2 *)(puVar5 + 2) = 0x1c;
  *(undefined2 *)((int)puVar5 + 10) = 0x17;
  *(undefined2 *)(puVar5 + 3) = 0x6c;
  *(undefined2 *)((int)puVar5 + 0xe) = 8;
  *(undefined2 *)((int)puVar5 + 0x12) = 0xffff;
  *(undefined2 *)(puVar5 + 5) = 0x82;
  pWVar13 = (LPWSTR)((int)puVar5 + 0x16);
  iVar10 = MultiByteToWideChar(0,0,(LPCSTR)&DAT_006f0670,-1,pWVar13,0x32);
  GlobalUnlock(lpTemplate);
  pHVar4 = GetModuleHandleA((LPCSTR)0x0);
  pHVar18 = CreateDialogIndirectParamA(pHVar4,lpTemplate,(HWND)0x0,dialog_center_on_screen,0);
  ShowWindow(pHVar18,5);
  SetWindowPos(pHVar18,(HWND)0xffffffff,0,0,0,0,3);
  while (BVar7 = PeekMessageA((LPMSG)(puVar12 + -0x100),pHVar18,0,0,1), BVar7 != 0) {
    DispatchMessageA((MSG *)(puVar12 + -0x100));
  }
  *(undefined4 *)(puVar12 + -0x48) = 0xc;
  *(undefined4 *)(puVar12 + -0x40) = 1;
  pvVar3 = CreateFileMappingA((HANDLE)0xffffffff,(LPSECURITY_ATTRIBUTES)(puVar12 + -0x48),4,0,0x1c50
                              ,(LPCSTR)0x0);
  if (pvVar3 == (HANDLE)0x0) {
    if (DAT_00721f10 != (FARPROC)0x0) {
      (*DAT_00721f10)();
    }
    ExitProcess(1);
  }
  puVar5 = MapViewOfFile(*(HANDLE *)(puVar12 + -0x4c),6,0,0,0);
  if (puVar5 == (undefined4 *)0x0) {
    if (DAT_00721f10 != (FARPROC)0x0) {
      (*DAT_00721f10)();
    }
    ExitProcess(1);
  }
  for (iVar10 = 0x714; iVar10 != 0; iVar10 = iVar10 + -1) {
    *puVar17 = 0;
    puVar17 = puVar17 + 1;
  }
  pvVar3 = CreateEventA((LPSECURITY_ATTRIBUTES)(puVar12 + -0x48),0,0,(LPCSTR)0x0);
  pvVar8 = CreateEventA((LPSECURITY_ATTRIBUTES)(puVar12 + -0x48),0,0,(LPCSTR)0x0);
  pvVar8 = CreateMutexA((LPSECURITY_ATTRIBUTES)(puVar12 + -0x48),0,(LPCSTR)0x0);
  lpTargetHandle = (LPHANDLE)(puVar5 + 9);
  BVar7 = DuplicateHandle(hSourceProcessHandle,hSourceHandle,pvVar8,lpTargetHandle,0x1f0fff,1,0);
  if (BVar7 == 0) {
    ...ReportFault / ExitProcess(1)
  }
  if (((*(int *)(puVar12 + -0x28) == 0) || (pvVar3 == (HANDLE)0x0)) ||
     (iVar10 = *(int *)(puVar12 + -0x24), iVar10 == 0)) {
    ...ReportFault / ExitProcess(1)
  }
  puVar5[1] = GetCurrentProcessId();
  puVar5[2] = GetCurrentThreadId();
  puVar5[7] = *(undefined4 *)(puVar12 + -0x28);
  puVar5[5] = pvVar3;
  puVar5[8] = iVar10;
  *puVar5 = 0x1c50;
  piVar2 = *(int **)(puVar12 + 8);
  puVar5[4] = piVar2;
  puVar5[3] = *(undefined4 *)(*piVar2 + 0xc);
  puVar5[0xd] = 1;
  puVar5[10] = 0x100;
  _wcscpy((wchar_t *)(puVar5 + 0x12),L"Halo");
  GetModuleFileNameA((HMODULE)0x0,puVar12 + -0x204,0x104);
  _mbstowcs((wchar_t *)(puVar5 + 0x2e),puVar12 + -0x204,(int)pcVar9 - (int)(puVar12 + -0x203));
  (copy of "Microsoft\\PCHealth\\ErrorReporting\\DW" to puVar5 + 0x452)
  puVar5[0xf] = 0x11;
  (copy of "watson.microsoft.com" to puVar5 + 0x38e)
  puVar12[-0x81c] = 0;
  *(undefined4 *)(puVar12 + -200) = 0x44;
  GetTempPathA(0x104,puVar12 + -0x920);
  builtin_strncpy(pcVar11 + 1,"dxdiag.txt",0xb);
  wsprintfA(puVar12 + -0xb28,"dxdiag.exe /whql:off /t %s");
  BVar7 = CreateProcessA((LPCSTR)0x0,puVar12 + -0xb28,(LPSECURITY_ATTRIBUTES)0x0,
                         (LPSECURITY_ATTRIBUTES)0x0,0,0x4000020,(LPVOID)0x0,(LPCSTR)0x0,
                         (LPSTARTUPINFOA)(puVar12 + -200),(LPPROCESS_INFORMATION)(puVar12 + -0x3c));
  if (BVar7 != 0) {
    DVar19 = GetPriorityClass(GetCurrentProcess());
    SetPriorityClass(GetCurrentProcess(),0x40);
    do {
      DVar19 = MsgWaitForMultipleObjects(1,(HANDLE *)(puVar12 + -0x3c),0,60000,0xff);
      if (DVar19 == 0) {
        (append "|" when the list is not empty, then copy the dxdiag.txt path to its start)
      }
      while (BVar7 = PeekMessageA((LPMSG)(puVar12 + -0xe4),pHVar18,0,0,1), BVar7 != 0) {
        if (*(int *)(puVar12 + -0xe0) == 0x111) {
          DVar19 = 0x102;
        }
        DispatchMessageA((MSG *)(puVar12 + -0xe4));
      }
    } while ((DVar19 != 0) && (DVar19 != 0x102));
    SetPriorityClass(GetCurrentProcess(),DVar19);
    CloseHandle(*(HANDLE *)(puVar12 + -0x3c));
    CloseHandle(*(HANDLE *)(puVar12 + -0x38));
  }
  (append "|", the current directory and "\\debug.txt"; "|", the current directory and
   "\\network.log")
  if (puVar12[-0x81c] != '\0') {
    __strlwr(puVar12 + -0x81c);
    MultiByteToWideChar(0,0,puVar12 + -0x81c,-1,(LPWSTR)(puVar5 + 0x506),0x400);
  }
  if (*(HWND *)(puVar12 + -0x2c) != (HWND)0x0) {
    DestroyWindow(*(HWND *)(puVar12 + -0x2c));
  }
  if (*(HGLOBAL *)(puVar12 + -0x58) != (HGLOBAL)0x0) {
    GlobalFree(*(HGLOBAL *)(puVar12 + -0x58));
  }
  *(undefined4 *)(puVar12 + -0x20) = 1;
  wsprintfA(puVar12 + -0x40c,".\\Watson\\dw15.exe -x -s %u");
  BVar7 = CreateProcessA((LPCSTR)0x0,puVar12 + -0x40c,(LPSECURITY_ATTRIBUTES)0x0,
                         (LPSECURITY_ATTRIBUTES)0x0,1,0x4000020,(LPVOID)0x0,(LPCSTR)0x0,
                         (LPSTARTUPINFOA)(puVar12 + -200),(LPPROCESS_INFORMATION)(puVar12 + -0x3c));
  if (BVar7 == 0) {
    ...ReportFault / ExitProcess(1)
  }
  pvVar3 = *(HANDLE *)(puVar12 + -100);
  do {
    while( true ) {
      while( true ) {
        while( true ) {
          if (*(int *)(puVar12 + -0x20) == 0) {
            CloseHandle(*(HANDLE *)(puVar12 + -0x3c));
            CloseHandle(*(HANDLE *)(puVar12 + -0x38));
            CloseHandle(*(HANDLE *)(puVar12 + -0x28));
            CloseHandle(pvVar3);
            CloseHandle(*(HANDLE *)(puVar12 + -0x24));
            iVar10 = ((*(byte *)(puVar5 + 0xb) & 1) != 0) + 1;
            if (iVar10 != 1) {
              ExitProcess(0);
            }
            CloseHandle((HANDLE)puVar5[9]);
            UnmapViewOfFile(puVar5);
            CloseHandle(*(HANDLE *)(puVar12 + -0x4c));
            return 0;
          }
          DVar19 = WaitForSingleObject(*(HANDLE *)(puVar12 + -0x28),600000);
          if (DVar19 != 0) break;
          DVar19 = WaitForSingleObject(pvVar3,1);
          if (DVar19 == 0) {
            *(undefined4 *)(puVar12 + -0x20) = 0;
          }
        }
        DVar19 = WaitForSingleObject(*(HANDLE *)(puVar12 + -0x24),20000);
        if (DVar19 != 0x102) break;
        *(undefined4 *)(puVar12 + -0x20) = 0;
      }
      if (DVar19 != 0x80) break;
      *(undefined4 *)(puVar12 + -0x20) = 0;
      ReleaseMutex(*(HANDLE *)(puVar12 + -0x24));
    }
    DVar19 = WaitForSingleObject(*(HANDLE *)(puVar12 + -0x28),1);
    if (DVar19 == 0) {
      DVar19 = WaitForSingleObject(pvVar3,1);
      if (DVar19 == 0) goto LAB_0054392e;
    }
    else {
      SetEvent(pvVar3);
LAB_0054392e:
      *(undefined4 *)(puVar12 + -0x20) = 0;
    }
    ReleaseMutex(*(HANDLE *)(puVar12 + -0x24));
  } while( true );
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
