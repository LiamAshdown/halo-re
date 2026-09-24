// message_delta_parameters_protocol_format_received_values  (Ghidra: FUN_004ec230; named per this rewrite)
// address 0x4ec230, size 177 bytes
// name confidence: 0.45   rewrite confidence: 0.55
// evidence: out/phase4/networking_functions.md summary: "Formats a decoded array of parameter
// values back into 'name value' text lines, used after receiving a parameters-protocol update."
// Mirrors message_delta_parameters_protocol_format_registered_values but reads each value out of
// the caller-supplied decoded array instead of message_delta_parameters[i].value.
// register convention: decoded value array as the recognized parameter (param_1).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern int32_t message_delta_parameter_count;               // 0x0071cfb0
extern message_delta_parameter message_delta_parameters[];  // 0x006b86c0
extern char message_delta_config_text_buffer[];              // 0x00860b40

extern int32_t sprintf(char *buffer, const char *format, ...);
extern void *memcpy(void *dest, const void *src, int32_t count);

// Serializes a decoded array of parameter values back into "name value\n" text lines (in
// registration order), used after message_delta_parameters_protocol_receive_update decodes an
// incoming update.
void message_delta_parameters_protocol_format_received_values(int32_t *values)
{
    char *dest;
    int32_t i;
    int32_t name_len;
    int32_t written;

    dest = message_delta_config_text_buffer;
    for (i = 0; i < message_delta_parameter_count; i++) {
        char *name = message_delta_parameters[i].name;
        for (name_len = 0; name[name_len] != 0; name_len++) {
        }
        memcpy(dest, name, name_len);
        dest[name_len] = ' ';
        dest = dest + name_len + 1;
        if (message_delta_parameters[i].type == 1) {
            written = sprintf(dest, "%d\n"); // UNSURE: no value argument is passed here either --
                                              // transcribed exactly, see #if 0 block
        } else {
            written = sprintf(dest, "%f\n", (double)*(float *)&values[i]);
        }
        dest = dest + written;
    }
}

#if 0
Original Ghidra decompilation (0x4ec230):

void FUN_004ec230(int param_1)

{
  char cVar1;
  char *pcVar2;
  uint uVar3;
  int iVar4;
  uint uVar5;
  char *pcVar6;
  int iVar7;
  char *pcVar8;
  undefined4 *local_4;

  iVar7 = 0;
  pcVar6 = &DAT_00860b40;
  if (0 < DAT_0071cfb0) {
    local_4 = &DAT_006b86c0;
    do {
      pcVar8 = (char *)*local_4;
      pcVar2 = pcVar8;
      do {
        cVar1 = *pcVar2;
        pcVar2 = pcVar2 + 1;
      } while (cVar1 != '\0');
      uVar3 = (int)pcVar2 - (int)(pcVar8 + 1);
      pcVar2 = pcVar6;
      for (uVar5 = uVar3 >> 2; uVar5 != 0; uVar5 = uVar5 - 1) {
        *(undefined4 *)pcVar2 = *(undefined4 *)pcVar8;
        pcVar8 = pcVar8 + 4;
        pcVar2 = pcVar2 + 4;
      }
      for (uVar5 = uVar3 & 3; uVar5 != 0; uVar5 = uVar5 - 1) {
        *pcVar2 = *pcVar8;
        pcVar8 = pcVar8 + 1;
        pcVar2 = pcVar2 + 1;
      }
      pcVar6[uVar3] = ' ';
      pcVar6 = pcVar6 + uVar3 + 1;
      if (local_4[1] == 1) {
        iVar4 = _sprintf(pcVar6,"%d\n");
      }
      else {
        iVar4 = _sprintf(pcVar6,"%f\n",(double)*(float *)(param_1 + iVar7 * 4));
      }
      pcVar6 = pcVar6 + iVar4;
      iVar7 = iVar7 + 1;
      local_4 = local_4 + 3;
    } while (iVar7 < DAT_0071cfb0);
  }
  return;
}
#endif
