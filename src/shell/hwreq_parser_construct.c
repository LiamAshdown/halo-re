// hwreq_parser_construct  (Ghidra: hwreq_parser_construct, already named)
// address 0x579ef0, size 235 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: sets the vtable to 0x006721e8 (hwreq_parser_vtable), default-constructs the five
//   msvc_std_string members (capacity 0xf, size 0, empty buffer byte) and the two map heads
//   (tree_head_node_allocate, is_nil = 1, self-linked left/parent/right, size 0), matching every
//   field hwreq_parser_destruct 0x57a010 tears back down. See out/phase4/shell_types_notes.md.
// register convention: this passed on the stack (Ghidra already recognizes it as param_1;
//   objdump confirms "mov esi,[esp+0x18]" then "ret 0x4", i.e. a plain one-argument call, not a
//   register this).
// UNSURE: the compiler-generated x86 SEH frame (ExceptionList / scope-index bookkeeping around
//   the two tree_head_node_allocate calls) is compiler plumbing, not application logic, and is
//   not part of the C source -- the observable calls and their order are preserved exactly.

// VERIFIED against disassembly 0x579ef0..0x579fdb (2026-09-30): offsets 0x28/0x44/0x60/0x7c/0x98 strings, heads 0x6a4/0x6b0, flags/requirements 0x18/0x1c
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern hwreq_parser_vtable hwreq_parser_vtable_instance; // 0x006721e8

extern hwreq_map_node *tree_head_node_allocate(void); // 0x57cbf0, allocates a 0x30 byte sentinel node (left/parent/right zeroed, color = black, is_nil = 0)

// Default-constructs the hardware-requirements parser object in place: installs the vtable,
// empties the five embedded strings, and allocates and self-links the head sentinel node of
// each of the two maps (property_sets, graphic_detail_sets), marking each head is_nil.
hwreq_parser *hwreq_parser_construct(hwreq_parser *this_)
{
    hwreq_map_node *head;

    this_->vtable = (uint32_t)&hwreq_parser_vtable_instance;

    this_->error_message.capacity = k_msvc_string_inline_capacity;
    this_->error_message.size = 0;
    this_->error_message.buffer.inline_buffer[0] = 0;

    this_->graphics_device_name.capacity = k_msvc_string_inline_capacity;
    this_->graphics_device_name.size = 0;
    this_->graphics_device_name.buffer.inline_buffer[0] = 0;

    this_->graphics_vendor_name.capacity = k_msvc_string_inline_capacity;
    this_->graphics_vendor_name.size = 0;
    this_->graphics_vendor_name.buffer.inline_buffer[0] = 0;

    this_->sound_device_name.capacity = k_msvc_string_inline_capacity;
    this_->sound_device_name.size = 0;
    this_->sound_device_name.buffer.inline_buffer[0] = 0;

    this_->sound_vendor_name.capacity = k_msvc_string_inline_capacity;
    this_->sound_vendor_name.size = 0;
    this_->sound_vendor_name.buffer.inline_buffer[0] = 0;

    head = tree_head_node_allocate();
    this_->property_sets.head = (uint32_t)head;
    head->is_nil = 1;
    head->parent = (uint32_t)head;
    head->left = (uint32_t)head;
    head->right = (uint32_t)head;
    this_->property_sets.size = 0;

    head = tree_head_node_allocate();
    this_->graphic_detail_sets.head = (uint32_t)head;
    head->is_nil = 1;
    head->parent = (uint32_t)head;
    head->left = (uint32_t)head;
    head->right = (uint32_t)head;
    this_->graphic_detail_sets.size = 0;

    this_->flags = 0;
    this_->requirements = 0;

    return this_;
}

#if 0
Original Ghidra decompilation (0x579ef0):

undefined4 * hwreq_parser_construct(undefined4 *param_1)

{
  int iVar1;
  void *local_c;
  undefined1 *puStack_8;
  undefined1 local_4;
  undefined3 uStack_3;

  puStack_8 = &LAB_00639417;
  local_c = ExceptionList;
  ExceptionList = &local_c;
  *param_1 = &PTR_LAB_006721e8;
  param_1[0xf] = 0xf;
  param_1[0xe] = 0;
  *(undefined1 *)(param_1 + 10) = 0;
  param_1[0x16] = 0xf;
  param_1[0x15] = 0;
  *(undefined1 *)(param_1 + 0x11) = 0;
  param_1[0x1d] = 0xf;
  param_1[0x1c] = 0;
  *(undefined1 *)(param_1 + 0x18) = 0;
  param_1[0x24] = 0xf;
  param_1[0x23] = 0;
  *(undefined1 *)(param_1 + 0x1f) = 0;
  param_1[0x2b] = 0xf;
  param_1[0x2a] = 0;
  *(undefined1 *)(param_1 + 0x26) = 0;
  local_4 = 4;
  uStack_3 = 0;
  iVar1 = tree_head_node_allocate();
  param_1[0x1a9] = iVar1;
  *(undefined1 *)(iVar1 + 0x2d) = 1;
  *(undefined4 *)(param_1[0x1a9] + 4) = param_1[0x1a9];
  *(undefined4 *)param_1[0x1a9] = param_1[0x1a9];
  *(undefined4 *)(param_1[0x1a9] + 8) = param_1[0x1a9];
  param_1[0x1aa] = 0;
  _local_4 = CONCAT31(uStack_3,5);
  iVar1 = tree_head_node_allocate();
  param_1[0x1ac] = iVar1;
  *(undefined1 *)(iVar1 + 0x2d) = 1;
  *(undefined4 *)(param_1[0x1ac] + 4) = param_1[0x1ac];
  *(undefined4 *)param_1[0x1ac] = param_1[0x1ac];
  *(undefined4 *)(param_1[0x1ac] + 8) = param_1[0x1ac];
  param_1[0x1ad] = 0;
  param_1[6] = 0;
  param_1[7] = 0;
  ExceptionList = local_c;
  return param_1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
