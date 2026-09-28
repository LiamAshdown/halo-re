R = "C:\\Users\\Liam-\\halo-re\\"


def edit(rel, pairs):
    p = R + rel
    t = open(p, encoding="utf-8").read()
    for old, new in pairs:
        assert t.count(old) == 1, (rel, old[:70])
        t = t.replace(old, new)
    open(p, "w", encoding="utf-8", newline="\n").write(t)
    print("edited", rel)


# AccessCheck(descriptor, token, access, mapping, PrivilegeSet, PrivilegeSetLength, granted, status)
edit("src\\shell\\security_check_write_access.c", [
    ('''ok = AccessCheck(descriptor, impersonation_token, 1, (PGENERIC_MAPPING)&mapping, privilege_set,
                                          (PPRIVILEGE_SET)&privilege_set_length, &granted_access, &access_granted);''',
     '''ok = AccessCheck(descriptor, impersonation_token, 1, (PGENERIC_MAPPING)&mapping, (PPRIVILEGE_SET)privilege_set,
                                          &privilege_set_length, &granted_access, &access_granted);'''),
])
edit("scratchpad\\cast_win32_args.py", [
    ('"AccessCheck": {3: "PGENERIC_MAPPING", 5: "PPRIVILEGE_SET"}', '"AccessCheck": {3: "PGENERIC_MAPPING", 4: "PPRIVILEGE_SET"}'),
])
# the C runtime's own declarations come with the SDK headers now; local copies with int32_t sizes disagree
edit("src\\networking\\message_delta_parameters_protocol_register.c", [
    ('''extern char *strdup(const char *s);
extern int32_t strlen(const char *s);
extern void *memcpy(void *dest, const void *src, int32_t count);
extern char *strcpy(char *dest, const char *src);
''', ""),
    ('#include "win32.h"\n', '#include "win32.h"\n#include <string.h>\n'),
])
# GetDeviceID is DirectSound's (dsound.h / dsound.lib), not in windows.h
edit("src\\shell\\shell_detect_hardware_specs.c", [
    ('#include "win32.h"\n', '#include "win32.h"\n#include <dsound.h>\n'),
])
