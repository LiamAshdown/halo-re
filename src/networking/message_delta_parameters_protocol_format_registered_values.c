// message_delta_parameters_protocol_format_registered_values  (Ghidra: message_delta_parameters_protocol_format_registered_values, already named)
// address 0x4ec050, size 115 bytes
// name confidence: 0.5   rewrite confidence: 0.7
// evidence: strings "%s %d\n" / "%s %f\n"; writes through message_delta_parameters[] (see
// message_delta_parameters_protocol_free_registered.c) into the shared config text buffer that
// message_delta_parameters_protocol_dump_to_config_file later writes out to parameters.cfg.
// register convention: no register-passed arguments.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int32_t message_delta_parameter_count;               // 0x0071cfb0
extern message_delta_parameter message_delta_parameters[];  // 0x006b86c0
extern char message_delta_config_text_buffer[];              // 0x00860b40

extern int32_t sprintf(char *buffer, const char *format, ...);

// Serializes every registered dynamic parameter's current name/value pair as a "name value\n"
// line into the shared config text buffer.
void message_delta_parameters_protocol_format_registered_values(void)
{
    int32_t offset;
    int32_t i;
    int32_t written;

    offset = 0;
    for (i = 0; i < message_delta_parameter_count; i++) {
        if (message_delta_parameters[i].type == 1) {
            written = sprintf(message_delta_config_text_buffer + offset, "%s %d\n",
                               message_delta_parameters[i].name, *(int32_t *)message_delta_parameters[i].value);
        } else {
            written = sprintf(message_delta_config_text_buffer + offset, "%s %f\n",
                               message_delta_parameters[i].name, (double)*(float *)message_delta_parameters[i].value);
        }
        offset = offset + written;
    }
    message_delta_config_text_buffer[offset] = 0;
}

#if 0
Original Ghidra decompilation (0x4ec050):

void message_delta_parameters_protocol_format_registered_values(void)

{
  int iVar1;
  int iVar2;
  undefined4 *puVar3;
  int iVar4;

  iVar4 = 0;
  iVar2 = 0;
  if (0 < DAT_0071cfb0) {
    puVar3 = &DAT_006b86c8;
    do {
      if (puVar3[-1] == 1) {
        iVar1 = _sprintf(&DAT_00860b40 + iVar4,"%s %d\n",puVar3[-2],*(undefined4 *)*puVar3);
      }
      else {
        iVar1 = _sprintf(&DAT_00860b40 + iVar4,"%s %f\n",puVar3[-2],(double)*(float *)*puVar3);
      }
      iVar4 = iVar4 + iVar1;
      iVar2 = iVar2 + 1;
      puVar3 = puVar3 + 3;
    } while (iVar2 < DAT_0071cfb0);
  }
  (&DAT_00860b40)[iVar4] = 0;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
