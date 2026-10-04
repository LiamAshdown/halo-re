# MojoShader (vendored, patched)

MojoShader (https://github.com/icculus/mojoshader), commit ad5dff84830c2863c841f4b1f4e3df78c705b383, zlib licence
(LICENSE.txt). Only the files the OpenGL backend builds are kept: the parser, the effects reader and the GLSL profile
(plus the headers they include). It translates Direct3D 9 shader bytecode and fx_2_0 effects to GLSL for
`HALO_RENDERER=gl`; see docs/OPENGL_PORT.md.

Halo's 2003-era shaders and effects use paths upstream does not handle, so this copy is modified. Every change is
marked `halo-re` in the source, and `halo.patch` is the full diff against the upstream commit:

- vs_1_x relative addressing (`c[a0.x + n]`) without a CTAB: the whole float constant file is one uniform array.
- vs_1_x `mov a0.x, r`: explicit float-to-int conversion (GLSL 1.10 has no implicit one).
- ps_1_4 `texld rN, tM` (sampler N at the coordinates in tM), emitting texture2D, or textureCube/texture3D when the
  caller's sampler map says the bound texture is a cube or volume; `_dz`/`_dw` projective source modifiers.
- the 20-byte constant table header the 2003 compiler writes.
- effects: a pass state that names a shader variable points at that variable's bytecode.
- effects: `MOJOSHADER_effectState` keeps the state index (`index`, the stage of `Texture[n]`) and, for
  `Vertex/PixelShaderConstant*[n] = <param>` states, the parameter they copy (`param`, 1-based), taken from the
  usage-1 name or from the CTAB of the FXLC expression tools/convert_fx.py writes for them.
- debugging hooks `mojo_trace` and `mojo_ctab_line`, defined in src/rasterizer/gl_effect.cpp.

To update: take the new upstream files, re-apply halo.patch, rebuild, and check a level under `HALO_RENDERER=gl`
(world lit and textured, flashlight lights the BSP, fog).
