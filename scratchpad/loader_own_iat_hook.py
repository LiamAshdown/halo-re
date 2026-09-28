p = "C:\\Users\\Liam-\\halo-re\\standalone\\loader.c"
t = open(p, encoding="utf-8").read()
old = '''    log_line("filled %d import slots (%d unresolved)", standalone_import_count, missing);
}
'''
assert t.count(old) == 1
t = t.replace(old, old + '''
/* The C calls Windows through this exe's own import table (each API is declared __stdcall and links against the SDK
   import libraries), so the CreateFileA data override must also sit in this module's import address table, not only
   in the retail slot fill_imports patches. Only relative paths with a copy under override\\ are redirected, so the
   loader's and the CRT's own file opens are unaffected. */
static void hook_own_create_file_a(void)
{
    unsigned char *base = (unsigned char *)GetModuleHandleA(NULL);
    IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)(base + ((IMAGE_DOS_HEADER *)base)->e_lfanew);
    IMAGE_DATA_DIRECTORY *dir = &nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    IMAGE_IMPORT_DESCRIPTOR *d;
    FARPROC real = GetProcAddress(GetModuleHandleA("kernel32.dll"), "CreateFileA");
    int hooked = 0;

    if (!real || !dir->VirtualAddress) return;
    if (!g_create_file_a) g_create_file_a = (create_file_a_fn)real;
    for (d = (IMAGE_IMPORT_DESCRIPTOR *)(base + dir->VirtualAddress); d->Name; d++) {
        IMAGE_THUNK_DATA *thunk = (IMAGE_THUNK_DATA *)(base + d->FirstThunk);
        for (; thunk->u1.Function; thunk++) {
            if ((FARPROC)thunk->u1.Function == real) {
                DWORD old_protect;
                VirtualProtect(&thunk->u1.Function, sizeof thunk->u1.Function, PAGE_READWRITE, &old_protect);
                thunk->u1.Function = (ULONG_PTR)standalone_create_file_a;
                VirtualProtect(&thunk->u1.Function, sizeof thunk->u1.Function, old_protect, &old_protect);
                hooked++;
            }
        }
    }
    log_line("CreateFileA override: %d entries in this module's import table", hooked);
}
''')
old = '''    fill_imports();
    emulate_crt_startup();'''
assert t.count(old) == 1
t = t.replace(old, '''    fill_imports();
    hook_own_create_file_a();
    emulate_crt_startup();''')
open(p, "w", encoding="utf-8", newline="\n").write(t)
print("ok")
