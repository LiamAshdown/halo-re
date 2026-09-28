"""where a file includes win32.h (the SDK), the project's copies of Windows structs whose field names match the SDK
become the SDK types themselves, so values and pointers pass to the API without casts"""
import re
p = "C:\\Users\\Liam-\\halo-re\\types\\interface.h"
t = open(p, encoding="utf-8").read()
ALIASES = [("win32_coord", "COORD"), ("win32_small_rect", "SMALL_RECT"),
           ("win32_console_screen_buffer_info", "CONSOLE_SCREEN_BUFFER_INFO"),
           ("win32_console_cursor_info", "CONSOLE_CURSOR_INFO"), ("win32_rect", "RECT"), ("win32_point", "POINT")]
for name, sdk in ALIASES:
    m = re.search(r"typedef struct %s \{.*?\} %s;[^\n]*\n" % (name, name), t, re.S)
    assert m, name
    block = m.group(0)
    t = t.replace(block, "#ifdef HALO_WIN32_H   /* the SDK's own type where windows.h is in (same layout and field names) */\n"
                         "typedef %s %s;\n#else\n%s#endif\n" % (sdk, name, block), 1)
open(p, "w", encoding="utf-8", newline="\n").write(t)
print("aliased", len(ALIASES), "types")
