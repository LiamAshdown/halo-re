// message_delta_parameters_protocol_parse_value_from_config  (Ghidra: message_delta_parameters_protocol_parse_value_from_config, already named)
// address 0x4ec0d0, size 62 bytes
// name confidence: 0.5   rewrite confidence: 0.45
// evidence: out/phase4/networking_functions.md summary; message_delta_parameters_protocol_register
// calls this with only ("%d"|"%f", value) yet it clearly needs the parameter's name to look up in
// the config text -- strstr(&DAT_00860b40) is called with a single visible argument, so the
// needle is a register value (name) still live from the caller, matching the register-pinning
// pattern used throughout this subsystem; other modules already treat strstr(haystack,
// needle) as a substring search (src/networking/map_list_matching_substring.c).
// register convention: parameter name pinned in EAX (in_EAX, unresolved register read) by the
// caller, message_delta_parameters_protocol_register; format and out_value are the __cdecl stack
// parameters.
// blam-cc: EAX -> name, stack -> format, out_value
// UNSURE: strstr's exact semantics (declared here as a substring search) and what
// 0x0066e0788 [0x00660788] is a delimiter set for.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern char message_delta_config_text_buffer[]; // 0x00860b40
extern char message_delta_config_value_delimiters[]; // 0x00660788, UNSURE

extern char *strstr(const char *haystack, const char *needle); // 0x625430, UNSURE: substring search
extern int32_t sscanf(const char *buffer, const char *format, ...);

// blam-cc: EAX -> name, stack -> format, out_value
// Looks up `name`'s text value in the loaded parameters.cfg buffer and scans it into *out_value
// with the given scanf format. Returns 1 on a successful scan, 0 if the name was not found or
// the scan failed.
int32_t message_delta_parameters_protocol_parse_value_from_config(char *name, char *format, void *out_value)
{
    char *found;
    char *value_start;
    int32_t scanned;

    found = strstr(message_delta_config_text_buffer, name);
    if (found != 0) {
        value_start = strstr(found, message_delta_config_value_delimiters);
        scanned = sscanf(value_start + 1, format, out_value);
        if (0 < scanned) {
            return 1;
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4ec0d0):

int __cdecl message_delta_parameters_protocol_parse_value_from_config(char *format,void *out_value)

{
  int iVar1;
  uint uVar2;

  iVar1 = FUN_00625430(&DAT_00860b40);
  uVar2 = 0;
  if (iVar1 != 0) {
    iVar1 = FUN_00625430(iVar1,&DAT_00660788);
    iVar1 = _sscanf((char *)(iVar1 + 1),format,out_value);
    uVar2 = CONCAT31((int3)((uint)iVar1 >> 8),1);
    if (0 < iVar1) {
      return uVar2;
    }
  }
  return uVar2 & 0xffffff00;
}
#endif
