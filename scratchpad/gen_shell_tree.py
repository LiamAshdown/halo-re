import os
os.chdir(r'C:\Users\Liam-\halo-re')

HEAD = '''#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"
'''
VALUE_TYPE = '''
typedef struct hwreq_map_value_type {
    msvc_std_string key;
    uint32_t value;
} hwreq_map_value_type; // size 0x20, see tree_node_allocate.c
'''
EXC = '''
typedef struct hwreq_parse_exception {
    uint32_t vtable;         // 0x00
    uint32_t dofree;         // 0x04
    uint32_t legacy_what;    // 0x08
    msvc_std_string message; // 0x0c
} hwreq_parse_exception; // size 0x28

extern hwreq_parse_exception *hwreq_parse_exception_construct(hwreq_parse_exception *this,
    const msvc_std_string *message); // 0x5782b0, blam-cc: ECX -> this, stack -> message
extern __declspec(noreturn) void __stdcall _CxxThrowException(void *object, void *throw_info); // CRT: 0x639177
extern msvc_std_string *msvc_string_assign_n(msvc_std_string *this, const char *source, uint32_t count); // 0x57bc90
'''

FILES = {}

FILES['tree_splice_insert'] = '''// tree_splice_insert  (not a Ghidra function; MSVC 7.1 std::_Tree::_Insert for the hwreq std::map<std::string, T*>:
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

''' + HEAD + VALUE_TYPE + EXC + '''
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
'''

FILES['tree_erase_one'] = '''// tree_erase_one  (not a Ghidra function; MSVC 7.1 std::_Tree::erase(iterator) for the hwreq std::map)
// address 0x57c820, size 744 bytes
// name confidence: 0.7   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x57c820..0x57cb07. Erasing the head throws out_of_range("invalid map/set<T>
//   iterator"). The successor of the node becomes the result. A node with at most one child is replaced by it (the
//   leftmost / rightmost head links move to the parent or the child's minimum / maximum); a node with two children is
//   replaced by its successor, which takes over its links and colour. Erasing a black node runs the standard red-black
//   erase fix-up (tree_rotate_left / tree_rotate_right). The node's key string is released and the node freed, the
//   size drops (never below 0) and the successor goes to *result_holder, whose address is returned.
// blam-cc: stack -> tree, result_holder, node (callee pops 0xc)

''' + HEAD + EXC + '''
extern void tree_iterator_increment(hwreq_map_node **iterator); // 0x57c5e0, blam-cc: EDX
extern hwreq_map_node *tree_find_min(hwreq_map_node **subtree_root_left_field); // 0x57cb70, blam-cc: EAX
extern hwreq_map_node *tree_find_max(hwreq_map_node *node); // 0x57cd20, blam-cc: EAX
extern void tree_rotate_left(hwreq_map_node *x, msvc_std_map *tree); // 0x57cb10
extern void tree_rotate_right(hwreq_map_node *x, msvc_std_map *tree); // 0x57cb90
extern void _free(void *ptr); // 0x6277e8
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
        _free((void *)erased->key.buffer.heap_buffer);
    }
    erased->key.capacity = 0xf;
    erased->key.size = 0;
    erased->key.buffer.inline_buffer[0] = 0;
    _free(erased);
    if (tree->size > 0) {
        tree->size--;
    }
    *result_holder = successor;
    return result_holder;
}
'''

