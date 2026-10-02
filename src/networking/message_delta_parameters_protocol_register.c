// message_delta_parameters_protocol_register  (Ghidra: message_delta_parameters_protocol_register, already named)
// address 0x4ebe00, size 328 bytes
// name confidence: 0.6   rewrite confidence: 0.55
// evidence: out/phase4/networking_types_notes.md "message delta protocol":
// message_delta_parameters_protocol_register strides 0x006b86c0 by 3 dwords (name, type,
// value pointer), matching message_delta_parameter in types/networking.h; the "<scope>::<name>"
// format is stated there too.
// register convention: an optional scope-prefix string in EAX (in_EAX, unresolved register
// read), then the __cdecl stack parameters name, type, value.
// blam-cc: EAX -> scope, stack -> name, type, value
// UNSURE: when scope is non-NULL, the byte immediately after the copied name is never written
// by this function (the GlobalAlloc call passes flag 0, not GMEM_ZEROINIT). This looks like a
// latent off-by-one in the original binary -- the allocation is exactly
// strlen(scope)+strlen(name)+3 bytes ("scope" + "::" + "name" + NUL) but only the first
// strlen(scope)+2+strlen(name) bytes are ever written -- and is preserved here rather than
// "fixed", per the no-invented-behaviour rule.

#include "win32.h"
#include <string.h>
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint8_t message_delta_parameters_enabled;              // 0x0071cfa8
extern int32_t message_delta_parameter_count;                 // 0x0071cfb0
extern message_delta_parameter message_delta_parameters[];    // 0x006b86c0

extern char message_delta_parameters_protocol_find_registered(char *name, void **out_value); // 0x4ec110, this module
extern int32_t message_delta_parameters_protocol_parse_value_from_config(char *format, void *out_value); // 0x4ec0d0, this module

// blam-cc: EAX -> scope, stack -> name, type, value
// Registers a named dynamic tunable parameter (type 1 == int, otherwise float). When scope is
// non-NULL the stored name is "<scope>::<name>"; otherwise it is a duplicate of name. If the
// name is not already registered, appends a new (name, type, value) row. Either way, seeds
// *value from the previously-loaded parameters.cfg text via the appropriate scanf format.
void message_delta_parameters_protocol_register(char *scope, char *name, int32_t type, void *value)
{
    char *buffer;

    if (message_delta_parameters_enabled == 1) {
        if (scope == 0) {
            buffer = (char *)GlobalAlloc(0, strlen(name) + 1);
            strcpy(buffer, strdup(name));
        } else {
            int32_t scope_len = strlen(scope);
            int32_t name_len = strlen(name);
            buffer = (char *)GlobalAlloc(0, scope_len + name_len + 3);
            memcpy(buffer, scope, scope_len + 1); // includes scope's NUL
            buffer[scope_len] = ':';
            buffer[scope_len + 1] = ':';
            buffer[scope_len + 2] = '\0';
            memcpy(buffer + scope_len + 2, name, name_len); // UNSURE: no trailing NUL written, see header
        }
        if (message_delta_parameters_protocol_find_registered(buffer, 0) == 0) {
            message_delta_parameters[message_delta_parameter_count].name = buffer;
            message_delta_parameters[message_delta_parameter_count].type = type;
            message_delta_parameters[message_delta_parameter_count].value = value;
            message_delta_parameter_count = message_delta_parameter_count + 1;
        }
        if (type == 1) {
            message_delta_parameters_protocol_parse_value_from_config((char *)"%d", value);
        } else {
            message_delta_parameters_protocol_parse_value_from_config((char *)"%f", value);
        }
    }
}

#if 0
Original Ghidra decompilation (0x4ebe00):

void __cdecl message_delta_parameters_protocol_register(char *name,int type,void *value)

{
  char cVar1;
  char *in_EAX;
  char *pcVar2;
  char *pcVar3;
  uint uVar4;
  int iVar5;
  char *pcVar6;
  char *pcVar7;

  if (DAT_0071cfa8 == '\x01') {
    if (in_EAX == (char *)0x0) {
      pcVar2 = name;
      do {
        cVar1 = *pcVar2;
        pcVar2 = pcVar2 + 1;
      } while (cVar1 != '\0');
      pcVar3 = GlobalAlloc(0,(SIZE_T)(pcVar2 + (1 - (int)(name + 1))));
      pcVar7 = __strdup(name);
      pcVar2 = pcVar3;
      do {
        cVar1 = *pcVar7;
        pcVar7 = pcVar7 + 1;
        *pcVar2 = cVar1;
        pcVar2 = pcVar2 + 1;
      } while (cVar1 != '\0');
    }
    else {
      pcVar2 = in_EAX;
      do {
        cVar1 = *pcVar2;
        pcVar2 = pcVar2 + 1;
      } while (cVar1 != '\0');
      pcVar3 = name;
      do {
        cVar1 = *pcVar3;
        pcVar3 = pcVar3 + 1;
      } while (cVar1 != '\0');
      pcVar3 = GlobalAlloc(0,(SIZE_T)(pcVar2 + (int)(pcVar3 + ((3 - (int)(name + 1)) -
                                                              (int)(in_EAX + 1)))));
      iVar5 = (int)pcVar3 - (int)in_EAX;
      do {
        cVar1 = *in_EAX;
        in_EAX[iVar5] = cVar1;
        in_EAX = in_EAX + 1;
      } while (cVar1 != '\0');
      pcVar2 = pcVar3 + -1;
      do {
        pcVar7 = pcVar2;
        pcVar2 = pcVar7 + 1;
      } while (pcVar7[1] != '\0');
      pcVar2[0] = ':';
      pcVar2[1] = ':';
      pcVar7[3] = '\0';
      pcVar2 = name;
      do {
        cVar1 = *pcVar2;
        pcVar2 = pcVar2 + 1;
      } while (cVar1 != '\0');
      pcVar7 = pcVar3 + -1;
      do {
        pcVar6 = pcVar7 + 1;
        pcVar7 = pcVar7 + 1;
      } while (*pcVar6 != '\0');
      pcVar6 = name;
      for (uVar4 = (uint)((int)pcVar2 - (int)name) >> 2; uVar4 != 0; uVar4 = uVar4 - 1) {
        *(undefined4 *)pcVar7 = *(undefined4 *)pcVar6;
        pcVar6 = pcVar6 + 4;
        pcVar7 = pcVar7 + 4;
      }
      for (uVar4 = (int)pcVar2 - (int)name & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
        *pcVar7 = *pcVar6;
        pcVar6 = pcVar6 + 1;
        pcVar7 = pcVar7 + 1;
      }
    }
    cVar1 = message_delta_parameters_protocol_find_registered(pcVar3,0);
    if (cVar1 == '\0') {
      (&DAT_006b86c0)[DAT_0071cfb0 * 3] = pcVar3;
      (&DAT_006b86c4)[DAT_0071cfb0 * 3] = type;
      (&DAT_006b86c8)[DAT_0071cfb0 * 3] = value;
      DAT_0071cfb0 = DAT_0071cfb0 + 1;
    }
    if (type == 1) {
      message_delta_parameters_protocol_parse_value_from_config("%d",value);
      return;
    }
    message_delta_parameters_protocol_parse_value_from_config("%f",value);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
