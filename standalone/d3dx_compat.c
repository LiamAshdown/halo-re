/* D3DX 2003 -> June 2010 compatibility for the standalone build.

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
   before it, as the 2003 Pass() did; End() closes an open pass first. */
#include <windows.h>
#include <d3dx9.h>
#include <stdio.h>

typedef struct effect_proxy {
    const void **vtable;
    ID3DXEffect *effect;
    int in_pass;
} effect_proxy;

void __cdecl standalone_log(const char *format, ...);

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
    ULONG count = proxy->effect->lpVtbl->Release(proxy->effect);
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
        proxy->effect->lpVtbl->EndPass(proxy->effect);
        proxy->in_pass = 0;
    }
    if (proxy->effect->lpVtbl->BeginPass(proxy->effect, pass) < 0) {
        return D3DERR_INVALIDCALL;
    }
    proxy->in_pass = 1;
    return D3D_OK;
}

static HRESULT __stdcall proxy_end(effect_proxy *proxy)
{
    if (proxy->in_pass) {
        proxy->effect->lpVtbl->EndPass(proxy->effect);
        proxy->in_pass = 0;
    }
    return proxy->effect->lpVtbl->End(proxy->effect);
}

static const void *proxy_vtable[71] = {
    f00, f01, proxy_release,
    f03, f04, f05, f06, f07, f08, f09, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24,
    f25, f26, f27, f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40, f41, f42, f43, f44, f45, f46,
    f47, f48, f49, f50, f51, f52, f53,
    proxy_set_shader, get_pixel_shader, proxy_set_shader, get_vertex_shader,
    get_pool, set_technique, get_current_technique, validate_technique, find_next_valid_technique,
    is_parameter_used, begin, proxy_pass, proxy_end, get_device, on_lost_device, on_reset_device, clone_effect,
};

/* D3DXCreateEffect as the rewritten C calls it (cdecl, the 2003 argument list, which June 2010 kept) */
long __cdecl standalone_d3dx_create_effect(IDirect3DDevice9 *device, const void *data, UINT size,
                                           const D3DXMACRO *defines, ID3DXInclude *include, DWORD flags,
                                           ID3DXEffectPool *pool, void **out_effect, ID3DXBuffer **out_errors)
{
    ID3DXEffect *effect = 0;
    HRESULT hr = D3DXCreateEffect(device, data, size, defines, include, flags, pool, &effect, out_errors);
    effect_proxy *proxy;

    if (hr < 0) {
        *out_effect = 0;
        return hr;
    }
    proxy = (effect_proxy *)HeapAlloc(GetProcessHeap(), 0, sizeof *proxy);
    proxy->vtable = proxy_vtable;
    proxy->effect = effect;
    proxy->in_pass = 0;
    *out_effect = proxy;
    return hr;
}
