/* One entry per adapter, for the differential tester (harness/gen/difftest_table.c from harness/gen_hooks.py).
   shape: one item per C parameter, "<register>[kind]" joined by commas: register 0..5 = eax ecx edx ebx esi edi,
   -1 = next stack slot; kind p = pointer, f = float, i = integer, d = 8-byte value. ret: v(oid) i(nt) f(loat) i(nt64 as 'i'). */
typedef struct difftest_entry {
    unsigned long original;
    void (*adapter)(void);
    const char *name, *module, *shape;
    char ret;
    int safe;
} difftest_entry;
extern const difftest_entry difftest_table[];
extern const unsigned difftest_count;
