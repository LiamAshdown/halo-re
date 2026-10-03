/**
 * @file include/halo/objects/tag_layout.hpp
 * Compile-time proof that the objects tag-definition members named after the engine code sit at the offsets it used to hard-code.
 */
#pragma once

#include <cstddef>
#include "tags.h"

static_assert(offsetof(ModelCollisionGeometry, unknown_20) == 0x20);
