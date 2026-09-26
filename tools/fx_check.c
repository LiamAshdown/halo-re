/* fx_check: load every effect of a decrypted fx.bin (records of [int32 size][effect], as tools/convert_fx.py writes
   with --plain) with the DirectX SDK (June 2010) D3DX on a real device, then Begin/BeginPass/EndPass/End every pass
   of every technique. Prints one line per failure and "ok N bad M".
   Usage: fx_check <fx.bin.plain> [effect index] */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <d3d9.h>
#include <d3dx9.h>

static int check_effect(IDirect3DDevice9 *device, ID3DXEffectPool *pool, int index, const void *data, int size)
{
    D3DXMACRO defines[2] = {{"PS_2_0_TARGET", "ps_2_0"}, {0, 0}};
    ID3DXEffect *effect = 0;
    ID3DXBuffer *errors = 0;
    D3DXEFFECT_DESC desc;
    UINT technique, passes, pass;
    int good = 1;
    HRESULT hr = D3DXCreateEffect(device, data, size, defines, 0, 0, pool, &effect, &errors);

    if (hr < 0) {
        printf("effect %d: create %08lx %s\n", index, hr,
               errors ? (char *)errors->lpVtbl->GetBufferPointer(errors) : "");
        return 0;
    }
    effect->lpVtbl->GetDesc(effect, &desc);
    for (technique = 0; technique < desc.Techniques; technique++) {
        D3DXHANDLE handle = effect->lpVtbl->GetTechnique(effect, technique);
        if (effect->lpVtbl->SetTechnique(effect, handle) < 0 || effect->lpVtbl->Begin(effect, &passes, 0) < 0) {
            printf("effect %d technique %u: Begin failed\n", index, technique);
            good = 0;
            continue;
        }
        for (pass = 0; pass < passes; pass++) {
            hr = effect->lpVtbl->BeginPass(effect, pass);
            if (hr < 0) {
                printf("effect %d technique %u pass %u: BeginPass %08lx\n", index, technique, pass, hr);
                good = 0;
            } else {
                effect->lpVtbl->EndPass(effect);
            }
        }
        effect->lpVtbl->End(effect);
    }
    effect->lpVtbl->Release(effect);
    return good;
}

int main(int argc, char **argv)
{
    FILE *f;
    long size;
    unsigned char *data;
    long cursor = 0;
    int index = 0, ok = 0, bad = 0;
    int only = argc > 2 ? atoi(argv[2]) : -1;
    IDirect3D9 *d3d = Direct3DCreate9(D3D_SDK_VERSION);
    IDirect3DDevice9 *device = 0;
    ID3DXEffectPool *pool = 0;
    D3DPRESENT_PARAMETERS pp;
    HRESULT hr;

    setvbuf(stdout, 0, _IONBF, 0);
    if (argc < 2 || !(f = fopen(argv[1], "rb"))) {
        return 2;
    }
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
    data = malloc(size);
    fread(data, 1, size, f);
    fclose(f);
    ZeroMemory(&pp, sizeof pp);
    pp.Windowed = TRUE;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.hDeviceWindow = GetDesktopWindow();
    hr = IDirect3D9_CreateDevice(d3d, 0, D3DDEVTYPE_HAL, GetDesktopWindow(), D3DCREATE_SOFTWARE_VERTEXPROCESSING, &pp,
                                 &device);
    if (hr < 0) {
        printf("device %08lx\n", hr);
        return 1;
    }
    D3DXCreateEffectPool(&pool);
    while (cursor + 4 <= size - 0x21) {
        int effect_size = *(int *)(data + cursor);
        if (only < 0 || only == index) {
            if (check_effect(device, pool, index, data + cursor + 4, effect_size)) {
                ok++;
            } else {
                bad++;
            }
        }
        cursor += 4 + effect_size;
        index++;
    }
    printf("ok %d bad %d\n", ok, bad);
    return 0;
}
