import re
R = "C:\\Users\\Liam-\\halo-re\\"


def edit(rel, pairs, include=None):
    p = R + rel
    t = open(p, encoding="utf-8").read()
    for old, new in pairs:
        if isinstance(old, re.Pattern):
            t, n = old.subn(new, t, count=1)
            assert n == 1, (rel, old.pattern[:60])
        else:
            assert t.count(old) == 1, (rel, old[:70])
            t = t.replace(old, new)
    if include and include not in t:
        i = t.index("#include")
        t = t[:i] + include + "\n" + t[i:]
    open(p, "w", encoding="utf-8", newline="\n").write(t)
    print("edited", rel)


DECL = lambda name: re.compile(r"^extern\s+[^;(]*?\b%s\s*\([^;]*?\)\s*;[^\n]*\n" % name, re.M | re.S)

edit("src\\rasterizer\\rasterizer_render_loading_screen.c", [
    (DECL("D3DXLoadSurfaceFromResourceA"), ""),
    ("D3DXLoadSurfaceFromResourceA(splash, 0, 0, (uint32_t)shell_module_handle, 0x86 /* MAKEINTRESOURCE */, 0,",
     "D3DXLoadSurfaceFromResourceA((LPDIRECT3DSURFACE9)splash, 0, 0, (HMODULE)shell_module_handle, MAKEINTRESOURCEA(0x86), 0,"),
], include='#include "d3d.h"')
edit("src\\rasterizer\\rasterizer_dx9_shaders_initialize.c", [
    (DECL("D3DXCreateEffectPool"), ""),
    ("hr = D3DXCreateEffectPool(&rasterizer_effect_pool);",
     "hr = D3DXCreateEffectPool((LPD3DXEFFECTPOOL *)&rasterizer_effect_pool);"),
], include='#include "d3d.h"')
edit("src\\rasterizer\\rasterizer_dx9_vertex_declarations_create.c", [
    (DECL("D3DXFVFFromDeclarator"), ""),
    ("D3DXFVFFromDeclarator(vertex_elements_model_processed, &rasterizer_vertex_declarations[15].fvf);",
     "D3DXFVFFromDeclarator((const D3DVERTEXELEMENT9 *)vertex_elements_model_processed,\n"
     "        (DWORD *)&rasterizer_vertex_declarations[15].fvf);"),
], include='#include "d3d.h"')
# WinINet: its SDK header and import library (tools/gen_standalone_link.py links wininet.lib)
edit("src\\networking\\autopatch_get_proxy_settings.c", [
    (DECL("InternetQueryOptionA"), ""),
    ('#include "win32.h"\n', '#include "win32.h"\n#include <wininet.h>\n'),
])
edit("tools\\gen_standalone_link.py", [
    ('EXTRA_LIBS = ["d3d9.lib", "d3dx9.lib", "legacy_stdio_definitions.lib"]',
     'EXTRA_LIBS = ["d3d9.lib", "d3dx9.lib", "legacy_stdio_definitions.lib", "wininet.lib"]'),
])
# the DirectX SDK headers on the include path (after types/, so the project's headers win)
edit("tools\\msvc_build.py", [
    ('CFLAGS = ["/nologo", "/c", "/TC", "/W3", "/Od", "/GS-", "/Oy-", "/Gy", "/wd4996", "/I", os.path.join(ROOT, "types"),',
     'DXSDK_INCLUDE = r"C:\\Program Files (x86)\\Microsoft DirectX SDK (June 2010)\\Include"\n'
     'CFLAGS = ["/nologo", "/c", "/TC", "/W3", "/Od", "/GS-", "/Oy-", "/Gy", "/wd4996", "/I", os.path.join(ROOT, "types"),\n'
     '          "/I", DXSDK_INCLUDE,'),
])
