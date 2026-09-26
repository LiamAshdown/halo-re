// write_to_error_file  (Ghidra: write_to_error_file, already named)
// address 0x449450, size 314 bytes
// name confidence: 0.85   rewrite confidence: 0.8  (reviewed line by line against objdump)
// evidence: out/phase2/cseries/00.md, out/phase4/cseries_types_notes.md; naming hint
// 'write_to_error_file' matches the code exactly, including the self-recursive header block
// that writes its own name ("_write_to_error_file") and its own address. Opens debug.txt via
// network_log_path_resolve 0x4e40a0 (Ghidra FUN_004e40a0) / FUN_00624186 (CRT fopen), optionally
// prefixes a localtime() timestamp, and fprintf's `message`.
// register convention: __cdecl(char *message, uint8_t with_timestamp), plain stack arguments,
// confirmed by the disassembly (`mov al,[esp+0x410]` reading with_timestamp from its stack slot).
//
// Decompiler correction (out/phase4/cseries_types_notes.md): Ghidra shows
// `FUN_004e40a0(&DAT_00660144)` followed by a one-argument `FUN_00624186(uVar1)`. The real
// sequence, per the disassembly (0x4494db..0x4494ec), is
// `FUN_00624186(network_log_path_resolve(ESI="debug.txt"), "a+b")`: ESI carries the file name
// into 0x4e40a0, and "a+b" (0x660144) is pushed once and stays on the stack across that call
// (FUN_004e40a0 does not touch it, since its own argument travels in ESI, not on the stack)
// until FUN_00624186's single `add esp,8` pops both the resolved path and the mode string.
// reconciled: R01 comment: 0x0087ac06 names merged to debug_log_level

#include "tags.h"
#include "cseries.h"
#include <time.h>

// Globals this module reads but does not own (see types/cseries.h "Globals this module reads
// but does not own"): the shell zeroes both at startup.
extern uint8_t debug_log_level;  // 0x0087ac06; every access in the image is byte-wide. R01
                                  // merged interface.h / networking.h onto this name and type.
extern uint8_t error_file_enabled; // 0x0087ac01, set to 1 by the shell startup; write_to_error_file
                                    // is its only reader in the image

extern uint8_t error_file_needs_header; // 0x00686b48, this module's own global (types/cseries.h)

// .rdata literals owned by this module (types/cseries.h)
extern char error_file_spacer[];            // 0x006601e8 "\r\n\r\n"
extern char error_file_banner[];            // 0x00660198 "halo pc 01.00.10.0621(CACHE) ----...\r\n"
extern char error_file_function_name[];     // 0x0066017c "_write_to_error_file"
extern char error_file_function_format[];   // 0x00660160 "reference function: %s\r\n"
extern char error_file_address_format[];    // 0x00660148 "reference address: %x\r\n"
extern char error_file_open_mode[];         // 0x00660144 "a+b"
extern char error_file_name[];              // 0x00660138 "debug.txt"
extern char error_file_timestamp_format[];  // 0x00660118 "%02d.%02d.%02d %02d:%02d:%02d  "
extern char error_file_no_timestamp[];      // 0x00660100 "<TIME UNAVAILABLE>  "

extern char *network_log_path_resolve(char *requested_path); // 0x4e40a0, foreign (networking,
    // src/networking/network_log_path_resolve.c); blam-cc: ESI -> requested_path. Formats the
    // name into the static char[0x104] at 0x006b85b8 and returns that buffer.
extern void *_fopen(const char *path, const char *mode); // 0x624186, fopen-shaped CRT wrapper
extern int32_t _fprintf(void *stream, const char *format, ...);              // 0x623de2
extern int32_t _sprintf(char *dest, const char *format, ...);                // 0x623693
extern void _fclose(void *file);                                              // 0x6241e5 _fclose
// _time32(__time32_t *) is declared by <time.h> above, matching the retail binary's
// 0x6240f1 FID_conflict:__time32 exactly (the 32-bit time() entry point, not the plain 64-bit
// time_t used elsewhere on this host).
extern struct tm *_localtime(const __time32_t *time_ptr);                    // 0x624886

