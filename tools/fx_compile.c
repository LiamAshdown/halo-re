/* fx_compile: compile an effect source with the DirectX SDK (June 2010) D3DX and write the fx_2_0 binary.
   tools/convert_fx.py builds and runs it to obtain modern FXLC expressions for fx.bin's shader-constant states.
   Usage: fx_compile <in.fx> <out.fxo> */
#include <windows.h>
#include <stdio.h>
#include <d3dx9.h>

int main(int argc, char **argv)
{
    ID3DXEffectCompiler *compiler = 0;
    ID3DXBuffer *out = 0, *errors = 0;
    HRESULT hr;
    FILE *f;

    if (argc != 3) {
        fprintf(stderr, "usage: fx_compile <in.fx> <out.fxo>\n");
        return 2;
    }
    hr = D3DXCreateEffectCompilerFromFileA(argv[1], 0, 0, 0, &compiler, &errors);
    if (hr < 0) {
        fprintf(stderr, "compiler %08lx %s\n", hr, errors ? (char *)errors->lpVtbl->GetBufferPointer(errors) : "");
        return 1;
    }
    hr = compiler->lpVtbl->CompileEffect(compiler, 0, &out, &errors);
    if (hr < 0) {
        fprintf(stderr, "compile %08lx %s\n", hr, errors ? (char *)errors->lpVtbl->GetBufferPointer(errors) : "");
        return 1;
    }
    f = fopen(argv[2], "wb");
    if (!f) {
        return 1;
    }
    fwrite(out->lpVtbl->GetBufferPointer(out), 1, out->lpVtbl->GetBufferSize(out), f);
    fclose(f);
    return 0;
}
