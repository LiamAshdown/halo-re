/* d3d.h -- Direct3D 9 and D3DX from the DirectX SDK (June 2010) headers, for the few source files that call D3DX
   functions directly (the rest of the rasterizer reaches Direct3D through vtables). tools/msvc_build.py puts the
   SDK's Include folder on the include path; the standalone link takes the functions from d3dx9.lib. */
#ifndef HALO_D3D_H
#define HALO_D3D_H

#include "win32.h"
#include <corecrt_math.h>   /* the CRT math functions d3dx9math.inl uses (<math.h> finds types/math.h) */
#include <d3d9.h>
#include <d3dx9.h>

#endif
