// tree_splice_insert  (not a Ghidra function; MSVC 7.1 std::_Tree::_Insert for the hwreq std::map<std::string, T*>:
//   links a new node under a known parent and rebalances)
// address 0x57c390, size 412 bytes
// name confidence: 0.7   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x57c390..0x57c52b. Throws length_error("map/set<T> too long") at 0x7fffffe
//   elements; allocates the node (head, parent, head, red) from the value; bumps the size; links it as the root (and
//   both extremes) under the head, else as the parent's left or right child, updating the leftmost / rightmost head
//   links; then the standard red-black insert fix-up with tree_rotate_left / tree_rotate_right; the root turns black.
//   The new node goes to *result_holder, whose address is returned (EAX).
// The earlier guessed prototype (result_holder, insert_as_left, value) was missing the two register arguments: the
//   callers pass the parent in ECX and the tree in EDI (0x57ba5d..0x57bb98, 0x57c283 / 0x57c2da); both callers are
//   updated to this prototype.
// blam-cc: EDI tree, ECX parent, stack -> result_holder, insert_as_left, value (callee pops 0xc)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

typedef struct hwreq_map_value_type {
    msvc_std_string key;
    uint32_t value;
} hwreq_map_value_type; // size 0x20, see tree_node_allocate.c

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

extern hwreq_map_node *tree_node_allocate(uint32_t left, uint32_t parent, uint32_t right, uint8_t color,
    const hwreq_map_value_type *source); // 0x57cc30, blam-cc: ECX source, stack left, parent, right, color
extern void tree_rotate_left(hwreq_map_node *x, msvc_std_map *tree); // 0x57cb10
extern void tree_rotate_right(hwreq_map_node *x, msvc_std_map *tree); // 0x57cb90
extern void *length_error_vtable; // 0x0065508c
extern uint8_t length_error_throw_info[]; // 0x00673524, _ThrowInfo for std::length_error

#define NODE(p) ((hwreq_map_node *)(p))

hwreq_map_node **tree_splice_insert(msvc_std_map *tree, hwreq_map_node *parent, hwreq_map_node **result_holder,
    uint8_t insert_as_left, const hwreq_map_value_type *value)
{
    hwreq_map_node *head = NODE(tree->head);
    hwreq_map_node *node;
    hwreq_map_node *x;

    if (tree->size >= 0x7fffffe) {
        msvc_std_string message;
        hwreq_parse_exception exception;

        message.capacity = 0xf;
        message.size = 0;
        message.buffer.inline_buffer[0] = 0;
        msvc_string_assign_n(&message, "map/set<T> too long", 0x13); // 0x00672244
        hwreq_parse_exception_construct(&exception, &message);
        exception.vtable = (uint32_t)&length_error_vtable;
        _CxxThrowException(&exception, length_error_throw_info);
    }
    node = tree_node_allocate((uint32_t)head, (uint32_t)parent, (uint32_t)head, 0, value);
    tree->size++;
    if (parent == head) {
        head->parent = (uint32_t)node;
        head->left = (uint32_t)node;
        head->right = (uint32_t)node;
    } else if (insert_as_left) {
        parent->left = (uint32_t)node;
        if (parent == NODE(head->left)) {
            head->left = (uint32_t)node;
        }
    } else {
        parent->right = (uint32_t)node;
        if (parent == NODE(head->right)) {
            head->right = (uint32_t)node;
        }
    }
    for (x = node; NODE(x->parent)->color == 0;) {
        hwreq_map_node *p = NODE(x->parent);
        hwreq_map_node *g = NODE(p->parent);

        if (p == NODE(g->left)) {
            hwreq_map_node *uncle = NODE(g->right);

            if (uncle->color == 0) {
                p->color = 1;
                uncle->color = 1;
                NODE(NODE(x->parent)->parent)->color = 0;
                x = NODE(NODE(x->parent)->parent);
            } else {
                if (x == NODE(p->right)) {
                    x = p;
                    tree_rotate_left(x, tree);
                }
                NODE(x->parent)->color = 1;
                NODE(NODE(x->parent)->parent)->color = 0;
                tree_rotate_right(NODE(NODE(x->parent)->parent), tree);
            }
        } else {
            hwreq_map_node *uncle = NODE(g->left);

            if (uncle->color == 0) {
                p->color = 1;
                uncle->color = 1;
                NODE(NODE(x->parent)->parent)->color = 0;
                x = NODE(NODE(x->parent)->parent);
            } else {
                if (x == NODE(p->left)) {
                    x = p;
                    tree_rotate_right(x, tree);
                }
                NODE(x->parent)->color = 1;
                NODE(NODE(x->parent)->parent)->color = 0;
                tree_rotate_left(NODE(NODE(x->parent)->parent), tree);
            }
        }
    }
    NODE(head->parent)->color = 1;
    *result_holder = node;
    return result_holder;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
