// string_compare  (already named; task-provided)
// address 0x57ce10, size 111 bytes
// name confidence: 0.5 (already carries this name; standard MSVC 7.1
//   std::basic_string<char>::compare(size_type pos, size_type n1, const char *s, size_type n2))
// rewrite confidence: 0.5 (standard library code; confirmed against objdump, including the
//   register convention, which Ghidra's own decompilation of this function did not show in
//   full -- see UNSURE)
// evidence: types/shell.h msvc_std_string. Sole caller in this batch is
//   hwreq_map_key_less_than.c (this pass), where objdump shows this function's true 5-value
//   call (ECX, EAX, and 3 stack args) even though Ghidra's own decompile of the caller only
//   printed the 3 stack ones.
// register convention (confirmed via objdump at the hwreq_map_key_less_than.c call site): ECX
//   = this (const msvc_std_string *), EAX = n1 (the clamp length, typically this->size), stack
//   arguments in order: pos, s (const char *), n2.
// blam-cc: string_compare(const msvc_std_string *this /*ECX*/, uint32_t n1 /*EAX*/,
//   uint32_t pos /*stack*/, const char *s /*stack*/, uint32_t n2 /*stack*/)
// UNSURE: FUN_00638e74 (`_Xlen`/`_Xran`, the length_error-throwing helper) is an opaque extern,
//   not rewritten here -- it is not in this pass's address list.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

extern void string_throw_length_error(void); // 0x638e74, UNSURE: opaque, not this pass; noreturn

int32_t string_compare(const msvc_std_string *this, uint32_t n1, uint32_t pos, const char *s, uint32_t n2)
{
    uint32_t remaining;
    uint32_t compare_count;
    int32_t result = 0;

    if (this->size < pos) {
        string_throw_length_error();
    }

    remaining = this->size - pos;
    if (remaining < n1) {
        n1 = remaining;
    }

    if (n1 != 0) {
        const char *lhs;
        compare_count = (n2 <= n1) ? n2 : n1;

        lhs = (this->capacity < 0x10) ? this->buffer.inline_buffer : (const char *)this->buffer.heap_buffer;
        lhs += pos;

        {
            uint32_t remaining_cmp = compare_count;
            uint8_t less = 0;
            uint8_t equal = 1;
            const uint8_t *l = (const uint8_t *)lhs;
            const uint8_t *r = (const uint8_t *)s;
            while (remaining_cmp != 0 && equal) {
                remaining_cmp--;
                less = *l < *r;
                equal = (*l == *r);
                l++;
                r++;
            }
            if (!equal) {
                result = less ? (uint32_t)-1 : 1;
            }
        }
        if (result != 0) {
            return result;
        }
    }

    if (n1 < n2) {
        return -1;
    }
    return (n1 != n2);
}

#if 0
Original Ghidra decompilation (0x57ce10):

uint string_compare(uint param_1,byte *param_2,uint param_3)

{
  uint in_EAX;
  uint uVar1;
  uint uVar2;
  int in_ECX;
  int iVar3;
  byte *pbVar4;
  bool bVar5;
  bool bVar6;

  if (*(uint *)(in_ECX + 0x14) < param_1) {
    FUN_00638e74();
  }
  uVar1 = *(int *)(in_ECX + 0x14) - param_1;
  if (uVar1 < in_EAX) {
    in_EAX = uVar1;
  }
  if (in_EAX != 0) {
    uVar1 = in_EAX;
    if (param_3 <= in_EAX) {
      uVar1 = param_3;
    }
    if (*(uint *)(in_ECX + 0x18) < 0x10) {
      iVar3 = in_ECX + 4;
    }
    else {
      iVar3 = *(int *)(in_ECX + 4);
    }
    bVar5 = false;
    uVar2 = 0;
    bVar6 = true;
    pbVar4 = (byte *)(iVar3 + param_1);
    do {
      if (uVar1 == 0) break;
      uVar1 = uVar1 - 1;
      bVar5 = *pbVar4 < *param_2;
      bVar6 = *pbVar4 == *param_2;
      pbVar4 = pbVar4 + 1;
      param_2 = param_2 + 1;
    } while (bVar6);
    if (!bVar6) {
      uVar2 = (1 - (uint)bVar5) - (uint)(bVar5 != 0);
    }
    if (uVar2 != 0) {
      return uVar2;
    }
  }
  if (param_3 <= in_EAX) {
    return (uint)(in_EAX != param_3);
  }
  return 0xffffffff;
}
#endif
