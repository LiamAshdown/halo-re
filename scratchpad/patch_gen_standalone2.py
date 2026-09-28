import os
os.chdir(r'C:\Users\Liam-\halo-re')
p = 'tools/gen_standalone.py'
s = open(p, encoding='utf-8').read()
old = '''        (0x64dff0, 0x64dff4),
    ]'''
new = '''        (0x64dff0, 0x64dff4),
    ]
    # Code pointers that are real but can never be followed in the standalone. catch(...) handler addresses in the
    # __CxxFrameHandler HandlerType arrays of the retail hwreq C++ functions (std::map / std::vector helpers at
    # 0x57bf00..0x57cf00): the CRT only reaches them while unwinding through those retail frames, and their C versions
    # register no EH frames. They are EBP-based funclets of the parent frame (e.g. 0x57c743 reads [ebp+0x8]), so they
    # cannot have C of their own. (2026-09-28)
    DEAD_CODE_POINTER_SLOTS = {
        0x673648: 0x57c7e2, 0x673658: 0x57c743, 0x6736e8: 0x57ccc4, 0x6737d0: 0x57ced1,
        0x673990: 0x57bfec, 0x6739a0: 0x57c0a8,
    }'''
assert old in s
s = s.replace(old, new, 1)
old2 = '''            if any(lo <= va < hi for lo, hi in NOT_CODE_POINTER_RANGES):
                continue'''
new2 = '''            if any(lo <= va < hi for lo, hi in NOT_CODE_POINTER_RANGES):
                continue
            if va in DEAD_CODE_POINTER_SLOTS:
                assert struct.unpack_from("<I", exe, s["raw"] + o)[0] == DEAD_CODE_POINTER_SLOTS[va]
                continue'''
assert old2 in s
s = s.replace(old2, new2, 1)
open(p, 'w', encoding='utf-8').write(s)
print('patched')
