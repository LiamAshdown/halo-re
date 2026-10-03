/**
 * D3DX 2003 -> June 2010 compatibility for the standalone build.

   halo.exe links the 2003 (DirectX 9.0b era) D3DX statically, and the rewritten C calls ID3DXEffect methods through
   that version's vtable layout (0x00644d10, 71 slots; each slot's argument size checked against the `ret N` of the
   original method). The standalone build uses d3dx9_43 from the June 2010 SDK, whose ID3DXEffect differs after
   slot 53:
       2003                                  June 2010
       54 SetPixelShader                     (removed)
       55 GetPixelShader                     54
       56 SetVertexShader                    (removed)
       57 GetVertexShader                    55
                                             56 SetArrayRange (new)
       58 GetPool .. 64 Begin                57 .. 63
       65 Pass(pass)                         64 BeginPass(pass), 65 CommitChanges, 66 EndPass
       66 End                                67 End
       67 GetDevice, 68 OnLostDevice,        68, 69, 70
       69 OnResetDevice
       70 CloneEffect                        77
   Slots 0..53 (IUnknown and the parameter / technique / value accessors) are the same in both.

   standalone_d3dx_create_effect replaces D3DXCreateEffect for the rewritten C (tools/gen_standalone_link.py routes
   the symbol here): it creates the June 2010 effect and hands back a proxy whose vtable has the 2003 layout. Pass()
   is emulated as EndPass (when a pass is open) + BeginPass, which applies the pass with the parameter values set
   before it, as the 2003 Pass() did; End() closes an open pass first.

   The one exported name, standalone_d3dx_create_effect, is in the single extern "C" block at the end; the proxy lives
   in halo::standalone::d3dx. */
#include <windows.h>
#include <d3dx9.h>
#include <stdio.h>

extern "C" {

void __cdecl standalone_log(const char *format, ...);
long __cdecl standalone_d3dx_create_effect(IDirect3DDevice9 *device, const void *data, UINT size,
                                           const D3DXMACRO *defines, ID3DXInclude *include, DWORD flags,
                                           ID3DXEffectPool *pool, void **out_effect, ID3DXBuffer **out_errors);

}

