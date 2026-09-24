/* Entry point of halo_rewrite.dll.
   hooks.txt next to the DLL selects which original functions are redirected to their rewritten C versions:
     one entry per line: a module name (e.g. math), a function name, or * for every safe function;
     a leading '-' excludes (e.g. -vector3d_normalize); '!' before a name forces a function whose call tree is not
     known to be safe; '#' starts a comment. No hooks.txt: nothing is redirected.
   Each selected original entry gets a 5-byte jmp to its adapter. Log: halo_rewrite.log next to the DLL, with the
   hooks installed, a crash report (faulting address, the function it is in), and per-function call counts at exit. */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "hook_table.h"

extern void *game_time;                  /* 0x006f1d6c, resolved to the live game global by harness/gen_link.py */
extern short global_structure_bsp_index; /* 0x0069e8d8 */

static char base_dir[MAX_PATH];
static unsigned char *installed;          /* per hook_table entry */
static HMODULE self;

void harness_log(const char *fmt, ...)
{
    FILE *f; va_list ap; SYSTEMTIME t; char path[MAX_PATH];
    if (!base_dir[0]) return;
    sprintf_s(path, MAX_PATH, "%shalo_rewrite.log", base_dir);
    if (fopen_s(&f, path, "a") != 0 || !f) return;
    GetLocalTime(&t);
    fprintf(f, "%02d:%02d:%02d.%03d ", t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);
    va_start(ap, fmt); vfprintf(f, fmt, ap); va_end(ap);
    fputc('\n', f); fclose(f);
}

#define MAX_HOOK_LINES 8192
static int selected(const hook_entry *h, char (*lines)[96], int n)
{
    int i, on = 0;
    for (i = 0; i < n; i++) {
        const char *l = lines[i]; int neg = l[0] == '-', force = l[0] == '!';
        const char *w = (neg || force) ? l + 1 : l;
        if (strcmp(w, "*") == 0 || strcmp(w, h->module) == 0 || strcmp(w, h->name) == 0) {
            if (neg) on = 0;
            else if (h->safe || (force && strcmp(w, h->name) == 0)) on = 1;
        }
    }
    return on;
}

static void install_hooks(void)
{
    char path[MAX_PATH], (*lines)[96] = (char (*)[96])malloc(MAX_HOOK_LINES * 96); int n = 0; FILE *f; unsigned i, count = 0, modules_hit = 0;
    sprintf_s(path, MAX_PATH, "%shooks.txt", base_dir);
    if (fopen_s(&f, path, "r") != 0 || !f) { harness_log("no hooks.txt: nothing redirected"); return; }
    while (n < MAX_HOOK_LINES && fgets(lines[n], sizeof lines[n], f)) {
        char *e = lines[n] + strcspn(lines[n], "#\r\n"); *e = 0;
        while (e > lines[n] && (e[-1] == ' ' || e[-1] == '\t')) *--e = 0;
        if (lines[n][0]) n++;
    }
    fclose(f);
    installed = (unsigned char *)calloc(hook_count, 1);
    for (i = 0; i < hook_count; i++) {
        const hook_entry *h = &hook_table[i]; unsigned char *p = (unsigned char *)h->original; DWORD old;
        if (!selected(h, lines, n)) continue;
        if (!VirtualProtect(p, 5, PAGE_EXECUTE_READWRITE, &old)) { harness_log("VirtualProtect failed at %08lx %s", h->original, h->name); continue; }
        p[0] = 0xE9; *(long *)(p + 1) = (long)((unsigned char *)h->adapter - (p + 5));
        VirtualProtect(p, 5, old, &old);
        installed[i] = 1; count++;
    }
    FlushInstructionCache(GetCurrentProcess(), NULL, 0);
    (void)modules_hit;
    harness_log("hooks.txt: %d lines, %u of %u functions redirected to the rewrite", n, count, hook_count);
    for (i = 0; i < hook_count && count <= 40; i++) if (installed[i]) harness_log("  hooked %08lx %s (%s)", hook_table[i].original, hook_table[i].name, hook_table[i].module);
}

static const char *function_at(unsigned long a, unsigned long *start)
{
    /* nearest hook_table entry at or below a: names both original addresses and (roughly) rewritten ones */
    unsigned i; const char *best = "?"; unsigned long b = 0;
    for (i = 0; i < hook_count; i++)
        if (hook_table[i].original <= a && hook_table[i].original > b) { b = hook_table[i].original; best = hook_table[i].name; }
    *start = b; return best;
}

