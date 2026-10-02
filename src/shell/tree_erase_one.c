// tree_erase_one  (not a Ghidra function; MSVC 7.1 std::_Tree::erase(iterator) for the hwreq std::map)
// address 0x57c820, size 744 bytes
// name confidence: 0.7   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x57c820..0x57cb07. Erasing the head throws out_of_range("invalid map/set<T>
//   iterator"). The successor of the node becomes the result. A node with at most one child is replaced by it (the
//   leftmost / rightmost head links move to the parent or the child's minimum / maximum); a node with two children is
//   replaced by its successor, which takes over its links and colour. Erasing a black node runs the standard red-black
//   erase fix-up (tree_rotate_left / tree_rotate_right). The node's key string is released and the node freed, the
//   size drops (never below 0) and the successor goes to *result_holder, whose address is returned.
// blam-cc: stack -> tree, result_holder, node (callee pops 0xc)

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

typedef struct hwreq_parse_exception {
    uint32_t vtable;         // 0x00
    uint32_t dofree;         // 0x04
    uint32_t legacy_what;    // 0x08
    msvc_std_string message; // 0x0c
} hwreq_parse_exception; // size 0x28

extern hwreq_parse_exception *hwreq_parse_exception_construct(hwreq_parse_exception *self,
    const msvc_std_string *message); // 0x5782b0, blam-cc: ECX -> this, stack -> message
extern __declspec(noreturn) void __stdcall _CxxThrowException(void *object, void *throw_info); // CRT: 0x639177
extern msvc_std_string *msvc_string_assign_n(msvc_std_string *self, const char *source, uint32_t count); // 0x57bc90

extern void tree_iterator_increment(hwreq_map_node **iterator); // 0x57c5e0, blam-cc: EDX
extern hwreq_map_node *tree_find_min(hwreq_map_node **subtree_root_left_field); // 0x57cb70, blam-cc: EAX
extern hwreq_map_node *tree_find_max(hwreq_map_node *node); // 0x57cd20, blam-cc: EAX
extern void tree_rotate_left(hwreq_map_node *x, msvc_std_map *tree); // 0x57cb10
extern void tree_rotate_right(hwreq_map_node *x, msvc_std_map *tree); // 0x57cb90
extern void *out_of_range_vtable; // 0x00655098
extern uint8_t out_of_range_throw_info[]; // 0x00673560, _ThrowInfo for std::out_of_range

#define NODE(p) ((hwreq_map_node *)(p))

