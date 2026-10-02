// tree_node_allocate  (already named; task-provided)
// address 0x57cc30, size 148 bytes
// name confidence: 0.6 (already carries this name; allocates and fully constructs one
//   hwreq_map_node: left/parent/right links, an empty key string re-assigned from a source
//   pair's key, the value copied from the source pair, and the requested color/is_nil)
// rewrite confidence: 0.5 (allocation and every field write are confirmed against the
//   decompilation; the "source" argument's shape is inferred rather than directly evidenced --
//   see UNSURE)
// evidence: types/shell.h hwreq_map_node (size 0x30: left 0x00, parent 0x04, right 0x08, key
//   0x0c, value 0x28, color 0x2c, is_nil 0x2d). `puVar1[10]` (node+0x28, the value field) is
//   read from `in_ECX+0x1c`; since a bare msvc_std_string is only 0x1c bytes, `in_ECX` cannot
//   be pointing at just the key string -- it must point at a {msvc_std_string key;
//   hwreq_property_set *value;} pair (std::map<string, T*>::value_type), which places the key
//   at ECX+0 (matching the string::assign call, whose "this" is the new node's key -- an
//   unseen register move, same pattern as every other MSVC constructor call in this batch --
//   and whose visible source argument is ECX itself) and the value at ECX+0x1c.
// register convention: stack arguments (Ghidra recognizes all 4): left (param_1), parent
//   (param_2), right (param_3), color (param_4, a byte); ECX = const map value_type *source
//   (key + value to copy into the new node).
// UNSURE: FUN_0057b830 (string::assign) is an opaque lib:crt extern defined elsewhere.

// VERIFIED against disassembly 0x57cc30..0x57ccc4 (2026-09-30): ECX=source, stack left/parent/right/color, ret 0x10; value copied from source+0x1c
#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

typedef struct hwreq_map_value_type {
    msvc_std_string key; // 0x00
    uint32_t value;       // 0x1c hwreq_property_set *
} hwreq_map_value_type; // size 0x20

extern msvc_std_string *string_assign_substr(msvc_std_string *this, const msvc_std_string *right,
    uint32_t pos, uint32_t count); // 0x57b830, module=lib:crt, not this pass

// blam-cc: ECX -> source, stack -> left, parent, right, color
hwreq_map_node *tree_node_allocate(uint32_t left, uint32_t parent, uint32_t right, uint8_t color,
                                    const hwreq_map_value_type *source)
{
    hwreq_map_node *node = (hwreq_map_node *)malloc(sizeof(hwreq_map_node));

    if (node != 0) {
        node->left = left;
        node->parent = parent;
        node->right = right;
        node->key.capacity = 0xf;
        node->key.size = 0;
        node->key.buffer.inline_buffer[0] = 0;
        string_assign_substr(&node->key, &source->key, 0, 0xffffffff);
        node->value = source->value;
        node->color = color;
        node->is_nil = 0;
    }
    return node;
}

#if 0
Original Ghidra decompilation (0x57cc30):

undefined4 *
tree_node_allocate(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined1 param_4)

{
  undefined4 *puVar1;
  int in_ECX;
  void *local_10;
  undefined1 *puStack_c;
  undefined4 local_8;

  local_8 = 0xffffffff;
  puStack_c = &LAB_00639355;
  local_10 = ExceptionList;
  ExceptionList = &local_10;
  puVar1 = operator_new(0x30);
  local_8 = 1;
  if (puVar1 != (undefined4 *)0x0) {
    puVar1[1] = param_2;
    *puVar1 = param_1;
    puVar1[2] = param_3;
    puVar1[9] = 0xf;
    puVar1[8] = 0;
    *(undefined1 *)(puVar1 + 4) = 0;
    FUN_0057b830(in_ECX,0,0xffffffff);
    puVar1[10] = *(undefined4 *)(in_ECX + 0x1c);
    *(undefined1 *)(puVar1 + 0xb) = param_4;
    *(undefined1 *)((int)puVar1 + 0x2d) = 0;
  }
  ExceptionList = local_10;
  return puVar1;
}
#endif