// Appends `message` to the game's debug.txt log file, optionally prefixed with a
// "MM.DD.YY HH:MM:SS  " timestamp. Does nothing while the shell's debug log level is below 2.
// On the very first call that passes that check, first recursively logs a one-time header block
// (a blank line, a version banner, and this function's own name and address) before logging
// `message` itself; does nothing further if the error log is not enabled (0x0087ac01) or
// debug.txt cannot be opened for append.
void write_to_error_file(char *message, uint8_t with_timestamp)
{
    char formatted[0x400];
    void *file;
    char *path;
    __time32_t time_value;
    struct tm *local_time;

    if (debug_log_level < k_error_file_minimum_level) {
        return;
    }

    if (error_file_needs_header != 0) {
        error_file_needs_header = 0;
        write_to_error_file(error_file_spacer, 0);
        write_to_error_file(error_file_banner, 1);
        _sprintf(formatted, error_file_function_format, error_file_function_name);
        write_to_error_file(formatted, 1);
        _sprintf(formatted, error_file_address_format, (uint32_t)(size_t)write_to_error_file);
        write_to_error_file(formatted, 1);
    }

    if (error_file_enabled != 0) {
        path = network_log_path_resolve(error_file_name); // ESI = "debug.txt"
        file = _fopen(path, error_file_open_mode);
        if (file != 0) {
            if (with_timestamp != 0) {
                _time32(&time_value);
                local_time = _localtime(&time_value);
                if (local_time == 0) {
                    _fprintf(file, error_file_no_timestamp);
                } else {
                    _fprintf(file, error_file_timestamp_format, local_time->tm_mon + 1,
                             local_time->tm_mday, local_time->tm_year % 100,
                             local_time->tm_hour, local_time->tm_min, local_time->tm_sec);
                }
            }
            _fprintf(file, "%s", message);
            _fclose(file);
        }
    }
}

#if 0
Original Ghidra decompilation (0x449450), from tools/pack.py 0x449450:

void __cdecl write_to_error_file(char *message,char with_timestamp)

{
  undefined4 uVar1;
  FILE *_File;
  tm *ptVar2;
  undefined8 local_404;

  if (1 < DAT_0087ac06) {
    if (DAT_00686b48 != '\0') {
      DAT_00686b48 = '\0';
      write_to_error_file("\r\n\r\n",'\0');
      write_to_error_file("halo pc 01.00.10.0621(CACHE) ----------------------------------------------\r\n"
                          ,'\x01');
      _sprintf((char *)((int)&local_404 + 4),"reference function: %s\r\n","_write_to_error_file");
      write_to_error_file((char *)((int)&local_404 + 4),'\x01');
      _sprintf((char *)((int)&local_404 + 4),"reference address: %x\r\n",write_to_error_file);
      write_to_error_file((char *)((int)&local_404 + 4),'\x01');
    }
    if (DAT_0087ac01 != '\0') {
      uVar1 = FUN_004e40a0(&DAT_00660144);
      _File = (FILE *)FUN_00624186(uVar1);
      if (_File != (FILE *)0x0) {
        if (with_timestamp != '\0') {
          FID_conflict___time32((__time32_t *)&local_404);
          ptVar2 = _localtime(&local_404);
          if (ptVar2 == (tm *)0x0) {
            _fprintf(_File,"<TIME UNAVAILABLE>  ");
          }
          else {
            _fprintf(_File,"%02d.%02d.%02d %02d:%02d:%02d  ",ptVar2->tm_mon + 1,ptVar2->tm_mday,
                     ptVar2->tm_year % 100,ptVar2->tm_hour,ptVar2->tm_min,ptVar2->tm_sec);
          }
        }
        _fprintf(_File,"%s",message);
        _fclose(_File);
      }
    }
  }
  return;
}

Disassembly (objdump -d -M intel, bin/halo.exe) confirms the fopen/mode argument order:
  4494db: push esi
  4494dc: push 0x660144               ; "a+b" (stays on the stack across the next call)
  4494e1: mov esi,0x660138            ; "debug.txt"
  4494e6: call 0x4e40a0               ; FUN_004e40a0(ESI="debug.txt") -> resolved path in EAX
  4494eb: push eax
  4494ec: call 0x624186               ; FUN_00624186(path, "a+b")
  4494f1: mov esi,eax
  4494f3: add esp,0x8                 ; pops both the path and the mode string in one shot
#endif
