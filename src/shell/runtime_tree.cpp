#include "halo/shell/runtime.hpp"

extern "C" {
extern void *length_error_vtable;
extern uint8_t length_error_throw_info[];
extern void *out_of_range_vtable;
extern uint8_t out_of_range_throw_info[];
}

#define NODE(p) ((hwreq_map_node *)(p))

namespace halo::shell {

/**
 * Advances the iterator to the in-order successor; the head sentinel stays where it is.
 *
 * @address 0x57c5e0
 */
void TreeIterator::increment()
{
    hwreq_map_node *node = *self;

    if (node->is_nil != 0) {
        return;
    }

    {
        hwreq_map_node *right = (hwreq_map_node *)node->right;
        if (right->is_nil == 0) {
            hwreq_map_node *cursor = (hwreq_map_node *)right->left;
            while (cursor->is_nil == 0) {
                right = cursor;
                cursor = (hwreq_map_node *)cursor->left;
            }
            *self = right;
            return;
        }
    }

    {
        hwreq_map_node *parent = (hwreq_map_node *)node->parent;
        while (parent->is_nil == 0 && node == (hwreq_map_node *)parent->right) {
            node = parent;
            parent = (hwreq_map_node *)parent->parent;
        }
        *self = parent;
    }
}

/**
 * Moves the iterator to the in-order predecessor; end() steps to the maximum.
 *
 * @address 0x57cd40
 */
void TreeIterator::decrement()
{
    hwreq_map_node *node = *self;

    if (node->is_nil != 0) {
        *self = (hwreq_map_node *)node->right;
        return;
    }

    {
        hwreq_map_node *left = (hwreq_map_node *)node->left;
        if (left->is_nil == 0) {
            hwreq_map_node *prev = left;
            hwreq_map_node *cursor = (hwreq_map_node *)left->right;
            while (cursor->is_nil == 0) {
                prev = cursor;
                cursor = (hwreq_map_node *)cursor->right;
            }
            *self = prev;
            return;
        }
    }

    {
        hwreq_map_node *parent = (hwreq_map_node *)node->parent;
        if (parent->is_nil == 0) {
            while (*self == (hwreq_map_node *)parent->left) {
                *self = parent;
                parent = (hwreq_map_node *)parent->parent;
                if (parent->is_nil != 0) {
                    break;
                }
            }
            if (parent->is_nil == 0) {
                *self = parent;
            }
        }
    }
}

/**
 * Rightmost node of the subtree reached through the right child.
 *
 * @address 0x57cd20
 */
hwreq_map_node *TreeNode::find_max()
{
    hwreq_map_node *cursor = (hwreq_map_node *)self->right;
    while (cursor->is_nil == 0) {
        cursor = (hwreq_map_node *)cursor->right;
    }
    return cursor;
}

/**
 * Leftmost node reached by following left children from the node stored in the given slot.
 *
 * @address 0x57cb70
 */
hwreq_map_node *TreeNode::find_min(hwreq_map_node **subtree_root_left_field)
{
    hwreq_map_node *node = *subtree_root_left_field;
    while (node->is_nil == 0) {
        node = (hwreq_map_node *)node->left;
    }
    return node;
}

/**
 * Frees every node of the subtree, releasing the key strings. Recurses on the right child and loops
 * on the left.
 *
 * @address 0x57cce0
 */
void TreeNode::destroy_subtree()
{
    while (self->is_nil == 0) {
        hwreq_map_node *left = (hwreq_map_node *)self->left;
        TreeNode((hwreq_map_node *)self->right).destroy_subtree();
        StdString(&self->key).destroy();
        free(self);
        self = left;
    }
}

/**
 * Allocates the zeroed head sentinel node (black, not yet marked nil). Returns null if the
 * allocation fails.
 *
 * @address 0x57cbf0
 */
hwreq_map_node *TreeNode::allocate_head()
{
    hwreq_map_node *node = (hwreq_map_node *)malloc(sizeof(hwreq_map_node));

    if (node != 0) {
        node->left = 0;
        node->parent = 0;
        node->right = 0;
        node->color = 1;
        node->is_nil = 0;
    }
    return node;
}

/**
 * Allocates a node linked to the given neighbours, with a copy of the value's key and mapped
 * pointer.
 *
 * @address 0x57cc30
 */
hwreq_map_node *TreeNode::allocate(uint32_t left, uint32_t parent, uint32_t right, uint8_t color, const hwreq_map_value_type *source)
{
    hwreq_map_node *node = (hwreq_map_node *)malloc(sizeof(hwreq_map_node));

    if (node != 0) {
        node->left = left;
        node->parent = parent;
        node->right = right;
        node->key.capacity = 0xf;
        node->key.size = 0;
        node->key.buffer.inline_buffer[0] = 0;
        StdString(&node->key).assign_substr(&source->key, 0, k_datum_index_none);
        node->value = source->value;
        node->color = color;
        node->is_nil = 0;
    }
    return node;
}

/**
 * Compares the search key with the key stored in the node.
 */
int32_t TreeNode::compare_key(const msvc_std_string *search_key)
{
    const char *node_data = (self->key.capacity < 0x10) ? self->key.buffer.inline_buffer : (const char *)self->key.buffer.heap_buffer;
    return StdString(search_key).compare(search_key->size, 0, node_data, self->key.size);
}

/**
 * First node whose key is not less than search_key, or the head sentinel.
 *
 * @address 0x57c530
 */
hwreq_map_node *StdMap::lower_bound(const msvc_std_string *search_key)
{
    hwreq_map_node *head = (hwreq_map_node *)self->head;
    hwreq_map_node *best = head;
    hwreq_map_node *candidate = (hwreq_map_node *)head->parent;

    const char *search_data = (search_key->capacity < 0x10) ? search_key->buffer.inline_buffer : (const char *)search_key->buffer.heap_buffer;

    if (candidate->is_nil == 0) {
        do {
            if (StdString(&candidate->key).compare(candidate->key.size, 0, search_data, search_key->size) < 0) {
                candidate = (hwreq_map_node *)candidate->right;
            } else {
                best = candidate;
                candidate = (hwreq_map_node *)candidate->left;
            }
        } while (candidate->is_nil == 0);
    }

    return best;
}

/**
 * std::map::find: the node whose key equals key, or the head sentinel when absent.
 *
 * @address 0x57b7a0
 */
hwreq_map_node *StdMap::find(msvc_std_string *key)
{
    hwreq_map_node *node = lower_bound(key);

    if (node != (hwreq_map_node *)self->head) {
        const char *node_key = node->key.capacity >= 0x10 ? (const char *)node->key.buffer.heap_buffer : node->key.buffer.inline_buffer;

        if (StdString(key).compare(key->size, 0, node_key, node->key.size) >= 0) {
            return node;
        }
    }
    return (hwreq_map_node *)self->head;
}

/**
 * Red-black tree left rotation around x.
 *
 * @address 0x57cb10
 */
void StdMap::rotate_left(hwreq_map_node *x)
{
    hwreq_map_node *y = (hwreq_map_node *)x->right;
    hwreq_map_node *head = (hwreq_map_node *)self->head;

    x->right = y->left;
    if (((hwreq_map_node *)y->left)->is_nil == 0) {
        ((hwreq_map_node *)y->left)->parent = (uint32_t)x;
    }
    y->parent = x->parent;

    if (x == (hwreq_map_node *)head->parent) {
        head->parent = (uint32_t)y;
    } else if (x == (hwreq_map_node *)((hwreq_map_node *)x->parent)->left) {
        ((hwreq_map_node *)x->parent)->left = (uint32_t)y;
    } else {
        ((hwreq_map_node *)x->parent)->right = (uint32_t)y;
    }

    y->left = (uint32_t)x;
    x->parent = (uint32_t)y;
}

/**
 * Red-black tree right rotation around x.
 *
 * @address 0x57cb90
 */
void StdMap::rotate_right(hwreq_map_node *x)
{
    hwreq_map_node *y = (hwreq_map_node *)x->left;
    hwreq_map_node *head = (hwreq_map_node *)self->head;

    x->left = y->right;
    if (((hwreq_map_node *)y->right)->is_nil == 0) {
        ((hwreq_map_node *)y->right)->parent = (uint32_t)x;
    }
    y->parent = x->parent;

    if (x == (hwreq_map_node *)head->parent) {
        head->parent = (uint32_t)y;
    } else if (x == (hwreq_map_node *)((hwreq_map_node *)x->parent)->right) {
        ((hwreq_map_node *)x->parent)->right = (uint32_t)y;
    } else {
        ((hwreq_map_node *)x->parent)->left = (uint32_t)y;
    }

    y->right = (uint32_t)x;
    x->parent = (uint32_t)y;
}

/**
 * Allocates a node for value, links it under parent and rebalances. Raises the map too long error
 * past the maximum size.
 *
 * @address 0x57c390
 */
hwreq_map_node **StdMap::splice_insert(hwreq_map_node *parent, hwreq_map_node **result_holder, uint8_t insert_as_left, const hwreq_map_value_type *value)
{
    hwreq_map_node *head = NODE(self->head);
    hwreq_map_node *node;
    hwreq_map_node *x;

    if (self->size >= 0x7fffffe) {
        StdThrow::map_too_long();
    }
    node = TreeNode::allocate((uint32_t)head, (uint32_t)parent, (uint32_t)head, 0, value);
    self->size++;
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
                    rotate_left(x);
                }
                NODE(x->parent)->color = 1;
                NODE(NODE(x->parent)->parent)->color = 0;
                rotate_right(NODE(NODE(x->parent)->parent));
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
                    rotate_right(x);
                }
                NODE(x->parent)->color = 1;
                NODE(NODE(x->parent)->parent)->color = 0;
                rotate_left(NODE(NODE(x->parent)->parent));
            }
        }
    }
    NODE(head->parent)->color = 1;
    *result_holder = node;
    return result_holder;
}

