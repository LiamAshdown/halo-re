p = "C:\\Users\\Liam-\\halo-re\\src\\interface\\ui_controls_options_populate_from_profile.c"
t = open(p, encoding="utf-8").read()

old = '''    field = *(int32_t *)(record + 0x58);
    if ((uint32_t)(field - 1) < 0x19) {
        uint8_t index = jump_table_index_004a0283[field];
        uint32_t (*handler)(void) = (uint32_t (*)(void))jump_table_004a0268[index];
        return handler();
    }
    control->selection_index = 0;
'''
assert old in t
t = t.replace(old, '''    // the switch at 0x4a011d (index bytes 0x4a0284 by field - 1, cases 0x4a0268): each case stores its selection
    // and falls through to the next row
    switch (*(int32_t *)(record + 0x58)) {
    case 3: control->selection_index = 1; break;
    case 5: control->selection_index = 2; break;
    case 10: control->selection_index = 3; break;
    case 15: control->selection_index = 4; break;
    case 25: control->selection_index = 5; break;
    default: control->selection_index = 0; break;
    }
''', 1)

old = '''extern void *jump_table_004a0268[25]; // 0x4a0268, TYPES-GAP, UNSURE: see file header
extern uint8_t jump_table_index_004a0283[25]; // 0x4a0283, TYPES-GAP, UNSURE: see file header

'''
assert old in t
t = t.replace(old, '', 1)

old = '''// Phase-4 s2 review:'''
t = t.replace(old, '''// FIXED 2026-09-28 (retail-independence loop): the third row is a plain switch compiled to a jump table in .text
// (index bytes at 0x4a0284 indexed by value - 1, not 0x4a0283; case addresses at 0x4a0268): 3 -> 1, 5 -> 2,
// 10 -> 3, 15 -> 4, 25 -> 5, anything else -> 0, then the fourth and fifth rows as before. The C called the case
// labels as functions (retail code, never runnable in the standalone) and returned early.

// Phase-4 s2 review:''', 1)
open(p, "w", encoding="utf-8", newline="\n").write(t)
print("ok")