namespace halo::standalone::d3dx {

struct effect_proxy {
    const void *const *vtable;
    ID3DXEffect *effect;
    int in_pass;
};

/* forward the call to the same-signature method of the June 2010 effect: replace `this` and jump */
#define FORWARD(name, slot)                                     \
    static __declspec(naked) void name(void)                    \
    {                                                           \
        __asm mov eax, [esp + 4]                                \
        __asm mov eax, [eax + 4]                                \
        __asm mov [esp + 4], eax                                \
        __asm mov ecx, [eax]                                    \
        __asm jmp dword ptr [ecx + 4 * slot]                    \
    }

FORWARD(f00, 0) FORWARD(f01, 1)
FORWARD(f03, 3) FORWARD(f04, 4) FORWARD(f05, 5) FORWARD(f06, 6) FORWARD(f07, 7) FORWARD(f08, 8) FORWARD(f09, 9)
FORWARD(f10, 10) FORWARD(f11, 11) FORWARD(f12, 12) FORWARD(f13, 13) FORWARD(f14, 14) FORWARD(f15, 15)
FORWARD(f16, 16) FORWARD(f17, 17) FORWARD(f18, 18) FORWARD(f19, 19) FORWARD(f20, 20) FORWARD(f21, 21)
FORWARD(f22, 22) FORWARD(f23, 23) FORWARD(f24, 24) FORWARD(f25, 25) FORWARD(f26, 26) FORWARD(f27, 27)
FORWARD(f28, 28) FORWARD(f29, 29) FORWARD(f30, 30) FORWARD(f31, 31) FORWARD(f32, 32) FORWARD(f33, 33)
FORWARD(f34, 34) FORWARD(f35, 35) FORWARD(f36, 36) FORWARD(f37, 37) FORWARD(f38, 38) FORWARD(f39, 39)
FORWARD(f40, 40) FORWARD(f41, 41) FORWARD(f42, 42) FORWARD(f43, 43) FORWARD(f44, 44) FORWARD(f45, 45)
FORWARD(f46, 46) FORWARD(f47, 47) FORWARD(f48, 48) FORWARD(f49, 49) FORWARD(f50, 50) FORWARD(f51, 51)
FORWARD(f52, 52) FORWARD(f53, 53)
FORWARD(get_pixel_shader, 54) FORWARD(get_vertex_shader, 55)
FORWARD(get_pool, 57) FORWARD(set_technique, 58) FORWARD(get_current_technique, 59) FORWARD(validate_technique, 60)
FORWARD(find_next_valid_technique, 61) FORWARD(is_parameter_used, 62) FORWARD(begin, 63)
FORWARD(get_device, 68) FORWARD(on_lost_device, 69) FORWARD(on_reset_device, 70) FORWARD(clone_effect, 77)

static ULONG __stdcall proxy_release(effect_proxy *proxy)
{
    ULONG count = proxy->effect->Release();
    if (count == 0) {
        HeapFree(GetProcessHeap(), 0, proxy);
    }
    return count;
}

static HRESULT __stdcall proxy_set_shader(effect_proxy *proxy, D3DXHANDLE parameter, void *shader)
{
    standalone_log("d3dx_compat: 2003 ID3DXEffect::Set%sShader called; June 2010 D3DX has no equivalent",
                   "Pixel/Vertex");
    return E_NOTIMPL;
}

static HRESULT __stdcall proxy_pass(effect_proxy *proxy, UINT pass)
{
    if (proxy->in_pass) {
        proxy->effect->EndPass();
        proxy->in_pass = 0;
    }
    if (proxy->effect->BeginPass(pass) < 0) {
        return D3DERR_INVALIDCALL;
    }
    proxy->in_pass = 1;
    return D3D_OK;
}

static HRESULT __stdcall proxy_end(effect_proxy *proxy)
{
    if (proxy->in_pass) {
        proxy->effect->EndPass();
        proxy->in_pass = 0;
    }
    return proxy->effect->End();
}

#define V(fn) ((const void *)(fn))

const void *const proxy_vtable[71] = {
    V(f00), V(f01), V(proxy_release), V(f03), V(f04), V(f05), V(f06), V(f07), V(f08), V(f09), V(f10), V(f11), V(f12),
    V(f13), V(f14), V(f15), V(f16), V(f17), V(f18), V(f19), V(f20), V(f21), V(f22), V(f23), V(f24), V(f25), V(f26),
    V(f27), V(f28), V(f29), V(f30), V(f31), V(f32), V(f33), V(f34), V(f35), V(f36), V(f37), V(f38), V(f39), V(f40),
    V(f41), V(f42), V(f43), V(f44), V(f45), V(f46), V(f47), V(f48), V(f49), V(f50), V(f51), V(f52), V(f53),
    V(proxy_set_shader), V(get_pixel_shader), V(proxy_set_shader), V(get_vertex_shader), V(get_pool),
    V(set_technique), V(get_current_technique), V(validate_technique), V(find_next_valid_technique),
    V(is_parameter_used), V(begin), V(proxy_pass), V(proxy_end), V(get_device), V(on_lost_device), V(on_reset_device),
    V(clone_effect),
};

#undef V

}  // namespace halo::standalone::d3dx

/** D3DXCreateEffect as the rewritten code calls it (cdecl, the 2003 argument list, which June 2010 kept). */
long __cdecl standalone_d3dx_create_effect(IDirect3DDevice9 *device, const void *data, UINT size,
                                           const D3DXMACRO *defines, ID3DXInclude *include, DWORD flags,
                                           ID3DXEffectPool *pool, void **out_effect, ID3DXBuffer **out_errors)
{
    using namespace halo::standalone::d3dx;
    ID3DXEffect *effect = nullptr;
    HRESULT hr = D3DXCreateEffect(device, data, size, defines, include, flags, pool, &effect, out_errors);

    if (hr < 0) {
        *out_effect = nullptr;
        return hr;
    }
    auto *proxy = static_cast<effect_proxy *>(HeapAlloc(GetProcessHeap(), 0, sizeof(effect_proxy)));
    proxy->vtable = proxy_vtable;
    proxy->effect = effect;
    proxy->in_pass = 0;
    *out_effect = proxy;
    return hr;
}