static void log_counts(void);

static LONG CALLBACK crash_logger(EXCEPTION_POINTERS *x)
{
    static int logged;
    DWORD code = x->ExceptionRecord->ExceptionCode; unsigned long eip = (unsigned long)x->ContextRecord->Eip, s;
    if (code != EXCEPTION_ACCESS_VIOLATION && code != EXCEPTION_ILLEGAL_INSTRUCTION && code != EXCEPTION_PRIV_INSTRUCTION
        && code != EXCEPTION_STACK_OVERFLOW && code != EXCEPTION_INT_DIVIDE_BY_ZERO) return EXCEPTION_CONTINUE_SEARCH;
    static volatile LONG in_logger;
    if (InterlockedExchange(&in_logger, 1)) return EXCEPTION_CONTINUE_SEARCH;   /* a fault inside the logger itself */
    if (logged++ < 5) {
        MEMORY_BASIC_INFORMATION mi; char mod[MAX_PATH] = "?";
        if (VirtualQuery((void *)eip, &mi, sizeof mi) && GetModuleFileNameA((HMODULE)mi.AllocationBase, mod, MAX_PATH)) {}
        if (eip >= 0x401000 && eip < 0x700000) {
            const char *fn = function_at(eip, &s);
            harness_log("EXCEPTION %08lx at %08lx in halo.exe, in or after %s+0x%lx", code, eip, fn, eip - s);
        } else harness_log("EXCEPTION %08lx at %08lx in %s (rewrite DLL: look it up in halo_rewrite.map)", code, eip, mod);
        harness_log("  eax=%08lx ebx=%08lx ecx=%08lx edx=%08lx esi=%08lx edi=%08lx esp=%08lx ebp=%08lx  fault address=%08lx",
            x->ContextRecord->Eax, x->ContextRecord->Ebx, x->ContextRecord->Ecx, x->ContextRecord->Edx, x->ContextRecord->Esi,
            x->ContextRecord->Edi, x->ContextRecord->Esp, x->ContextRecord->Ebp,
            x->ExceptionRecord->NumberParameters > 1 ? (unsigned long)x->ExceptionRecord->ExceptionInformation[1] : 0);
        if (logged == 1) log_counts();   /* which rewritten functions ran before the crash */
        {   /* return addresses on the stack that point into code: a rough call stack (readable pages only) */
            unsigned long *sp = (unsigned long *)x->ContextRecord->Esp; int k, shown = 0;
            MEMORY_BASIC_INFORMATION smi; unsigned long stack_end = 0;
            if (VirtualQuery(sp, &smi, sizeof smi)) stack_end = (unsigned long)smi.BaseAddress + smi.RegionSize;
            for (k = 0; k < 1024 && shown < 24 && (unsigned long)(sp + k + 1) <= stack_end; k++) {
                unsigned long v = sp[k];
                if ((v >= 0x401000 && v < 0x700000) || (v >= 0x30001000 && v < 0x31000000)) {
                    if (v < 0x700000) harness_log("  stack: %08lx  %s", v, function_at(v, &s));
                    else harness_log("  stack: %08lx  (rewrite DLL)", v);
                    shown++;
                }
            }
        }
    }
    in_logger = 0;
    return EXCEPTION_CONTINUE_SEARCH;
}

static int by_calls(const void *a, const void *b)
{
    unsigned long ca = *hook_table[*(const unsigned *)a].calls, cb = *hook_table[*(const unsigned *)b].calls;
    return ca < cb ? 1 : ca > cb ? -1 : 0;
}

static void log_counts(void)
{
    unsigned i, n = 0, *idx; unsigned long total = 0;
    if (!installed) return;
    idx = (unsigned *)malloc(hook_count * sizeof *idx);
    for (i = 0; i < hook_count; i++) if (installed[i]) { idx[n++] = i; total += *hook_table[i].calls; }
    qsort(idx, n, sizeof *idx, by_calls);
    harness_log("calls into rewritten functions: %lu total across %u hooked functions", total, n);
    for (i = 0; i < n; i++) if (*hook_table[idx[i]].calls)
        harness_log("  %10lu  %s (%s)", *hook_table[idx[i]].calls, hook_table[idx[i]].name, hook_table[idx[i]].module);
    free(idx);
}

