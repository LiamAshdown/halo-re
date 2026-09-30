// hwreq_device_override_list_find  (Ghidra: hwreq_device_override_list_find, already named)
// address 0x578630, size 98 bytes
// name confidence: 0.55   rewrite confidence: 0.7
// evidence: walks a property_set's flags vector (+4 begin, +8 end, stride 0x38), case
// insensitively matching each pair's first string, and strncpy's the matching pair's second
// string out to the caller.
// register convention: property_set in EAX, key name in EBX, then the stack parameters
// (out_buffer, capacity).
// blam-cc: EAX -> property_set (1st parameter), EBX -> key (2nd parameter)

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"
#include "fn_shell.h"


// Looks up a key in the device-override list and, if found, copies its associated value string
// into the caller-supplied buffer.
uint32_t hwreq_device_override_list_find(hwreq_property_set *property_set, const char *key,
                                          char *out_value, uint32_t capacity)
{
    hwreq_string_pair *cursor;
    hwreq_string_pair *end;
    const char *first_text;
    const char *second_text;

    cursor = (hwreq_string_pair *)property_set->flags.first;
    end = (hwreq_string_pair *)property_set->flags.last;

    while (cursor != end) {
        first_text = (cursor->first.capacity < 0x10) ? cursor->first.buffer.inline_buffer
                                                       : (const char *)cursor->first.buffer.heap_buffer;
        if (_stricmp(first_text, key) == 0) {
            break;
        }
        cursor++;
    }
    if (cursor == end) {
        return 0;
    }

    second_text = (cursor->second.capacity < 0x10) ? cursor->second.buffer.inline_buffer
                                                     : (const char *)cursor->second.buffer.heap_buffer;
    strncpy(out_value, second_text, capacity);
    return 1;
}

#if 0
Original Ghidra decompilation (0x578630):


undefined4 hwreq_device_override_list_find(char *param_1,size_t param_2)

{
  int iVar1;
  int in_EAX;
  int iVar2;
  char *unaff_EBX;
  int iVar3;
  char *pcVar4;
  
  iVar3 = *(int *)(in_EAX + 4);
  iVar1 = *(int *)(in_EAX + 8);
  while( true ) {
    if (iVar3 == iVar1) {
      return 0;
    }
    if (*(uint *)(iVar3 + 0x18) < 0x10) {
      pcVar4 = (char *)(iVar3 + 4);
    }
    else {
      pcVar4 = *(char **)(iVar3 + 4);
    }
    iVar2 = __stricmp(pcVar4,unaff_EBX);
    if (iVar2 == 0) break;
    iVar3 = iVar3 + 0x38;
  }
  if (*(uint *)(iVar3 + 0x34) < 0x10) {
    pcVar4 = (char *)(iVar3 + 0x20);
  }
  else {
    pcVar4 = *(char **)(iVar3 + 0x20);
  }
  _strncpy(param_1,pcVar4,param_2);
  return 1;
}
#endif
