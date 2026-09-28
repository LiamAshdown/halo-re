"""in files that include win32.h, cast the arguments that pass the project's own copies of Windows structs (same
layout, snake_case field names, so they cannot simply become the SDK types) to the SDK pointer type the API expects"""
import glob, re
R = "C:\\Users\\Liam-\\halo-re\\"
FT, CFT, ST, SA = "LPFILETIME", "const FILETIME *", "LPSYSTEMTIME", "LPSECURITY_ATTRIBUTES"
RULES = {
    "QueryPerformanceCounter": {0: "LARGE_INTEGER *"}, "QueryPerformanceFrequency": {0: "LARGE_INTEGER *"},
    "FindFirstFileA": {1: "LPWIN32_FIND_DATAA"}, "FindNextFileA": {1: "LPWIN32_FIND_DATAA"},
    "PeekMessageA": {0: "LPMSG"}, "GetMessageA": {0: "LPMSG"}, "TranslateMessage": {0: "const MSG *"},
    "DispatchMessageA": {0: "const MSG *"},
    "GetFileTime": {1: FT, 2: FT, 3: FT}, "SetFileTime": {1: CFT, 2: CFT, 3: CFT},
    "CompareFileTime": {0: CFT, 1: CFT}, "SystemTimeToFileTime": {0: "const SYSTEMTIME *", 1: FT},
    "FileTimeToSystemTime": {0: CFT, 1: ST}, "FileTimeToLocalFileTime": {0: CFT, 1: FT},
    "GetSystemTime": {0: ST}, "GetLocalTime": {0: ST},
    "VariantInit": {0: "VARIANTARG *"}, "VariantClear": {0: "VARIANTARG *"},
    "CreateMutexA": {0: SA}, "CreateEventA": {0: SA}, "CreateFileA": {3: SA}, "CreateFileMappingA": {1: SA},
    "CreateThread": {0: SA}, "CreateDirectoryA": {1: SA},
    "CreateProcessA": {2: SA, 3: SA, 8: "LPSTARTUPINFOA", 9: "LPPROCESS_INFORMATION"},
    "GetVersionExA": {0: "LPOSVERSIONINFOA"}, "RegisterClassExA": {0: "const WNDCLASSEXA *"},
    "CLSIDFromString": {1: "LPCLSID"}, "StringFromGUID2": {0: "REFGUID"},
    "GetWindowPlacement": {1: "WINDOWPLACEMENT *"}, "SetWindowPlacement": {1: "const WINDOWPLACEMENT *"},
    "GlobalMemoryStatus": {0: "LPMEMORYSTATUS"}, "CreateFontIndirectA": {0: "const LOGFONTA *"},
    "ReadConsoleInputA": {1: "PINPUT_RECORD"}, "AllocateAndInitializeSid": {0: "PSID_IDENTIFIER_AUTHORITY"},
    "AccessCheck": {3: "PGENERIC_MAPPING", 4: "PPRIVILEGE_SET"}, "WSAStartup": {1: "LPWSADATA"},
    "RegOpenKeyExA": {4: "PHKEY"}, "RegCreateKeyExA": {7: "PHKEY"},
    "GetDiskFreeSpaceExA": {1: "PULARGE_INTEGER", 2: "PULARGE_INTEGER", 3: "PULARGE_INTEGER"},
    "CoCreateInstance": {0: "REFCLSID", 3: "REFIID"}, "DuplicateHandle": {3: "LPHANDLE"}, "FormatMessageA": {4: "LPSTR"},
    "DialogBoxParamA": {3: "DLGPROC"}, "DialogBoxIndirectParamA": {3: "DLGPROC"}, "CreateDialogIndirectParamA": {3: "DLGPROC"},
}
CALL = re.compile(r"\b(%s)\s*\(" % "|".join(sorted(RULES, key=len, reverse=True)))


def split_args(s, start):
    """arguments of the call whose '(' is at start: [(begin, end)], index just past ')'"""
    depth, i, begin, out = 0, start, start + 1, []
    while i < len(s):
        c = s[i]
        if c in "([":
            depth += 1
        elif c in ")]":
            depth -= 1
            if depth == 0:
                out.append((begin, i))
                return out, i + 1
        elif c == "," and depth == 1:
            out.append((begin, i))
            begin = i + 1
        elif c == '"':
            i += 1
            while s[i] != '"':
                i += 2 if s[i] == "\\" else 1
        i += 1
    raise ValueError("unbalanced call")


def cast(arg, t):
    a = arg.strip()
    if not a or a in ("0", "(void *)0", "NULL") or a.startswith("(%s)" % t):
        return arg
    lead = arg[:len(arg) - len(arg.lstrip())]
    trail = arg[len(arg.rstrip()):]
    if re.fullmatch(r"&?[A-Za-z_][\w.\->\[\]]*", a):
        return "%s(%s)%s%s" % (lead, t, a, trail)
    return "%s(%s)(%s)%s" % (lead, t, a, trail)


files = casts = 0
for p in glob.glob(R + "src\\*\\*.c"):
    s = open(p, encoding="utf-8", errors="replace").read()
    if '#include "win32.h"' not in s:
        continue
    cut = s.find("\n#if 0")
    head, tail = (s, "") if cut < 0 else (s[:cut], s[cut:])
    out, pos, n = [], 0, 0
    for m in CALL.finditer(head):
        if m.start() < pos:
            continue
        line_start = head.rfind("\n", 0, m.start()) + 1
        if head[line_start:m.start()].lstrip().startswith(("//", "extern")):
            continue
        args, end = split_args(head, m.end() - 1)
        rule = RULES[m.group(1)]
        piece = head[m.start():m.end()]
        for k, (b, e) in enumerate(args):
            a = head[b:e]
            if k in rule:
                c = cast(a, rule[k])
                n += c != a
                a = c
            piece += a + ("," if k < len(args) - 1 else ")")
        out.append(head[pos:m.start()])
        out.append(piece)
        pos = end
    if n:
        out.append(head[pos:])
        open(p, "w", encoding="utf-8", newline="\n").write("".join(out) + tail)
        files += 1
        casts += n
print(casts, "arguments cast in", files, "files")
