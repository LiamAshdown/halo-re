/* difftest.exe: differential test of rewritten functions against the originals in halo.exe, outside the game.
   Maps bin/halo.exe at its image base 0x400000 (so every absolute global the rewrite uses points into it), then for
   each selected adapter runs N random samples: the same register and stack inputs (pointer parameters get
   separate but identical buffers of random floats) go to the original function and to the rewrite's adapter, with
   all of halo.exe's .data restored in between. Compares the return value, every buffer and all of .data.
   Usage: difftest.exe <path to halo.exe> [module or function ...] [-n samples] [-v]
   Build: python harness/gen_link.py --difftest */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#define ABSF(x) ((x) < 0 ? -(x) : (x))
#include "difftest.h"

void difftest_call(void *target, unsigned long regs[6], unsigned long *stack, int n, int float_return, unsigned long out[4]);

#define IMAGE_BASE 0x400000u
#define BUF_BYTES 1024
#define GUARD 256   /* identical bytes before and after every buffer, so small out-of-range reads compare equal */
#define MAX_PARAMS 16
static unsigned char *data_start; static size_t data_size;
static unsigned char *data_snapshot, *data_after_original;
static unsigned rng = 12345;
static unsigned rnd(void) { rng = rng * 1103515245u + 12345u; return rng >> 8; }
static float rndf(void) { return ((float)(rnd() % 20001) / 10000.0f - 1.0f) * 4.0f; }