/**
 * std::map::insert: inserts value unless an equal key exists and reports the node and whether it
 * was inserted.
 *
 * @address 0x57c1a0
 */
void StdMap::insert_unique(hwreq_tree_insert_result *result, const hwreq_map_value_type *value)
{
    hwreq_map_node *head = (hwreq_map_node *)self->head;
    hwreq_map_node *parent = head;
    hwreq_map_node *where;
    uint8_t went_left = 1;

    {
        hwreq_map_node *candidate = (hwreq_map_node *)head->parent;
        if (candidate->is_nil == 0) {
            do {
                parent = candidate;
                went_left = TreeNode(candidate).compare_key(&value->key) < 0;
                candidate = went_left ? (hwreq_map_node *)candidate->left : (hwreq_map_node *)candidate->right;
            } while (candidate->is_nil == 0);
        }
    }

    where = parent;

    if (went_left) {
        if (parent == (hwreq_map_node *)head->left) {
            hwreq_map_node *holder;
            hwreq_map_node *inserted = *splice_insert(where, &holder, 1, value);
            result->node = inserted;
            result->inserted = 1;
            return;
        }
        TreeIterator(&parent).decrement();
    }

    if (TreeNode(parent).compare_key(&value->key) > 0) {
        hwreq_map_node *holder;
        hwreq_map_node *inserted = *splice_insert(where, &holder, went_left, value);
        result->node = inserted;
        result->inserted = 1;
        return;
    }

    result->node = parent;
    result->inserted = 0;
}

