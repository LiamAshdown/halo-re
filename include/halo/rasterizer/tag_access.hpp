/**
 * @file include/halo/rasterizer/tag_access.hpp
 * Typed access to tag data the rasterizer reads: tag block elements and the runtime texture handle of a bitmap.
 */
#pragma once

#include <cstddef>
#include <cstdint>

#include "tags.h"
#include "halo/rasterizer/render_device.hpp"

namespace halo::rasterizer {

/** The first element of a loaded tag block, as the element type T. */
template <typename T>
inline T *tag_block_data(const TagReflexive &block)
{
    return reinterpret_cast<T *>(static_cast<uintptr_t>(block.pointer));
}

/** Element `index` of a loaded tag block. */
template <typename T>
inline T *tag_block_element(const TagReflexive &block, std::size_t index)
{
    return tag_block_data<T>(block) + index;
}

/** The Direct3D texture the bitmap data record carries at runtime (NULL when it has none). */
inline void *bitmap_hardware_texture(const BitmapData &bitmap)
{
    return d3d_arg(bitmap.hardware_texture).get();
}

}  // namespace halo::rasterizer
