// hwreq_parser_destruct  (Ghidra: hwreq_parser_destruct, already named)
// address 0x57a010, size 525 bytes
// name confidence: 0.65  rewrite confidence: 0.8
// evidence: sets the vtable (already-destructed guard, matches every constructor site), frees
//   the flags / requirements property sets, walks and frees every value in the property_sets and
//   graphic_detail_sets maps (each value's flags vector of hwreq_string_pair, then the
//   hwreq_property_set itself), frees both map node arrays via tree_erase_range, and tears down
//   the five embedded strings -- the exact mirror of hwreq_parser_construct 0x579ef0. See
//   out/phase4/shell_types_notes.md.
// register convention: this passed on the stack, exactly like hwreq_parser_construct (objdump:
//   "mov edi,[esp+0x2c]" then "ret 0x4").
// blam-cc: FUN_0057b990 (declared here as hwreq_property_set_flags_destruct, library code, not
//   in the function list) takes the property set pointer in EBX, no return value (objdump: no
//   argument load from ECX/stack at its entry, every field access is [ebx+n]; call site here
//   loads the pointer into EBX immediately before the call). tree_iterator_increment 0x57c5e0
//   (library code) takes a pointer TO the iterator variable in EDX and updates it in place
//   (objdump: "lea edx,[esp+0x10]; call 0x57c5e0" then the caller re-reads [esp+0x10]).
// UNSURE: the compiler-generated x86 SEH frame / scope-index bookkeeping (ExceptionList, the
//   "local_4" scope index writes between every teardown step in the Ghidra output) is compiler
//   plumbing, not application logic, and is not part of the C source -- the observable calls, frees and
//   field writes are preserved exactly, in the same order.

// VERIFIED against disassembly 0x57a010..0x57a21d (2026-09-30): flags/requirements, both map walks and erase_range arg order, five strings
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

extern hwreq_parser_vtable hwreq_parser_vtable_instance; // 0x006721e8

extern void hwreq_property_set_flags_destruct(hwreq_property_set *set); // 0x57b990, blam-cc: set in EBX; library code (map neighbour), not in the function list
extern void hwreq_string_pair_destruct(hwreq_string_pair *pair); // 0x5785b0
extern hwreq_map_node **tree_erase_range(hwreq_map_node **out_iterator, hwreq_map_node *first,
                              hwreq_map_node *last, msvc_std_map *tree); // 0x57c310, src/shell; blam-cc: tree in ESI (objdump: reads
                              // [esi+4] with no this-load at entry, live from the caller); out_iterator,
                              // first, last on the stack, in that order. Parameter order follows src/shell/tree_erase_range.c. Library code (map neighbour of
                              // the skip-listed tree_* helpers), not in the function list.
                              // UNSURE: out_iterator's exact purpose; unused by every caller in this file.
extern void tree_iterator_increment(hwreq_map_node **iterator); // 0x57c5e0, blam-cc: iterator address in EDX, updated in place
extern void free(void *block); // 0x6277e8, CRT free

// Full destructor for the hardware-requirements parser object: frees the flags / requirements
// property sets, walks both maps freeing every value (property set) they own, discards both
// maps' node storage, and empties the five embedded strings -- everything
// hwreq_parser_construct 0x579ef0 set up.
void hwreq_parser_destruct(hwreq_parser *this)
{
    hwreq_map_node *head;
    hwreq_map_node *node;
    hwreq_property_set *set;
    hwreq_string_pair *pair;
    hwreq_string_pair *pair_end;

    this->vtable = (uint32_t)&hwreq_parser_vtable_instance;

    if (this->flags != 0) {
        hwreq_property_set_flags_destruct((hwreq_property_set *)this->flags);
        free((void *)this->flags);
    }
    if (this->requirements != 0) {
        hwreq_property_set_flags_destruct((hwreq_property_set *)this->requirements);
        free((void *)this->requirements);
    }

    head = (hwreq_map_node *)this->property_sets.head;
    for (node = (hwreq_map_node *)head->left; node != head; ) {
        set = (hwreq_property_set *)node->value;
        if (set != 0) {
            pair = (hwreq_string_pair *)set->flags.first;
            if (pair != 0) {
                pair_end = (hwreq_string_pair *)set->flags.last;
                for (; pair != pair_end; pair++) {
                    hwreq_string_pair_destruct(pair);
                }
                free((void *)set->flags.first);
            }
            set->flags.first = 0;
            set->flags.last = 0;
            set->flags.end = 0;
            free(set);
        }
        tree_iterator_increment(&node);
    }

    head = (hwreq_map_node *)this->graphic_detail_sets.head;
    tree_erase_range(&node, (hwreq_map_node *)head->left, head, &this->graphic_detail_sets);
    free((void *)this->graphic_detail_sets.head);
    this->graphic_detail_sets.head = 0;
    this->graphic_detail_sets.size = 0;

    head = (hwreq_map_node *)this->property_sets.head;
    tree_erase_range(&node, (hwreq_map_node *)head->left, head, &this->property_sets);
    free((void *)this->property_sets.head);
    this->property_sets.head = 0;
    this->property_sets.size = 0;

    if (this->sound_vendor_name.capacity > k_msvc_string_inline_capacity) {
        free((void *)this->sound_vendor_name.buffer.heap_buffer);
    }
    this->sound_vendor_name.capacity = k_msvc_string_inline_capacity;
    this->sound_vendor_name.size = 0;
    this->sound_vendor_name.buffer.inline_buffer[0] = 0;

    if (this->sound_device_name.capacity > k_msvc_string_inline_capacity) {
        free((void *)this->sound_device_name.buffer.heap_buffer);
    }
    this->sound_device_name.capacity = k_msvc_string_inline_capacity;
    this->sound_device_name.size = 0;
    this->sound_device_name.buffer.inline_buffer[0] = 0;

    if (this->graphics_vendor_name.capacity > k_msvc_string_inline_capacity) {
        free((void *)this->graphics_vendor_name.buffer.heap_buffer);
    }
    this->graphics_vendor_name.capacity = k_msvc_string_inline_capacity;
    this->graphics_vendor_name.size = 0;
    this->graphics_vendor_name.buffer.inline_buffer[0] = 0;

    if (this->graphics_device_name.capacity > k_msvc_string_inline_capacity) {
        free((void *)this->graphics_device_name.buffer.heap_buffer);
    }
    this->graphics_device_name.capacity = k_msvc_string_inline_capacity;
    this->graphics_device_name.size = 0;
    this->graphics_device_name.buffer.inline_buffer[0] = 0;

    if (this->error_message.capacity > k_msvc_string_inline_capacity) {
        free((void *)this->error_message.buffer.heap_buffer);
    }
    this->error_message.capacity = k_msvc_string_inline_capacity;
    this->error_message.size = 0;
    this->error_message.buffer.inline_buffer[0] = 0;
}

