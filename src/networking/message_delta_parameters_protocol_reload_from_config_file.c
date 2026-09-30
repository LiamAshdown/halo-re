// message_delta_parameters_protocol_reload_from_config_file  (Ghidra: message_delta_parameters_protocol_reload_from_config_file, already named)
// address 0x4ebda0, size 96 bytes
// name confidence: 0.85   rewrite confidence: 0.8
// evidence: string "parameters.cfg"; the same shared text buffer (0x00860b40) is written by
// _format_registered_values/FUN_004ec230 and consumed by _parse_value_from_config, and dumped
// back out by _dump_to_config_file.
// register convention: no register-passed arguments.
// UNSURE: the fopen mode string at 0x0066e660 and the buffer's declared capacity are not
// established by this function alone (it trusts the file to fit).

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "fn_networking.h"

extern uint8_t message_delta_parameters_enabled; // 0x0071cfa8
extern char message_delta_config_mode_string[];  // 0x0066e660, fopen mode, UNSURE exact text
extern char message_delta_config_text_buffer[];  // 0x00860b40, shared parameters.cfg text


// Reads the whole of parameters.cfg into the shared config text buffer, NUL-terminated, so
// message_delta_parameters_protocol_parse_value_from_config can scan values out of it later.
void message_delta_parameters_protocol_reload_from_config_file(void)
{
    void *file;
    int32_t length;

    if (message_delta_parameters_enabled == 1) {
        file = fopen("parameters.cfg", message_delta_config_mode_string);
        if (file != 0) {
            fseek(file, 0, 2);
            length = ftell(file);
            fseek(file, 0, 0);
            fread(message_delta_config_text_buffer, 1, length, file);
            message_delta_config_text_buffer[length] = 0;
            fclose(file);
        }
    }
}

#if 0
Original Ghidra decompilation (0x4ebda0):

void message_delta_parameters_protocol_reload_from_config_file(void)

{
  FILE *_File;
  size_t _Count;

  if ((DAT_0071cfa8 == '\x01') &&
     (_File = (FILE *)FUN_00624186("parameters.cfg",&DAT_0066e660), _File != (FILE *)0x0)) {
    _fseek(_File,0,2);
    _Count = _ftell(_File);
    _fseek(_File,0,0);
    _fread(&DAT_00860b40,1,_Count,_File);
    (&DAT_00860b40)[_Count] = 0;
    _fclose(_File);
  }
  return;
}
#endif