static int map_halo(const char *path)
{
    FILE *f; long len; unsigned char *file, *img; IMAGE_NT_HEADERS32 *nt; IMAGE_SECTION_HEADER *s; unsigned i;
    if (fopen_s(&f, path, "rb") || !f) { printf("cannot open %s\n", path); return 0; }
    fseek(f, 0, SEEK_END); len = ftell(f); fseek(f, 0, SEEK_SET);
    file = (unsigned char *)malloc(len); fread(file, 1, len, f); fclose(f);
    nt = (IMAGE_NT_HEADERS32 *)(file + ((IMAGE_DOS_HEADER *)file)->e_lfanew);
    img = (unsigned char *)VirtualAlloc((void *)IMAGE_BASE, nt->OptionalHeader.SizeOfImage, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    if (img != (unsigned char *)IMAGE_BASE) { printf("could not map halo.exe at 0x%x (error %lu)\n", IMAGE_BASE, GetLastError()); return 0; }
    memcpy(img, file, nt->OptionalHeader.SizeOfHeaders);
    s = IMAGE_FIRST_SECTION(nt);
    for (i = 0; i < nt->FileHeader.NumberOfSections; i++, s++) {
        memcpy(img + s->VirtualAddress, file + s->PointerToRawData, min(s->SizeOfRawData, s->Misc.VirtualSize));
        if (memcmp(s->Name, ".data", 6) == 0) { data_start = img + s->VirtualAddress; data_size = s->Misc.VirtualSize; }
    }
    free(file);
    data_snapshot = (unsigned char *)malloc(data_size); data_after_original = (unsigned char *)malloc(data_size);
    memcpy(data_snapshot, data_start, data_size);
    return data_start != 0;
}

static int close_enough(float a, float b)
{
    if (a == b) return 1;
    if (_isnan(a) && _isnan(b)) return 1;
    if (!_finite(a) || !_finite(b)) return 0;
    return ABSF(a - b) <= 1e-4f * max(1.0f, max(ABSF(a), ABSF(b)));
}

/* compare two regions word by word, as floats when they look like floats; returns the first bad offset or -1 */
static long compare_region(const unsigned char *a, const unsigned char *b, size_t n)
{
    size_t i;
    for (i = 0; i + 4 <= n; i += 4) {
        unsigned long wa = *(const unsigned long *)(a + i), wb = *(const unsigned long *)(b + i);
        if (wa != wb && !close_enough(*(const float *)(a + i), *(const float *)(b + i))) return (long)i;
    }
    return -1;
}

static int guarded_call(void *target, unsigned long regs[6], unsigned long *stack, int n, int fr, unsigned long out[4])
{
    __try { difftest_call(target, regs, stack, n, fr, out); return 1; }
    __except (EXCEPTION_EXECUTE_HANDLER) { return 0; }
}

static int selected(const difftest_entry *e, int argc, char **argv)
{
    int i, any = 0;
    for (i = 2; i < argc; i++) {
        if (argv[i][0] == '-') { if (argv[i][1] == 'n') i++; continue; }
        any = 1;
        if (!strcmp(argv[i], e->module) || !strcmp(argv[i], e->name) || !strcmp(argv[i], "*")) return 1;
    }
    return !any;
}

/* By the time main runs, the loader's heaps and mapped files occupy 0x400000. So the first instance starts a
   suspended copy of itself, reserves halo.exe's range in it before its loader initializes, and waits for it. */
static int run_in_reserved_child(void)
{
    STARTUPINFOA si; PROCESS_INFORMATION pi; DWORD code = 1;
    ZeroMemory(&si, sizeof si); si.cb = sizeof si;
    SetEnvironmentVariableA("DIFFTEST_CHILD", "1");
    if (!CreateProcessA(NULL, GetCommandLineA(), NULL, NULL, TRUE, CREATE_SUSPENDED, NULL, NULL, &si, &pi)) {
        printf("CreateProcess failed (%lu)\n", GetLastError()); return 1; }
    {   /* the child dies with this process (a driver timeout kills only the parent) */
        HANDLE job = CreateJobObjectA(NULL, NULL); JOBOBJECT_EXTENDED_LIMIT_INFORMATION li;
        ZeroMemory(&li, sizeof li); li.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        if (job) { SetInformationJobObject(job, JobObjectExtendedLimitInformation, &li, sizeof li); AssignProcessToJobObject(job, pi.hProcess); }
    }
    if (!VirtualAllocEx(pi.hProcess, (void *)IMAGE_BASE, 0x491000, MEM_RESERVE, PAGE_EXECUTE_READWRITE))
        printf("warning: could not reserve 0x%x in the child (%lu)\n", IMAGE_BASE, GetLastError());
    ResumeThread(pi.hThread);
    WaitForSingleObject(pi.hProcess, INFINITE);
    GetExitCodeProcess(pi.hProcess, &code);
    return (int)code;
}

int main(int argc, char **argv)
{
    unsigned t, samples = 200, fails_total = 0, tested = 0; int verbose = 0, i; unsigned cw;
    if (argc < 2) { printf("usage: difftest <halo.exe> [module|function ...] [-n samples] [-v]\n"); return 1; }
    if (!GetEnvironmentVariableA("DIFFTEST_CHILD", NULL, 0)) return run_in_reserved_child();
    setvbuf(stdout, NULL, _IONBF, 0);
    for (i = 2; i < argc; i++) { if (!strcmp(argv[i], "-n") && i + 1 < argc) samples = atoi(argv[i + 1]); if (!strcmp(argv[i], "-v")) verbose = 1; }
    if (!map_halo(argv[1])) return 1;
    _controlfp_s(&cw, _PC_24, _MCW_PC);          /* Direct3D leaves the x87 unit in single precision in the game */
    for (t = 0; t < difftest_count; t++) {
        const difftest_entry *e = &difftest_table[t];
        static unsigned char store_a[MAX_PARAMS][BUF_BYTES + 2 * GUARD], store_b[MAX_PARAMS][BUF_BYTES + 2 * GUARD];
        unsigned char *bufs_a[MAX_PARAMS], *bufs_b[MAX_PARAMS];
        unsigned s, ran = 0, crashed_orig = 0, bad = 0, crashed_rewrite = 0; char first[256] = "";
        if (!selected(e, argc, argv)) continue;
        for (s = 0; s < samples; s++) {
            unsigned long regs_a[6], regs_b[6], stack_a[2 * MAX_PARAMS], stack_b[2 * MAX_PARAMS], out_a[4] = {0}, out_b[4] = {0};
            int ns = 0, np = 0, fr = e->ret == 'f', ok_a, ok_b; const char *p = e->shape; long off; int k;
            for (k = 0; k < 6; k++) regs_a[k] = regs_b[k] = 0x0badf00du;
            while (*p) {                                 /* build the inputs from the shape string */
                int reg = strtol(p, (char **)&p, 10); char kind = *p++; unsigned long va, vb, hi = 0;
                if (kind == 'p') {
                    bufs_a[np] = store_a[np] + GUARD; bufs_b[np] = store_b[np] + GUARD;
                    for (k = 0; k < (BUF_BYTES + 2 * GUARD) / 4; k++) ((float *)store_a[np])[k] = rndf();
                    memcpy(store_b[np], store_a[np], BUF_BYTES + 2 * GUARD);
                    va = (unsigned long)bufs_a[np]; vb = (unsigned long)bufs_b[np]; np++;
                } else if (kind == 'f') { float f = rndf(); va = vb = *(unsigned long *)&f; }
                else if (kind == 'd') { double d = rndf(); va = vb = ((unsigned long *)&d)[0]; hi = ((unsigned long *)&d)[1]; }
                else va = vb = rnd() % 8;
                if (reg >= 0 && kind == 'i' && (s & 1)) { va |= 0xa5a50000u; vb = va; }   /* junk above a byte/word argument */
                if (reg >= 0) { regs_a[reg] = va; regs_b[reg] = vb; }
                else { stack_a[ns] = va; stack_b[ns] = vb; ns++; if (kind == 'd') { stack_a[ns] = stack_b[ns] = hi; ns++; } }
                if (*p == ',') p++;
            }
            memcpy(data_start, data_snapshot, data_size);
            ok_a = guarded_call((void *)e->original, regs_a, stack_a, ns, fr, out_a);
            if (!ok_a) { crashed_orig++; continue; }
            memcpy(data_after_original, data_start, data_size);
            memcpy(data_start, data_snapshot, data_size);
            ok_b = guarded_call((void *)e->adapter, regs_b, stack_b, ns, fr, out_b);
            ran++;
            if (!ok_b) { crashed_rewrite++; if (!bad++) sprintf_s(first, sizeof first, "rewrite crashed (original did not)"); continue; }
            {   /* compare the return at its declared width: a bool leaves junk above AL, a short above AX */
                unsigned long mask = e->ret == 'b' ? 0xffu : e->ret == 'w' ? 0xffffu : 0xffffffffu;
                out_a[0] &= mask; out_b[0] &= mask;
            }
            if (strchr("ibw", e->ret) && out_a[0] != out_b[0] && !(np && e->ret == 'i' && out_a[0] - (unsigned long)bufs_a[0] == out_b[0] - (unsigned long)bufs_b[0])) {
                if (!bad++) sprintf_s(first, sizeof first, "return 0x%08lx vs 0x%08lx", out_a[0], out_b[0]); continue; }
            if (fr && !close_enough((float)*(double *)&out_a[2], (float)*(double *)&out_b[2])) {
                if (!bad++) sprintf_s(first, sizeof first, "float return %g vs %g", *(double *)&out_a[2], *(double *)&out_b[2]); continue; }
            for (k = 0; k < np; k++) if ((off = compare_region(bufs_a[k], bufs_b[k], BUF_BYTES)) >= 0) {
                if (!bad++) sprintf_s(first, sizeof first, "pointer arg #%d +0x%lx: %g (0x%08lx) vs %g (0x%08lx)", k, off,
                    *(float *)(bufs_a[k] + off), *(unsigned long *)(bufs_a[k] + off), *(float *)(bufs_b[k] + off), *(unsigned long *)(bufs_b[k] + off));
                break; }
            if (k < np) continue;
            if ((off = compare_region(data_after_original, data_start, data_size)) >= 0) {
                if (!bad++) sprintf_s(first, sizeof first, "global 0x%08lx: 0x%08lx vs 0x%08lx", (unsigned long)(data_start - (unsigned char *)0) + off,
                    *(unsigned long *)(data_after_original + off), *(unsigned long *)(data_start + off)); }
        }
        memcpy(data_start, data_snapshot, data_size);
        tested++;
        if (bad) fails_total++;
        if (bad || verbose)
            printf("%-4s %-48s %-10s %3u/%3u differ%s%s  [%s]\n", bad ? "FAIL" : "ok", e->name, e->module, bad, ran,
                   crashed_orig ? " (original faulted on some inputs)" : "", "", e->shape);
        if (bad) printf("       first difference: %s\n", first);
        if (crashed_rewrite) printf("       rewrite crashed on %u of them; %u differ in value\n",
                                    crashed_rewrite, bad - crashed_rewrite);
    }
    printf("difftest: %u functions tested, %u differ\n", tested, fails_total);
    return fails_total ? 2 : 0;
}