/**
 * std::map::insert(hint, value): tries the position next to the hint before falling back to a full
 * unique insert.
 *
 * @address 0x57ba50
 */
hwreq_map_node *StdMap::hint_insert_unique(hwreq_map_node **result_holder, hwreq_map_node *hint, const hwreq_map_value_type *value)
{
    hwreq_map_node *head = (hwreq_map_node *)self->head;
    const msvc_std_string *value_key = &value->key;

    if (self->size == 0) {
        return *splice_insert(head, result_holder, 1, value);
    }

    if (hint == (hwreq_map_node *)head->left) {
        const char *hint_data = (hint->key.capacity < 0x10) ? hint->key.buffer.inline_buffer : (const char *)hint->key.buffer.heap_buffer;
        if (StdString(value_key).compare(value_key->size, 0, hint_data, hint->key.size) < 0) {
            return *splice_insert(hint, result_holder, 1, value);
        }
        goto full_search;
    }

    if (hint == head) {
        if (StdString(&((hwreq_map_node *)head->right)->key).less_than(value_key)) {
            return *splice_insert((hwreq_map_node *)head->right, result_holder, 0, value);
        }
        goto full_search;
    }

    if (StdString(value_key).less_than(&hint->key)) {
        hwreq_map_node *predecessor = hint;
        TreeIterator(&predecessor).decrement();
        if (StdString(&predecessor->key).less_than(value_key)) {
            if (((hwreq_map_node *)predecessor->right)->is_nil != 0) {
                return *splice_insert(predecessor, result_holder, 0, value);
            }
            return *splice_insert(hint, result_holder, 1, value);
        }
    } else if (StdString(&hint->key).less_than(value_key)) {
        hwreq_map_node *successor = hint;
        TreeIterator(&successor).increment();
        if (successor == head || StdString(value_key).less_than(&successor->key)) {
            if (((hwreq_map_node *)hint->right)->is_nil != 0) {
                return *splice_insert(hint, result_holder, 0, value);
            }
            return *splice_insert(successor, result_holder, 1, value);
        }
    }

full_search:
    {
        hwreq_tree_insert_result local_result;
        insert_unique(&local_result, value);
        *result_holder = local_result.node;
        return *result_holder;
    }
}

