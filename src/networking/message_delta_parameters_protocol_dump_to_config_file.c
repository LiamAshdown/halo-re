// message_delta_parameters_protocol_dump_to_config_file  (Ghidra: message_delta_parameters_protocol_dump_to_config_file, already named)
// address 0x4ec330, size 90 bytes
// name confidence: 0.85   rewrite confidence: 0.7
// evidence: string "parameters.cfg"; tears down every message type's field bindings first (via
// message_delta_definitions_teardown_field_bindings, 0x4ec750) and clears the same 28-record
// table message_delta_protocol_initialize marks, unconditionally -- only the actual file write
// and the parameter-table free are gated on the parameters protocol being enabled.
// register convention: no register-passed arguments.

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint8_t message_delta_parameters_enabled;                // 0x0071cfa8
extern uint8_t message_delta_unknown_table_0069a304[28][0x18];  // 0x0069a304
extern char message_delta_config_text_buffer[];                 // 0x00860b40
extern char message_delta_config_write_mode_string[];           // 0x0066e674, fopen mode, UNSURE exact text

extern void message_delta_definitions_teardown_field_bindings(void); // 0x4ec750, this module
extern void message_delta_parameters_protocol_free_registered(void); // 0x4ebd50, this module

// Tears down every message type's field bindings and, if the dynamic-parameters protocol is
// enabled, writes the current formatted parameter values out to parameters.cfg and frees the
// registration table.
void message_delta_parameters_protocol_dump_to_config_file(void)
{
    int32_t i;
    void *file;

    message_delta_definitions_teardown_field_bindings();
    for (i = 0; i < 28; i++) {
        message_delta_unknown_table_0069a304[i][0] = 0;
    }
    if (message_delta_parameters_enabled == 1) {
        file = fopen("parameters.cfg", message_delta_config_write_mode_string);
        if (file != 0) {
            fprintf((FILE *)file, message_delta_config_text_buffer);
            fclose((FILE *)file);
        }
        message_delta_parameters_protocol_free_registered();
    }
}

#if 0
Original Ghidra decompilation (0x4ec330):

void message_delta_parameters_protocol_dump_to_config_file(void)

{
  undefined1 *puVar1;
  FILE *_File;

  FUN_004ec750();
  puVar1 = &DAT_0069a304;
  do {
    *puVar1 = 0;
    puVar1 = puVar1 + 0x18;
  } while ((int)puVar1 < 0x69a5a4);
  if (DAT_0071cfa8 == '\x01') {
    _File = (FILE *)FUN_00624186("parameters.cfg",&DAT_0066e674);
    if (_File != (FILE *)0x0) {
      _fprintf(_File,&DAT_00860b40);
      _fclose(_File);
    }
    message_delta_parameters_protocol_free_registered();
    return;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