hwreq_map_node **tree_erase_one(msvc_std_map *tree, hwreq_map_node **result_holder, hwreq_map_node *erased)
{
    hwreq_map_node *head;
    hwreq_map_node *successor = erased;
    hwreq_map_node *fix;
    hwreq_map_node *fix_parent;

    if (erased->is_nil) {
        msvc_std_string message;
        hwreq_parse_exception exception;

        message.capacity = 0xf;
        message.size = 0;
        message.buffer.inline_buffer[0] = 0;
        msvc_string_assign_n(&message, "invalid map/set<T> iterator", 0x1b); // 0x00672228
        hwreq_parse_exception_construct(&exception, &message);
        exception.vtable = (uint32_t)&out_of_range_vtable;
        _CxxThrowException(&exception, out_of_range_throw_info);
    }
    tree_iterator_increment(&successor);
    head = NODE(tree->head);
    if (NODE(erased->left)->is_nil || NODE(erased->right)->is_nil) {
        fix = NODE(erased->left)->is_nil ? NODE(erased->right) : NODE(erased->left);
        fix_parent = NODE(erased->parent);
        if (!fix->is_nil) {
            fix->parent = (uint32_t)fix_parent;
        }
        if (NODE(head->parent) == erased) {
            head->parent = (uint32_t)fix;
        } else if (NODE(fix_parent->left) == erased) {
            fix_parent->left = (uint32_t)fix;
        } else {
            fix_parent->right = (uint32_t)fix;
        }
        if (NODE(head->left) == erased) {
            head->left = (uint32_t)(fix->is_nil ? fix_parent : tree_find_min((hwreq_map_node **)fix));
        }
        if (NODE(head->right) == erased) {
            head->right = (uint32_t)(fix->is_nil ? fix_parent : tree_find_max(fix));
        }
    } else {
        hwreq_map_node *replacement = successor;
        uint8_t color;

        fix = NODE(replacement->right);
        NODE(erased->left)->parent = (uint32_t)replacement;
        replacement->left = erased->left;
        if (replacement == NODE(erased->right)) {
            fix_parent = replacement;
        } else {
            fix_parent = NODE(replacement->parent);
            if (!fix->is_nil) {
                fix->parent = (uint32_t)fix_parent;
            }
            fix_parent->left = (uint32_t)fix;
            replacement->right = erased->right;
            NODE(erased->right)->parent = (uint32_t)replacement;
        }
        if (NODE(head->parent) == erased) {
            head->parent = (uint32_t)replacement;
        } else if (NODE(NODE(erased->parent)->left) == erased) {
            NODE(erased->parent)->left = (uint32_t)replacement;
        } else {
            NODE(erased->parent)->right = (uint32_t)replacement;
        }
        replacement->parent = erased->parent;
        color = replacement->color;
        replacement->color = erased->color;
        erased->color = color;
    }
    if (erased->color == 1) {
        for (; fix != NODE(head->parent) && fix->color == 1; fix = fix_parent, fix_parent = NODE(fix->parent)) {
            if (fix == NODE(fix_parent->left)) {
                hwreq_map_node *sibling = NODE(fix_parent->right);

                if (sibling->color == 0) {
                    sibling->color = 1;
                    fix_parent->color = 0;
                    tree_rotate_left(fix_parent, tree);
                    sibling = NODE(fix_parent->right);
                }
                if (sibling->is_nil) {
                    continue;
                }
                if (NODE(sibling->left)->color == 1 && NODE(sibling->right)->color == 1) {
                    sibling->color = 0;
                    continue;
                }
                if (NODE(sibling->right)->color == 1) {
                    NODE(sibling->left)->color = 1;
                    sibling->color = 0;
                    tree_rotate_right(sibling, tree);
                    sibling = NODE(fix_parent->right);
                }
                sibling->color = fix_parent->color;
                fix_parent->color = 1;
                NODE(sibling->right)->color = 1;
                tree_rotate_left(fix_parent, tree);
                break;
            } else {
                hwreq_map_node *sibling = NODE(fix_parent->left);

                if (sibling->color == 0) {
                    sibling->color = 1;
                    fix_parent->color = 0;
                    tree_rotate_right(fix_parent, tree);
                    sibling = NODE(fix_parent->left);
                }
                if (sibling->is_nil) {
                    continue;
                }
                if (NODE(sibling->right)->color == 1 && NODE(sibling->left)->color == 1) {
                    sibling->color = 0;
                    continue;
                }
                if (NODE(sibling->left)->color == 1) {
                    NODE(sibling->right)->color = 1;
                    sibling->color = 0;
                    tree_rotate_left(sibling, tree);
                    sibling = NODE(fix_parent->left);
                }
                sibling->color = fix_parent->color;
                fix_parent->color = 1;
                NODE(sibling->left)->color = 1;
                tree_rotate_right(fix_parent, tree);
                break;
            }
        }
        fix->color = 1;
    }
    if (erased->key.capacity >= 0x10) {
        free((void *)erased->key.buffer.heap_buffer);
    }
    erased->key.capacity = 0xf;
    erased->key.size = 0;
    erased->key.buffer.inline_buffer[0] = 0;
    free(erased);
    if (tree->size > 0) {
        tree->size--;
    }
    *result_holder = successor;
    return result_holder;
}
