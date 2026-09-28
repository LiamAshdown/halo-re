import os
os.chdir(r'C:\Users\Liam-\halo-re')

p = 'src/objects/objects_dump_memory.c'
s = open(p, encoding='utf-8').read()
old = 'extern void *fopen_00624186(const char *path, void *mode_or_flags); // 0x624186, UNSURE: see file header'
new = ('extern void *fopen(const char *path, const char *mode); // 0x624186 _fopen\n'
       '// FIXED 2026-09-28 (retail-independence loop): the second fopen argument is the mode "a+b" (0x00660144, pushed at\n'
       '//   0x4fa66d); the earlier 0 would crash in the CRT, and fopen_00624186 bound to nothing (a direct trap).')
assert old in s
s = s.replace(old, new)
old = 'void *file = fopen_00624186("object_memory.txt", 0);'
assert old in s
s = s.replace(old, 'void *file = fopen("object_memory.txt", "a+b"); // 0x0066e850, 0x00660144')
open(p, 'w', encoding='utf-8').write(s)
print('patched', p)

COMMON = '''#include "tags.h"
#include "memory.h"
#include "math.h"
#include <string.h>
#include <stdlib.h>

extern void *std_exception_vtable; // 0x0064ef90
'''
open('src/shell/exception_copy_construct.c', 'w', encoding='utf-8').write('''// exception_copy_construct  (CRT library code: std::exception::exception(const exception &), MSVC 7.1)
// address 0x627dd2, size 74 bytes
// name confidence: 0.9   rewrite confidence: 0.9
// WRITTEN 2026-09-28 from objdump 0x627dd2..0x627e1b: stores the std::exception vtable (0x0064ef90), copies the
//   owns-message flag (+0x08); an owned message (+0x04) is duplicated (malloc(strlen + 1), strcpy; a failed malloc
//   leaves NULL), otherwise the pointer is shared. Returns this. Reached from hwreq_parse_exception_copy_construct,
//   which calls it as an ordinary C function (the binary passes this in ECX).
// blam-cc: ECX this, stack -> other (callee pops 4)

''' + COMMON + '''
void *exception_copy_construct(void *this, const void *other)
{
    uint8_t *self = (uint8_t *)this;
    const uint8_t *source = (const uint8_t *)other;

    *(void **)self = &std_exception_vtable;
    *(uint32_t *)(self + 8) = *(const uint32_t *)(source + 8);
    if (*(const uint32_t *)(source + 8) != 0) {
        const char *message = *(const char *const *)(source + 4);
        char *copy = (char *)malloc(strlen(message) + 1);

        *(char **)(self + 4) = copy;
        if (copy != 0) {
            strcpy(copy, message);
        }
    } else {
        *(const char **)(self + 4) = *(const char *const *)(source + 4);
    }
    return this;
}
''')
open('src/shell/exception_destruct.c', 'w', encoding='utf-8').write('''// exception_destruct  (CRT library code: std::exception::~exception, MSVC 7.1)
// address 0x627e1c, size 22 bytes
// name confidence: 0.9   rewrite confidence: 0.95
// WRITTEN 2026-09-28 from objdump 0x627e1c..0x627e31: restores the std::exception vtable (0x0064ef90) and frees the
//   message (+0x04) when the owns-message flag (+0x08) is set. Reached from hwreq_parse_exception_destruct as an
//   ordinary C function (the binary passes this in ECX).
// blam-cc: ECX this

''' + COMMON + '''
void exception_destruct(void *this)
{
    uint8_t *self = (uint8_t *)this;

    *(void **)self = &std_exception_vtable;
    if (*(uint32_t *)(self + 8) != 0) {
        free(*(void **)(self + 4));
    }
}
''')
print('wrote exception methods')
