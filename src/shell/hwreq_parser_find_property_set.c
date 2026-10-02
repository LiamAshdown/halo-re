// hwreq_parser_find_property_set  (Ghidra: FUN_005788f0, unnamed)
// address 0x5788f0, size 222 bytes
// name confidence: 0.8  rewrite confidence: 0.8
// evidence: hwreq_parser_vtable slot 0x0c (the vtable at 0x006721e8 points here, see
//   out/phase4/shell_types_notes.md). objdump 0x578948 loads EDI = this + 0x6a0 (the
//   property_sets map) before calling the map find helper 0x57b7a0, and 0x57895d compares the
//   result with this + 0x6a4 (property_sets.head). A found node returns its value (+0x28).
// register convention: __thiscall, this in ECX (mov ebp,ecx at 0x578914), name on the stack,
//   ret 4. Ghidra dropped every register argument of 0x57b7a0; objdump pins them: EDI = the map,
//   ESI = the key string (lea esi,[esp+0x10]) and EBX = an iterator slot the helper writes the
//   node into and returns (lea ebx,[esp+0x3c], the caller's own argument slot reused as scratch).
//   The slot is folded into the return value, as hwreq_parser_parse_block.c does.
// The key is a local std::string: it is set to the empty inline state (capacity 0xf, size 0,
//   first byte 0), assigned from name through 0x57bc90, and its heap buffer (capacity >= 0x10) is
//   freed on both exits. The SEH frame only guards that destructor and is not part of the C source.

// VERIFIED against disassembly 0x5788f0..0x5789ce (2026-09-30): strlen loop, assign_n(ECX=key), find(EDI map,ESI key,EBX slot), head compare, key freed on both exits
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

extern void msvc_string_assign_n(msvc_std_string *dest, const char *source, uint32_t length); // 0x57bc90, blam-cc: dest in ECX, source/length on the stack; library code, not in the function list
extern hwreq_map_node *hwreq_map_find(msvc_std_map *map, msvc_std_string *key); // 0x57b7a0, blam-cc: map in EDI, key in ESI, iterator slot in EBX (folded into the return value); library code (std::map<string,T*>::find); returns the found node or the map's head sentinel
extern void free(void *block); // 0x6277e8 CRT

// Looks name up in the parser's property_sets map ("propertyset" definitions) and returns the
// registered set, or NULL when there is no set of that name.
hwreq_property_set *hwreq_parser_find_property_set(hwreq_parser *self, const char *name)
{
    msvc_std_string key;
    hwreq_map_node *node;
    hwreq_property_set *result;
    uint32_t length = 0;

    key.capacity = k_msvc_string_inline_capacity;
    key.size = 0;
    key.buffer.inline_buffer[0] = 0;
    while (name[length] != 0) {
        length++;
    }
    msvc_string_assign_n(&key, name, length);

    node = hwreq_map_find(&self->property_sets, &key);
    if (node == (hwreq_map_node *)self->property_sets.head) {
        result = 0;
    } else {
        result = (hwreq_property_set *)node->value;
    }

    if (key.capacity >= 0x10) {
        free((void *)key.buffer.heap_buffer);
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x5788f0):

/* WARNING: Removing unreachable block (ram,0x005789aa) */
/* WARNING: Removing unreachable block (ram,0x00578974) */

undefined4 FUN_005788f0(char *param_1)

{
  char cVar1;
  char *pcVar2;
  int *piVar3;
  int in_ECX;
  void *local_c;
  undefined1 *puStack_8;
  undefined4 local_4;

  local_4 = 0xffffffff;
  puStack_8 = &LAB_00639368;
  local_c = ExceptionList;
  pcVar2 = param_1;
  do {
    cVar1 = *pcVar2;
    pcVar2 = pcVar2 + 1;
  } while (cVar1 != '\0');
  ExceptionList = &local_c;
  FUN_0057bc90(param_1,(int)pcVar2 - (int)(param_1 + 1));
  local_4 = 0;
  piVar3 = (int *)FUN_0057b7a0();
  if (*piVar3 == *(int *)(in_ECX + 0x6a4)) {
    ExceptionList = local_c;
    return 0;
  }
  ExceptionList = local_c;
  return *(undefined4 *)(*piVar3 + 0x28);
}
#endif
