/**
 * @file standalone/data/link/d3dx.hpp
 * Link names of the D3DX entry points the original code calls (the data image and standalone/bridges.cpp provide them
 * under these C names); only src/rasterizer/d3dx.cpp includes this header.
 */
#pragma once

#include <stdint.h>

extern "C" {
unsigned int __stdcall D3DXGetFVFVertexSize(unsigned int fvf);
int D3DXCreateEffect(void *device, const void *data, uint32_t size, const void *defines, void *include, uint32_t flags, void *pool, void *out_effect, void **out_error_buffer);
}
