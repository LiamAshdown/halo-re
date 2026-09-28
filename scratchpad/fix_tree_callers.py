import os, re
os.chdir(r'C:\Users\Liam-\halo-re')

NEW_EXTERN = ('extern hwreq_map_node **tree_splice_insert(msvc_std_map *tree, hwreq_map_node *parent, hwreq_map_node **result_holder,\n'
              '    uint8_t insert_as_left, const hwreq_map_value_type *value); // 0x57c390, blam-cc: EDI tree, ECX parent, stack rest')

# ---- tree_hint_insert_unique.c
p = 'src/shell/tree_hint_insert_unique.c'
s = open(p, encoding='utf-8').read()
old_ext = re.search(r'extern hwreq_map_node \*tree_splice_insert\(hwreq_map_node \*\*result_holder, uint8_t insert_as_left,\n    const hwreq_map_value_type \*value\); // 0x57c390, UNSURE: signature guessed, not this pass', s).group(0)
s = s.replace(old_ext, NEW_EXTERN)
start = s.index('// blam-cc: EAX -> tree, ESI -> result_holder, EBX -> value, stack -> hint\nhwreq_map_node *tree_hint_insert_unique(')
end = s.index('\n}\n', start) + 3
s = s[:start] + '''// blam-cc: EAX -> tree, ESI -> result_holder, EBX -> value, stack -> hint
// FIXED 2026-09-28 (retail-independence loop): tree_splice_insert also takes the tree (EDI) and the parent (ECX); per
//   objdump 0x57ba50..0x57bbbe the parent is the head for an empty tree, the hint before begin(), the rightmost node
//   after the end, and in the neighbour cases the predecessor (when ITS right child is nil, as a right child) or the
//   hint (left), resp. the hint (when its right child is nil, as a right child) or the successor (left). The old
//   predecessor case tested hint->left instead of predecessor->right.
hwreq_map_node *tree_hint_insert_unique(msvc_std_map *tree, hwreq_map_node **result_holder,
                                         hwreq_map_node *hint, const hwreq_map_value_type *value)
{
    hwreq_map_node *head = (hwreq_map_node *)tree->head;
    const msvc_std_string *value_key = &value->key;

    if (tree->size == 0) {
        return *tree_splice_insert(tree, head, result_holder, 1, value);
    }

    if (hint == (hwreq_map_node *)head->left) { // hint == begin(): try inserting before the first element
        const char *hint_data = (hint->key.capacity < 0x10) ? hint->key.buffer.inline_buffer : (const char *)hint->key.buffer.heap_buffer;
        if (string_compare(value_key, value_key->size, 0, hint_data, hint->key.size) < 0) {
            return *tree_splice_insert(tree, hint, result_holder, 1, value);
        }
        goto full_search;
    }

    if (hint == head) { // hint == end(): try inserting after the last element
        if (hwreq_map_key_less_than(&((hwreq_map_node *)head->right)->key, value_key)) {
            return *tree_splice_insert(tree, (hwreq_map_node *)head->right, result_holder, 0, value);
        }
        goto full_search;
    }

    if (hwreq_map_key_less_than(value_key, &hint->key)) {
        // value < *hint: try just after the predecessor of hint
        hwreq_map_node *predecessor = hint;
        tree_iterator_decrement(&predecessor);
        if (hwreq_map_key_less_than(&predecessor->key, value_key)) {
            if (((hwreq_map_node *)predecessor->right)->is_nil != 0) {
                return *tree_splice_insert(tree, predecessor, result_holder, 0, value);
            }
            return *tree_splice_insert(tree, hint, result_holder, 1, value);
        }
    } else if (hwreq_map_key_less_than(&hint->key, value_key)) {
        // *hint < value: try just before the successor of hint
        hwreq_map_node *successor = hint;
        tree_iterator_increment(&successor);
        if (successor == head || hwreq_map_key_less_than(value_key, &successor->key)) {
            if (((hwreq_map_node *)hint->right)->is_nil != 0) {
                return *tree_splice_insert(tree, hint, result_holder, 0, value);
            }
            return *tree_splice_insert(tree, successor, result_holder, 1, value);
        }
    }

full_search:
    {
        uint8_t local_result[8];
        tree_insert_unique(tree, local_result, value);
        *result_holder = *(hwreq_map_node **)local_result;
        return *result_holder;
    }
}
''' + s[end:]
open(p, 'w', encoding='utf-8').write(s)
print('patched', p)

# ---- tree_insert_unique.c
p = 'src/shell/tree_insert_unique.c'
s = open(p, encoding='utf-8').read()
old_ext = re.search(r'extern hwreq_map_node \*tree_splice_insert\(hwreq_map_node \*\*parent_holder, uint8_t insert_as_left,\n    const hwreq_map_value_type \*value\); // 0x57c390, UNSURE: signature guessed, not this pass', s).group(0)
s = s.replace(old_ext, NEW_EXTERN)
old1 = '''    if (went_left) {
        if (parent == (hwreq_map_node *)head->left) {
            hwreq_map_node *inserted = tree_splice_insert(&parent, 1, value);'''
new1 = '''    where = parent; // FIXED 2026-09-28: the insertion parent is the descent node (EBX at 0x57c283 / 0x57c2da);
                    //   only the comparison uses the decremented copy
    if (went_left) {
        if (parent == (hwreq_map_node *)head->left) {
            hwreq_map_node *holder;
            hwreq_map_node *inserted = *tree_splice_insert(tree, where, &holder, 1, value);'''
assert old1 in s
s = s.replace(old1, new1)
old2 = '''        hwreq_map_node *inserted = tree_splice_insert(&parent, went_left, value);'''
new2 = '''        hwreq_map_node *holder;
        hwreq_map_node *inserted = *tree_splice_insert(tree, where, &holder, went_left, value);'''
assert old2 in s
s = s.replace(old2, new2)
old3 = '''    hwreq_map_node *parent = head;
    uint8_t went_left = 1;
'''
new3 = '''    hwreq_map_node *parent = head;
    hwreq_map_node *where;
    uint8_t went_left = 1;
'''
assert old3 in s
s = s.replace(old3, new3, 1)
open(p, 'w', encoding='utf-8').write(s)
print('patched', p)