FILES['hwreq_map_find'] = '''// hwreq_map_find  (not a Ghidra function; std::map<std::string, T*>::find for the hwreq parser's maps)
// address 0x57b7a0, size 89 bytes
// name confidence: 0.8   rewrite confidence: 0.9
// WRITTEN 2026-09-28 from objdump 0x57b7a0..0x57b7f8: lower_bound of the key (tree_lower_bound 0x57c530); unless that
//   is the head or the key compares below the node's key (string_compare 0x57ce10 < 0), the node, else the head. The
//   binary writes the iterator through EBX and returns that address; the callers only read the node, so it is
//   returned directly (as their prototypes already expect).
// blam-cc: EDI map, ESI key, EBX iterator slot

''' + HEAD + '''
extern hwreq_map_node *tree_lower_bound(msvc_std_map *tree, const msvc_std_string *search_key); // 0x57c530, blam-cc: EAX tree, ECX key
extern int32_t string_compare(const msvc_std_string *this, uint32_t n1, uint32_t pos, const char *s, uint32_t n2); // 0x57ce10

hwreq_map_node *hwreq_map_find(msvc_std_map *map, msvc_std_string *key)
{
    hwreq_map_node *node = tree_lower_bound(map, key);

    if (node != (hwreq_map_node *)map->head) {
        const char *node_key = node->key.capacity >= 0x10 ? (const char *)node->key.buffer.heap_buffer : node->key.buffer.inline_buffer;

        if (string_compare(key, key->size, 0, node_key, node->key.size) >= 0) {
            return node;
        }
    }
    return (hwreq_map_node *)map->head;
}
'''

FILES['hwreq_property_set_map_index'] = '''// hwreq_property_set_map_index  (not a Ghidra function; std::map<std::string, hwreq_property_set *>::operator[])
// address 0x57b6e0, size 189 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x57b6e0..0x57b79c: lower_bound of the key; when that is the head or the key
//   compares below it, a (copy of the key, NULL) pair is inserted with that node as the hint
//   (tree_hint_insert_unique 0x57ba50) and the local key copy released. Returns the address of the node's value.
// blam-cc: EDI key, stack -> map (callee pops 4)

''' + HEAD + VALUE_TYPE + '''
extern hwreq_map_node *tree_lower_bound(msvc_std_map *tree, const msvc_std_string *search_key); // 0x57c530, blam-cc: EAX tree, ECX key
extern int32_t string_compare(const msvc_std_string *this, uint32_t n1, uint32_t pos, const char *s, uint32_t n2); // 0x57ce10
extern msvc_std_string *string_assign_substr(msvc_std_string *this, const msvc_std_string *right, uint32_t pos,
    uint32_t count); // 0x57b830, blam-cc: ECX this, stack right, pos, count
extern hwreq_map_node *tree_hint_insert_unique(msvc_std_map *tree, hwreq_map_node **result_holder,
    hwreq_map_node *hint, const hwreq_map_value_type *value); // 0x57ba50
extern void _free(void *ptr); // 0x6277e8

hwreq_property_set **hwreq_property_set_map_index(msvc_std_string *key, msvc_std_map *map)
{
    hwreq_map_node *node = tree_lower_bound(map, key);

    if (node == (hwreq_map_node *)map->head ||
        string_compare(key, key->size, 0, node->key.capacity >= 0x10 ? (const char *)node->key.buffer.heap_buffer :
            node->key.buffer.inline_buffer, node->key.size) < 0) {
        hwreq_map_value_type pair;
        hwreq_map_node *inserted;

        pair.key.capacity = 0xf;
        pair.key.size = 0;
        pair.key.buffer.inline_buffer[0] = 0;
        string_assign_substr(&pair.key, key, 0, 0xffffffff);
        pair.value = 0;
        tree_hint_insert_unique(map, &inserted, node, &pair);
        node = inserted;
        if (pair.key.capacity >= 0x10) {
            _free((void *)pair.key.buffer.heap_buffer);
        }
    }
    return (hwreq_property_set **)&node->value;
}
'''

