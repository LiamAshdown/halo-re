// hwreq_property_set_upsert  (Ghidra: FUN_00578410; chosen name, see the module notes)
// address 0x578410, size 411 bytes
// name confidence: 0.4 (Ghidra) / 0.55 (this name)   rewrite confidence: 0.75
// evidence: shell.h's notes: "0x578410 (upsert: walks begin +4 .. end +8, stricmp on the key,
// re-assigns the value or push_back)"; the stride (0x38), the <0x10 inline-buffer test and the
// capacity/size/buffer field order all match hwreq_string_pair / msvc_std_string exactly.
// register convention: property_set in the recognized 1st stack parameter, key in the 2nd,
// value in the 3rd (Ghidra already recovered all three as normal parameters here).
// Review fix (objdump 0x578410..0x5785a8): both string writes are std::string::assign 0x57bc90
//   with the destination in ECX (lea ecx,[esi+0x1c] at 0x57858b for the found pair's value;
//   lea ecx,[esp+..] on fresh empty locals for the new pair), so they are declared as the shared
//   msvc_string_assign_n extern like the other hwreq files instead of a private reimplementation.
//   The existing value keeps its buffer when it fits (the private helper freed and reallocated).
// register convention: __stdcall, ret 0xc.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

extern int32_t __stricmp(const char *a, const char *b); // 0x628d8b CRT
extern void free(void *block); // 0x6277e8 CRT
extern void msvc_string_assign_n(msvc_std_string *dest, const char *source, uint32_t length); // 0x57bc90, blam-cc: dest in ECX, source/length on the stack; library code, not in the function list

extern hwreq_string_pair *hwreq_string_pair_construct(hwreq_string_pair *dest, msvc_std_string *first,
                                                        msvc_std_string *second); // 0x0057b670
extern void hwreq_device_list_push_back(hwreq_property_set *property_set, hwreq_string_pair *pair); // 0x0057b5e0, set in EAX, pair on the stack
extern void hwreq_string_pair_destruct(hwreq_string_pair *pair); // 0x005785b0, this module

static uint32_t hwreq_string_length(const char *text)
{
    const char *p = text;
    while (*p != '\0') p++;
    return (uint32_t)(p - text);
}

// Looks up param_2 in a linear key/value list of device-override strings and, if absent,
// inserts a new (key, value) entry; if present, only rewrites the value's storage without
// changing the key.
void hwreq_property_set_upsert(hwreq_property_set *property_set, char *key, char *value)
{
    hwreq_string_pair *cursor;
    hwreq_string_pair *end;
    const char *first_text;

    cursor = (hwreq_string_pair *)property_set->flags.first;
    end = (hwreq_string_pair *)property_set->flags.last;

    while (cursor != end) {
        first_text = (cursor->first.capacity < 0x10) ? cursor->first.buffer.inline_buffer
                                                       : (const char *)cursor->first.buffer.heap_buffer;
        if (__stricmp(first_text, key) == 0) {
            msvc_string_assign_n(&cursor->second, value, hwreq_string_length(value));
            return;
        }
        cursor++;
    }

    {
        msvc_std_string key_string;
        msvc_std_string value_string;
        hwreq_string_pair new_pair;

        value_string.capacity = k_msvc_string_inline_capacity;
        value_string.size = 0;
        value_string.buffer.inline_buffer[0] = 0;
        msvc_string_assign_n(&value_string, value, hwreq_string_length(value));
        key_string.capacity = k_msvc_string_inline_capacity;
        key_string.size = 0;
        key_string.buffer.inline_buffer[0] = 0;
        msvc_string_assign_n(&key_string, key, hwreq_string_length(key));

        hwreq_string_pair_construct(&new_pair, &key_string, &value_string);
        hwreq_device_list_push_back(property_set, &new_pair);
        hwreq_string_pair_destruct(&new_pair);

        if (value_string.capacity > k_msvc_string_inline_capacity) {
            free((void *)value_string.buffer.heap_buffer);
        }
        if (key_string.capacity > k_msvc_string_inline_capacity) {
            free((void *)key_string.buffer.heap_buffer);
        }
    }
}

#if 0
Original Ghidra decompilation (0x578410):


void FUN_00578410(int param_1,char *param_2,char *param_3)

{
  char cVar1;
  int iVar2;
  void **ppvVar3;
  int iVar4;
  char *pcVar5;
  undefined4 uVar6;
  int iVar7;
  undefined1 local_7c [4];
  void *local_78;
  undefined4 local_68;
  uint local_64;
  undefined1 local_60 [4];
  void *local_5c;
  undefined4 local_4c;
  uint local_48;
  undefined1 local_44 [56];
  void *local_c;
  undefined1 *puStack_8;
  int local_4;
  
  local_4 = 0xffffffff;
  puStack_8 = &LAB_006394fa;
  iVar7 = *(int *)(param_1 + 4);
  iVar2 = *(int *)(param_1 + 8);
  ppvVar3 = &local_c;
  local_c = ExceptionList;
  while( true ) {
    ExceptionList = ppvVar3;
    if (iVar7 == iVar2) {
      local_48 = 0xf;
      local_4c = 0;
      local_5c = (void *)((uint)local_5c & 0xffffff00);
      pcVar5 = param_3;
      do {
        cVar1 = *pcVar5;
        pcVar5 = pcVar5 + 1;
      } while (cVar1 != '\0');
      FUN_0057bc90(param_3,(int)pcVar5 - (int)(param_3 + 1));
      local_4 = 0;
      local_64 = 0xf;
      local_68 = 0;
      local_78 = (void *)((uint)local_78 & 0xffffff00);
      pcVar5 = param_2;
      do {
        cVar1 = *pcVar5;
        pcVar5 = pcVar5 + 1;
      } while (cVar1 != '\0');
      FUN_0057bc90(param_2,(int)pcVar5 - (int)(param_2 + 1));
      local_4._0_1_ = 1;
      uVar6 = hwreq_string_pair_construct(local_44,local_7c,local_60);
      local_4._0_1_ = 2;
      hwreq_device_list_push_back(uVar6);
      local_4._0_1_ = 1;
      hwreq_string_pair_destruct(local_44);
      local_4 = (uint)local_4._1_3_ << 8;
      if (0xf < local_64) {
        _free(local_78);
      }
      local_4 = 0xffffffff;
      local_64 = 0xf;
      local_68 = 0;
      local_78 = (void *)((uint)local_78 & 0xffffff00);
      if (0xf < local_48) {
        _free(local_5c);
      }
      ExceptionList = local_c;
      return;
    }
    if (*(uint *)(iVar7 + 0x18) < 0x10) {
      pcVar5 = (char *)(iVar7 + 4);
    }
    else {
      pcVar5 = *(char **)(iVar7 + 4);
    }
    iVar4 = __stricmp(pcVar5,param_2);
    if (iVar4 == 0) break;
    iVar7 = iVar7 + 0x38;
    ppvVar3 = ExceptionList;
  }
  pcVar5 = param_3;
  do {
    cVar1 = *pcVar5;
    pcVar5 = pcVar5 + 1;
  } while (cVar1 != '\0');
  FUN_0057bc90(param_3,(int)pcVar5 - (int)(param_3 + 1));
  ExceptionList = local_c;
  return;
}
#endif
