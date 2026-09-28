import os, re
os.chdir(r'C:\Users\Liam-\halo-re')

SETTERS = [
    (0x57d120, 'LinearTextureAddressing', 0x722b28, 0x69fe54),
    (0x57d130, 'UseAnisotropicFilter', 0x722b68, 0x69fee4),
    (0x57d140, 'DisableSpecular', 0x722b6c, 0x69fedc),
    (0x57d150, 'LinearTextureAddressingZoom', 0x722b2c, 0x69fe5c),
    (0x57d160, 'LinearTextureAddressingSun', 0x722b30, 0x69fe64),
    (0x57d170, 'UseFixedFunction', 0x722b34, 0x69fe74),
    (0x57d180, 'DisableRenderTargets', 0x722b70, 0x69fe84),
    (0x57d190, 'DisableAlphaRenderTargets', 0x722b74, 0x69fe8c),
    (0x57d1a0, 'UseAlternateConvolveMask', 0x722b78, 0x69fe9c),
    (0x57d1b0, 'PrototypeCard', 0x722b40, 0x69ff1c),
    (0x57d1c0, 'UnsupportedCard', 0x722b3c, 0x69fe7c),
    (0x57d1d0, 'OldDriver', 0x722b44, 0x69fea4),
    (0x57d1e0, 'EnableStopStart', 0x722b58, 0x69feac),
    (0x57d1f0, 'HeadRelativeSpeech', 0x722b5c, 0x69feb4),
    (0x57d200, 'OldSoundDriver', 0x722b48, 0x69febc),
    (0x57d210, 'InvalidDriver', 0x722b4c, 0x69fec4),
    (0x57d220, 'InvalidSoundDriver', 0x722b50, 0x69fecc),
]

def snake(k):
    return re.sub(r'(?<=[a-z])(?=[A-Z])', '_', k).lower()

for addr, key, glob, slot in SETTERS:
    s = snake(key)
    name = 'config_set_' + s
    p = 'src/shell/%s.c' % name
    assert not os.path.exists(p), p
    open(p, 'w', encoding='utf-8').write('''// %(name)s  (not a Ghidra function; the "%(key)s" entry of the config.txt property table at 0x0069fe40,
//   setter slot 0x%(slot)08x; no C existed, so the stored pointer trapped as unlisted_%(addr)x)
// address 0x%(addr)x, size 11 bytes
// name confidence: 0.8   rewrite confidence: 0.95
// WRITTEN 2026-09-28 from objdump 0x%(addr)x..0x%(end)x: mov eax, 1 / mov [0x%(glob)08x], eax / ret -- a bare presence
//   flag like config_set_safe_mode; the value string is not read and EAX = 1 comes back.
// blam-cc: (value on the stack, unused).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

extern int32_t config_%(s)s; // 0x%(glob)08x

// config.txt "%(key)s" setter: presence alone sets the flag.
uint8_t %(name)s(const char *value)
{
    (void)value;
    config_%(s)s = 1;
    return 1;
}
''' % dict(name=name, key=key, slot=slot, addr=addr, end=addr + 10, glob=glob, s=s))
    print('wrote', p)