/**
 * Unlinks and frees one node, rebalancing the tree, and reports the following node. Raises the
 * invalid iterator error when asked to erase the head sentinel.
 *
 * @address 0x57c820
 */
hwreq_map_node **StdMap::erase_one(hwreq_map_node **result_holder, hwreq_map_node *erased)
{
    hwreq_map_node *head;
    hwreq_map_node *successor = erased;
    hwreq_map_node *fix;
    hwreq_map_node *fix_parent;

    if (erased->is_nil) {
        StdThrow::invalid_map_iterator();
    }
    TreeIterator(&successor).increment();
    head = NODE(self->head);
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
            head->left = (uint32_t)(fix->is_nil ? fix_parent : TreeNode::find_min((hwreq_map_node **)fix));
        }
        if (NODE(head->right) == erased) {
            head->right = (uint32_t)(fix->is_nil ? fix_parent : TreeNode(fix).find_max());
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
                    rotate_left(fix_parent);
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
                    rotate_right(sibling);
                    sibling = NODE(fix_parent->right);
                }
                sibling->color = fix_parent->color;
                fix_parent->color = 1;
                NODE(sibling->right)->color = 1;
                rotate_left(fix_parent);
                break;
            } else {
                hwreq_map_node *sibling = NODE(fix_parent->left);

                if (sibling->color == 0) {
                    sibling->color = 1;
                    fix_parent->color = 0;
                    rotate_right(fix_parent);
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
                    rotate_left(sibling);
                    sibling = NODE(fix_parent->left);
                }
                sibling->color = fix_parent->color;
                fix_parent->color = 1;
                NODE(sibling->left)->color = 1;
                rotate_right(fix_parent);
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
    if (self->size > 0) {
        self->size--;
    }
    *result_holder = successor;
    return result_holder;
}

/**
 * Erases [first, last); erasing the whole map frees every node at once.
 *
 * @address 0x57c310
 */
hwreq_map_node **StdMap::erase_range(hwreq_map_node **out, hwreq_map_node *first, hwreq_map_node *last)
{
    hwreq_map_node *head = (hwreq_map_node *)self->head;

    if (first == (hwreq_map_node *)head->left && last == head) {
        TreeNode((hwreq_map_node *)head->parent).destroy_subtree();
        head->parent = (uint32_t)head;
        self->size = 0;
        head->left = (uint32_t)head;
        head->right = (uint32_t)head;
        *out = (hwreq_map_node *)head->left;
        return out;
    }

    while (first != last) {
        hwreq_map_node *current = first;
        TreeIterator(&first).increment();
        erase_one(out, current);
    }
    *out = first;
    return out;
}

/**
 * Erases every node, frees the head sentinel and empties the map.
 *
 * @address 0x579fe0
 */
void StdMap::destruct()
{
    hwreq_map_node *dummy_out;
    hwreq_map_node *head = (hwreq_map_node *)self->head;

    erase_range(&dummy_out, (hwreq_map_node *)head->left, head);

    free((void *)self->head);
    self->head = 0;
    self->size = 0;
}

/**
 * std::map::operator[]: returns the slot for key, inserting a null entry when it is missing.
 *
 * @address 0x57b6e0
 */
hwreq_property_set **StdMap::index_property_set(msvc_std_string *key)
{
    hwreq_map_node *node = lower_bound(key);

    if (node == (hwreq_map_node *)self->head ||
        StdString(key).compare(key->size, 0, node->key.capacity >= 0x10 ? (const char *)node->key.buffer.heap_buffer :
            node->key.buffer.inline_buffer, node->key.size) < 0) {
        hwreq_map_value_type pair;
        hwreq_map_node *inserted;

        pair.key.capacity = 0xf;
        pair.key.size = 0;
        pair.key.buffer.inline_buffer[0] = 0;
        StdString(&pair.key).assign_substr(key, 0, k_datum_index_none);
        pair.value = 0;
        hint_insert_unique(&inserted, node, &pair);
        node = inserted;
        if (pair.key.capacity >= 0x10) {
            free((void *)pair.key.buffer.heap_buffer);
        }
    }
    return (hwreq_property_set **)&node->value;
}

}

#undef NODE
