// network_player_update_history_log_write  (Ghidra: network_player_update_history_log_write,
// already named)
// address 0x4e7f90, size 95 bytes
// name confidence: 0.75   rewrite confidence: 0.8
// evidence: out/phase4/networking_functions.md; literal string "ServerPlayerUpdateHistory.log";
// player_update_history_log_write.c (this batch's sibling client-side logger, same fopen wrapper
// and mode string).
// register convention: none; __cdecl, format plus varargs on the stack.
// UNSURE: DAT_00710320's exact meaning beyond "server player-update-history logging enabled".

#include "tags.h"
#include "memory.h"
#include <stdio.h>
#include <stdarg.h>

extern uint8_t network_player_update_log_enabled; // 0x00710320
extern char *network_player_update_history_log_path; // 0x0069a2c8, "ServerPlayerUpdateHistory.log"
extern char network_player_update_log_file_mode_string[]; // 0x0066b87c, "a"

// fopen: <stdio.h>, resolved to the game CRT at 0x624186 // 0x624186, fopen-shaped CRT wrapper

// Formats format (with its trailing varargs) into a scratch buffer, then appends it to
// ServerPlayerUpdateHistory.log when server player-update-history logging is enabled.
void network_player_update_history_log_write(const char *format, ...)
{
    char buffer[0x400];
    va_list args;
    FILE *file;

    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);
    if (network_player_update_log_enabled == 1) {
        file = (FILE *)fopen(network_player_update_history_log_path,
            network_player_update_log_file_mode_string);
        if (file != 0) {
            fprintf(file, buffer);
            fclose(file);
        }
    }
}

#if 0
Original Ghidra decompilation (0x4e7f90), from tools/pack.py 0x4e7f90:

void __cdecl network_player_update_history_log_write(char *format,...)

{
  FILE *_File;
  char local_400 [1024];

  _vsprintf(local_400,format,&stack0x00000008);
  if ((DAT_00710320 == '\x01') &&
     (_File = (FILE *)FUN_00624186(PTR_s_ServerPlayerUpdateHistory_log_0069a2c8,&DAT_0066b87c),
     _File != (FILE *)0x0)) {
    _fprintf(_File,local_400);
    _fclose(_File);
  }
  return;
}
#endif