FILES['hwreq_property_set_apply'] = '''// hwreq_property_set_apply  (not a Ghidra function; merges one property set into another)
// address 0x57b470, size 77 bytes
// name confidence: 0.7   rewrite confidence: 0.9
// WRITTEN 2026-09-28 from objdump 0x57b470..0x57b4ba: for every (name, value) pair of the source's flag vector,
//   hwreq_property_set_upsert (0x578410) into the target with the two strings' c_str().
// blam-cc: EAX source, stack -> target (callee pops 4)

''' + HEAD + '''
extern void hwreq_property_set_upsert(hwreq_property_set *property_set, char *key, char *value); // 0x578410

static char *c_str(msvc_std_string *s)
{
    return s->capacity >= 0x10 ? (char *)s->buffer.heap_buffer : s->buffer.inline_buffer;
}

void hwreq_property_set_apply(hwreq_property_set *source, hwreq_property_set *target)
{
    hwreq_string_pair *pair = (hwreq_string_pair *)source->flags.first;
    hwreq_string_pair *end = (hwreq_string_pair *)source->flags.last;

    for (; pair != end; pair++) {
        hwreq_property_set_upsert(target, c_str(&pair->first), c_str(&pair->second));
    }
}
'''

THIS_NOTE = ('// The C++ runtime calls it through the %s with __thiscall (this in ECX, arguments on the stack, callee\n'
             '//   pops); __fastcall has exactly that shape for a first pointer argument (the EDX slot is unused).\n')

FILES['std_exception_what'] = '''// std_exception_what  (not a Ghidra function; the what() slot of the std::logic_error / length_error / out_of_range
//   vtables 0x00655084, 0x00655090, 0x0065509c; no C existed, so those stored pointers trapped)
// address 0x578380, size 14 bytes
// name confidence: 0.8   rewrite confidence: 0.95
// WRITTEN 2026-09-28 from objdump 0x578380..0x57838d: returns the message string's c_str() -- the heap pointer at
//   +0x10 when its capacity (+0x24) is 0x10 or more, else the inline buffer at +0x10.
''' + THIS_NOTE % 'vtable' + '''// blam-cc: ECX this

''' + HEAD + '''
const char *__fastcall std_exception_what(uint8_t *this, void *unused_edx)
{
    (void)unused_edx;
    return *(uint32_t *)(this + 0x24) >= 0x10 ? *(const char **)(this + 0x10) : (const char *)(this + 0x10);
}
'''

for name, addr, vtable, vaddr, slot, kind in (('std_out_of_range_copy_construct', 0x57bc00, 'out_of_range_vtable', 0x655098, 0x6735d0, 'out_of_range'),
                                              ('std_length_error_copy_construct', 0x57c6b0, 'length_error_vtable', 0x65508c, 0x67355c, 'length_error')):
    FILES[name] = ('''// %(name)s  (not a Ghidra function; the copy constructor in the std::%(kind)s catchable type at 0x%(slot08)s;
//   no C existed, so the stored pointer trapped)
// address 0x%(addr)x, size 25 bytes
// name confidence: 0.8   rewrite confidence: 0.9
// WRITTEN 2026-09-28 from objdump 0x%(addr)x..0x%(end)x: copy constructs the logic_error part
//   (hwreq_parse_exception_copy_construct 0x57bc20), then stores the std::%(kind)s vtable (0x%(vaddr)08x); returns this.
''' % dict(name=name, kind=kind, slot08='%08x' % slot, addr=addr, end=addr + 0x16, vaddr=vaddr)) + THIS_NOTE % 'catchable type' + \
        '// blam-cc: ECX this, stack -> other (callee pops 4)\n\n' + HEAD + EXC.split('extern __declspec')[0] + '''
extern void hwreq_parse_exception_copy_construct(hwreq_parse_exception *this, const hwreq_parse_exception *other); // 0x57bc20
extern void *%(vtable)s; // 0x%(vaddr)08x

hwreq_parse_exception *__fastcall %(name)s(hwreq_parse_exception *this, void *unused_edx,
    const hwreq_parse_exception *other)
{
    (void)unused_edx;
    hwreq_parse_exception_copy_construct(this, other);
    this->vtable = (uint32_t)&%(vtable)s;
    return this;
}
''' % dict(name=name, vtable=vtable, vaddr=vaddr)

for name, src in FILES.items():
    p = 'src/shell/%s.c' % name
    assert not os.path.exists(p), p
    open(p, 'w', encoding='utf-8').write(src)
    print('wrote', p)
