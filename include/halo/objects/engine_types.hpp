/**
 * @file include/halo/objects/engine_types.hpp
 * Pulls in the engine C type headers the object system API is declared against, in the order the original
 * sources used. The headers have no include guards, so every translation unit reaches them through here.
 */
#pragma once

#include "win32.h"
#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "halo/core/datum.hpp"
#include "halo/objects/flags.hpp"
struct Antenna;
struct DamageEffect;
struct ModelCollisionGeometry;
struct ModelCollisionGeometryMaterial;
struct ColorRGB;
struct Flag;
struct GBXModel;
struct ModelRegion;
struct TagReflexive;
struct bsp_leaf_reference;
struct damage_data;
struct data_array;
struct glow;
struct glow_particle;
struct object;
struct object_marker;
struct object_memory_dump_record;
struct object_placement_cursor;
struct object_placement_data;
struct object_shield_impulse_result;
struct object_statistics;
struct particle;
struct real_matrix4x3;
struct real_point3d;
struct real_vector3d;
struct render_lighting;