/* Freeze watchdog: every second, sample the game's main thread (the one that loaded this DLL) -- its EIP, the
   code addresses on its stack, and the hooked functions called most since the last sample -- and rewrite
   halo_watchdog.log with the last 12 samples. After a hang, that file shows the loop the thread is stuck in. */
static HANDLE main_thread;
static unsigned long *last_counts;
#define WD_SAMPLES 12
static char wd_ring[WD_SAMPLES][1024];
static int wd_next;
static int code_address(DWORD a) { return (a >= 0x401000 && a < 0x632000) || (a >= (DWORD)(size_t)self && a < (DWORD)(size_t)self + 0x400000); }
static DWORD WINAPI watchdog(LPVOID unused)
{
    (void)unused;
    last_counts = (unsigned long *)calloc(hook_count, sizeof *last_counts);
    for (;;) {
        CONTEXT c; char *s; int len, i, found = 0; unsigned k, top[3] = {0, 0, 0}; unsigned long d[3] = {0, 0, 0};
        MEMORY_BASIC_INFORMATION mbi; char path[MAX_PATH]; FILE *f; SYSTEMTIME t;
        Sleep(1000);
        if (!installed) continue;
        c.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;
        if (SuspendThread(main_thread) == (DWORD)-1) continue;
        if (!GetThreadContext(main_thread, &c)) { ResumeThread(main_thread); continue; }
        s = wd_ring[wd_next]; GetLocalTime(&t);
        len = sprintf_s(s, 1024, "%02d:%02d:%02d eip=%08lx esp=%08lx ebp=%08lx stack:", t.wHour, t.wMinute, t.wSecond, c.Eip, c.Esp, c.Ebp);
        if (VirtualQuery((void *)(size_t)c.Esp, &mbi, sizeof mbi) && mbi.State == MEM_COMMIT) {
            DWORD *sp = (DWORD *)(size_t)c.Esp, *end = (DWORD *)((char *)mbi.BaseAddress + mbi.RegionSize);
            for (; sp < end && found < 16 && sp < (DWORD *)(size_t)c.Esp + 4096; sp++)
                if (code_address(*sp)) { len += sprintf_s(s + len, 1024 - len, " %08lx", *sp); found++; }
        }
        ResumeThread(main_thread);
        for (k = 0; k < hook_count; k++) {
            unsigned long now = *hook_table[k].calls, dd = now - last_counts[k]; last_counts[k] = now;
            for (i = 0; i < 3; i++) if (dd > d[i]) { int j; for (j = 2; j > i; j--) { d[j] = d[j - 1]; top[j] = top[j - 1]; } d[i] = dd; top[i] = k; break; }
        }
        len += sprintf_s(s + len, 1024 - len, " | busiest:");
        for (i = 0; i < 3 && d[i]; i++) len += sprintf_s(s + len, 1024 - len, " %s=%lu", hook_table[top[i]].name, d[i]);
        wd_next = (wd_next + 1) % WD_SAMPLES;
        sprintf_s(path, MAX_PATH, "%shalo_watchdog.log", base_dir);
        if (fopen_s(&f, path, "w") == 0 && f) {
            for (i = 0; i < WD_SAMPLES; i++) { const char *r = wd_ring[(wd_next + i) % WD_SAMPLES]; if (r[0]) fprintf(f, "%s\n", r); }
            fclose(f);
        }
    }
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    (void)reserved;
    if (reason == DLL_PROCESS_ATTACH) {
        char *slash;
        self = instance;
        GetModuleFileNameA(instance, base_dir, MAX_PATH);
        slash = strrchr(base_dir, '\\'); if (slash) slash[1] = 0;
        harness_log("halo_rewrite.dll attached at %p (pid %lu), %u adapters available", (void *)instance, GetCurrentProcessId(), hook_count);
        harness_log("live globals: game_time=%p structure_bsp_index=%d", game_time, (int)global_structure_bsp_index);
        AddVectoredExceptionHandler(1, crash_logger);
        install_hooks();
        DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &main_thread, 0, FALSE, DUPLICATE_SAME_ACCESS);
        CreateThread(NULL, 0, watchdog, NULL, 0, NULL);
    } else if (reason == DLL_PROCESS_DETACH) {
        harness_log("detach: game_time=%p structure_bsp_index=%d", game_time, (int)global_structure_bsp_index);
        log_counts();
    }
    return TRUE;
}