#if 0
Original Ghidra decompilation (0x57a010):

void hwreq_parser_destruct(undefined4 *param_1)

{
  void *pvVar1;
  int *piVar2;
  int iVar3;
  int iVar4;
  int *piVar5;
  int *local_10;
  void *local_c;
  undefined1 *puStack_8;
  int local_4;

  puStack_8 = &LAB_006394d4;
  local_c = ExceptionList;
  ExceptionList = &local_c;
  *param_1 = &PTR_LAB_006721e8;
  local_4 = 6;
  pvVar1 = (void *)param_1[6];
  if (pvVar1 != (void *)0x0) {
    FUN_0057b990();
    _free(pvVar1);
  }
  pvVar1 = (void *)param_1[7];
  if (pvVar1 != (void *)0x0) {
    FUN_0057b990();
    _free(pvVar1);
  }
  piVar5 = (int *)param_1[0x1a9];
  piVar2 = (int *)*piVar5;
  local_10 = piVar5;
  while (piVar2 != piVar5) {
    pvVar1 = (void *)piVar2[10];
    if (pvVar1 != (void *)0x0) {
      iVar4 = *(int *)((int)pvVar1 + 4);
      if (iVar4 != 0) {
        iVar3 = *(int *)((int)pvVar1 + 8);
        for (; iVar4 != iVar3; iVar4 = iVar4 + 0x38) {
          hwreq_string_pair_destruct(iVar4);
        }
        _free(*(void **)((int)pvVar1 + 4));
        piVar5 = local_10;
      }
      *(undefined4 *)((int)pvVar1 + 4) = 0;
      *(undefined4 *)((int)pvVar1 + 8) = 0;
      *(undefined4 *)((int)pvVar1 + 0xc) = 0;
      _free(pvVar1);
    }
    tree_iterator_increment();
  }
  local_4._1_3_ = (uint3)((uint)local_4 >> 8);
  local_4._0_1_ = 5;
  tree_erase_range(&local_10,*(undefined4 *)param_1[0x1ac],(undefined4 *)param_1[0x1ac]);
  _free((void *)param_1[0x1ac]);
  param_1[0x1ac] = 0;
  param_1[0x1ad] = 0;
  local_4._0_1_ = 4;
  tree_erase_range(&local_10,*(undefined4 *)param_1[0x1a9],(undefined4 *)param_1[0x1a9]);
  _free((void *)param_1[0x1a9]);
  param_1[0x1a9] = 0;
  param_1[0x1aa] = 0;
  local_4._0_1_ = 3;
  if (0xf < (uint)param_1[0x2b]) {
    _free((void *)param_1[0x26]);
  }
  param_1[0x2b] = 0xf;
  param_1[0x2a] = 0;
  *(undefined1 *)(param_1 + 0x26) = 0;
  local_4._0_1_ = 2;
  if (0xf < (uint)param_1[0x24]) {
    _free((void *)param_1[0x1f]);
  }
  param_1[0x24] = 0xf;
  param_1[0x23] = 0;
  *(undefined1 *)(param_1 + 0x1f) = 0;
  local_4._0_1_ = 1;
  if (0xf < (uint)param_1[0x1d]) {
    _free((void *)param_1[0x18]);
  }
  param_1[0x1d] = 0xf;
  param_1[0x1c] = 0;
  *(undefined1 *)(param_1 + 0x18) = 0;
  local_4 = (uint)local_4._1_3_ << 8;
  if (0xf < (uint)param_1[0x16]) {
    _free((void *)param_1[0x11]);
  }
  param_1[0x16] = 0xf;
  param_1[0x15] = 0;
  *(undefined1 *)(param_1 + 0x11) = 0;
  local_4 = 0xffffffff;
  if (0xf < (uint)param_1[0xf]) {
    _free((void *)param_1[10]);
  }
  param_1[0xf] = 0xf;
  param_1[0xe] = 0;
  *(undefined1 *)(param_1 + 10) = 0;
  ExceptionList = local_c;
  return;
}
#endif
