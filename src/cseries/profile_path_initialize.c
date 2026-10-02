// profile_path_initialize  (Ghidra: profile_path_initialize, already named)
// address 0x449390, size 185 bytes
// name confidence: 0.6   rewrite confidence: 0.8  (reviewed line by line against objdump)
// evidence: out/phase2/cseries/00.md, out/phase4/cseries_types_notes.md; checks the "-path"
// command-line flag to optionally override the profile directory; otherwise resolves
// CSIDL_PERSONAL through the shell's cached SHGetFolderPathA pointer and formats
// "%s\\My Games\\Halo" into the module's profile_directory global; falls back to "." plus a
// fatal-error dialog if the special-folder lookup fails. Runs once at startup (called from the
// shell entry point at 0x540f06, per the module's types notes).
// register convention: __cdecl, no arguments. Calls command_line_check_flag with "-path" pushed
// on the stack and EDI = the address of a local out_value slot (that call's own established
// convention, confirmed at 0x449397/0x44939c/0x4493a0); reads AL for the flag result and the
// out_value slot afterward, exactly as every other command_line_check_flag call site does.
//
// UNSURE / decompiler quirk (documented in out/phase4/cseries_types_notes.md): the
// printf("Using profile path %s.\n", profile_directory) call at 0x4493ce..0x4493dd runs BEFORE
// SHGetFolderPathA fills profile_directory, so it always prints an empty string: the shell zeroes
// the 0x105-byte block at 0x540ef9..0x540f05 just before calling this function (0x540f06), and
// the -path branch, the only other writer, returns before the printf. That is the shipped binary's actual
// behaviour, not a decompiler artifact -- Ghidra just drops the printf's second argument and the
// SHGetFolderPathA argument list, both of which the disassembly (0x4493ce..0x4493ed) restores.
// This rewrite reproduces the bug rather than "fixing" the argument order.

#include "crt.h"
#include "tags.h"
#include "cseries.h"
#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern char profile_directory[k_profile_directory_storage_size]; // 0x006ac900

// SHGetFolderPathA, resolved once by the shell startup code and cached as a raw FARPROC; see
// src/shell/shell_winmain.c / src/shell/engine_initialize_subsystems.c for the resolving code.
typedef int32_t (__stdcall *sh_get_folder_path_proc)(void *owner, int32_t csidl, void *token,
                                                       uint32_t flags, char *out_path);
extern void *sh_get_folder_path; // 0x0074626c FARPROC, foreign (shell module)

extern uint8_t command_line_check_flag(const char *flag_name, const char **out_value); // 0x542760, blam-cc: EDI out_value
// strncpy (0x623a90 strncpy) is declared by <string.h> above.
extern int32_t shell_display_fatal_error_dialog(uint32_t resource_id, uint32_t help_text_or_id, int32_t is_fatal); // 0x57ea70

// Determines and stores the player's profile directory path into the profile_directory global:
// a "-path" command-line override if present, otherwise "<CSIDL_PERSONAL>\My Games\Halo"; if
// even the special-folder lookup fails, falls back to "." and raises a fatal error dialog.
void profile_path_initialize(void)
{
    const char *path_argument;
    char documents_path[k_cseries_path_length];
    int32_t result;

    if (command_line_check_flag("-path", &path_argument) != 0 && path_argument != 0) {
        strncpy(profile_directory, path_argument, k_cseries_path_length);
        return;
    }

    printf("Using profile path %s.\n", profile_directory);
    result = ((sh_get_folder_path_proc)sh_get_folder_path)(0, k_csidl_personal, 0, 0,
                                                             documents_path);
    if (result >= 0) {
        _snprintf(profile_directory, k_cseries_path_length, "%s\\My Games\\Halo",
                   documents_path);
        return;
    }

    strncpy(profile_directory, ".", k_cseries_path_length);
    shell_display_fatal_error_dialog(k_profile_path_error_title, (uint32_t)((const char *)k_profile_path_error_message), 1);
}

#if 0
Original Ghidra decompilation (0x449390), from tools/pack.py 0x449390:

void profile_path_initialize(void)

{
  char cVar1;
  int iVar2;
  undefined4 uStack_11c;
  char *local_10c;

  cVar1 = command_line_check_flag();
  if ((cVar1 != '\0') && (local_10c != (char *)0x0)) {
    uStack_11c = 0x4493c4;
    _strncpy((char *)&DAT_006ac900,local_10c,0x104);
    return;
  }
  _printf("Using profile path %s.\n");
  uStack_11c = 5;
  iVar2 = (*DAT_0074626c)(0);
  if (-1 < iVar2) {
    __snprintf((char *)&DAT_006ac900,0x104,"%s\\My Games\\Halo",&uStack_11c);
    return;
  }
  _strncpy((char *)&DAT_006ac900,".",0x104);
  shell_display_fatal_error_dialog(0x8b,0x8c,1);
  return;
}

Disassembly (objdump -d -M intel, bin/halo.exe) fills in the arguments Ghidra dropped:
  449390: sub esp,0x10c
  449396: push edi
  449397: push 0x6600e0               ; "-path"
  44939c: lea edi,[esp+0x8]           ; edi = &local (out_value)
  4493a0: call 0x542760               ; command_line_check_flag("-path", edi)
  4493a5: add esp,0x4
  4493a8: test al,al
  4493aa: pop edi
  4493ab: je 0x4493ce
  4493ad: mov eax,[esp]               ; local out_value
  4493b0: test eax,eax
  4493b2: je 0x4493ce
  4493b4: push 0x104
  4493b9: push eax
  4493ba: push 0x6ac900               ; profile_directory
  4493bf: call 0x623a90               ; _strncpy(profile_directory, out_value, 0x104)
  4493c4: add esp,0xc
  4493c7: add esp,0x10c
  4493cd: ret
  4493ce: push 0x6ac900               ; profile_directory (still empty here)
  4493d3: push 0x6600c8               ; "Using profile path %s.\n"
  4493d8: call 0x62427c               ; _printf(fmt, profile_directory)
  4493dd: add esp,0x8
  4493e0: lea eax,[esp+0x4]           ; scratch buffer for SHGetFolderPathA
  4493e4: push eax
  4493e5: push 0x0                    ; dwFlags
  4493e7: push 0x0                    ; hToken
  4493e9: push 0x5                    ; CSIDL_PERSONAL
  4493eb: push 0x0                    ; hwndOwner
  4493ed: call DWORD PTR ds:0x74626c  ; SHGetFolderPathA(0, CSIDL_PERSONAL, 0, 0, buffer)
  4493f3: test eax,eax
  4493f5: jl 0x44941a
  4493f7: lea ecx,[esp+0x4]
  4493fb: push ecx
  4493fc: push 0x6600b4               ; "%s\\My Games\\Halo"
  449401: push 0x104
  449406: push 0x6ac900               ; profile_directory
  44940b: call 0x623a2d               ; __snprintf(profile_directory, 0x104, fmt, buffer)
  449410: add esp,0x10
  449413: add esp,0x10c
  449419: ret
  44941a: push 0x104
  44941f: push 0x6600b0               ; "."
  449424: push 0x6ac900               ; profile_directory
  449429: call 0x623a90               ; _strncpy(profile_directory, ".", 0x104)
  44942e: push 0x1
  449430: push 0x8c
  449435: push 0x8b
  44943a: call 0x57ea70               ; shell_display_fatal_error_dialog(0x8b, 0x8c, 1)
  44943f: add esp,0x18
  449442: add esp,0x10c
  449448: ret
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
