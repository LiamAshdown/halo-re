/* One entry per rewritten function that has an inbound adapter (harness/gen/hook_table.c, from harness/gen_hooks.py). */
typedef struct hook_entry {
    unsigned long original;          /* entry address in halo.exe */
    void (*adapter)(void);           /* original convention -> the rewritten C function */
    unsigned long *calls;            /* incremented by the adapter on every call */
    const char *name, *module;
    int safe;                        /* 1: its whole call tree stays in rewritten or standard-convention code */
} hook_entry;
extern const hook_entry hook_table[];
extern const unsigned hook_count;
