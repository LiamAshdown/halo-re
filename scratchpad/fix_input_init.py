p = "C:\\Users\\Liam-\\halo-re\\src\\input\\input_system_initialize.c"
t = open(p, encoding="utf-8").read()
old = '''            (void *)0x491d70, (void *)0, 1); // EnumDevices, DI8DEVCLASS_GAMECTRL, DIEDFL_ATTACHEDONLY'''
assert t.count(old) == 1
t = t.replace(old, '''            (void *)input_enumerate_gamepad_callback, (void *)0, 1); // EnumDevices, DI8DEVCLASS_GAMECTRL, DIEDFL_ATTACHEDONLY''')
i = t.index("uint32_t input_system_initialize(void)")
t = t[:i] + '''// FIXED 2026-09-28 (retail-independence loop): the EnumDevices callback is the C input_enumerate_gamepad_callback,
// not the literal retail address 0x491d70 (original code the standalone cannot run).
extern int32_t __stdcall input_enumerate_gamepad_callback(const di_device_instance *instance, void *reference); // 0x491d70

''' + t[i:]
open(p, "w", encoding="utf-8", newline="\n").write(t)
print("ok")
