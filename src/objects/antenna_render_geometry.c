// antenna_render_geometry  (Ghidra: no function; called only from antenna_render_callback 0x4fac90)
// address 0x4fb340, size 399 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN from objdump 0x4fb340..0x4fb4ce. EDI = the Antenna tag, stack: the antenna.
//   Nothing for a tag without vertices (+0xc4). The lod fade is clamp01((100 - cutoff pixels +0x98) /
//   (falloff pixels +0x94 - cutoff)). A build_sprite_data on the stack takes the tag's bitmaps (+0x2c), one sprite
//   per vertex, the shader pointer 0x69e9f8, flags 4 and the zero centroid (0x6966f8). Every vertex with a texture
//   scale (+0x18) while the fade is positive becomes a sprite: build_sprite(EBX data, AX tag vertex sequence
//   +0x28, CX 0, mode 1, origin the vertex, direction to the next vertex, rotation 0, scale, the tag vertex
//   colour +0x2c, fade, flags 0); build_sprites_end(ESI data) closes the batch.
// blam-cc: EDI -> antenna_tag, stack -> ant

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "rasterizer.h"
#include "render.h"
#include "fn_objects.h"
#include <stdint.h>

extern real_point3d *global_zero_vector3d_pointer; // 0x006966f8 -> 0x0065c230 {0,0,0}
extern uint8_t antenna_sprite_shader[];            // 0x0069e9f8

extern void build_sprite(build_sprite_data *data, int16_t sequence_index, int16_t sprite_index, int16_t mode,
    real_point3d *origin, real_vector3d *direction, float rotation, float scale, ColorARGB *color, float fade,
    uint32_t flags); // 0x511700, EBX, AX, CX, stack
extern void build_sprites_end(build_sprite_data *data); // 0x511620, ESI

void antenna_render_geometry(Antenna *antenna_tag, antenna *ant)
{
    uint8_t *tag = (uint8_t *)antenna_tag;
    int32_t count = *(int32_t *)&((struct Antenna *)tag)->vertices.count;
    build_sprite_data data;
    float fade;
    int16_t i;

    if (count == 0) {
        return;
    }
    fade = (100.0f - ((struct Antenna *)tag)->cutoff_pixels) / (((struct Antenna *)tag)->falloff_pixels - ((struct Antenna *)tag)->cutoff_pixels);
    if (fade < 0.0f) {
        fade = 0.0f;
    } else if (fade > 1.0f) {
        fade = 1.0f;
    }
    data.bitmap_group_index = *(datum_index *)&((struct Antenna *)tag)->bitmaps.tag_id;
    data.maximum_sprite_count = (int16_t)count;
    data.shader = (uint32_t)(uintptr_t)antenna_sprite_shader;
    data.sprite_count = 0;
    data.flags = 4;
    data.centroid = *global_zero_vector3d_pointer;
    data.group_count = 0;

    for (i = 0; i < *(int32_t *)&((struct Antenna *)tag)->vertices.count; i++) {
        antenna_vertex *vertex = &ant->vertices[i];
        uint8_t *tag_vertex = *(uint8_t **)&((struct Antenna *)tag)->vertices.pointer + i * 0x80;
        real_vector3d direction;
        ColorARGB color;

        direction.i = ant->vertices[i + 1].position.x - vertex->position.x;
        direction.j = ant->vertices[i + 1].position.y - vertex->position.y;
        direction.k = ant->vertices[i + 1].position.z - vertex->position.z;
        color = *(ColorARGB *)(tag_vertex + 0x2c);
        if (vertex->texture_scale != 0.0f && fade > 0.0f) {
            build_sprite(&data, *(int16_t *)(tag_vertex + 0x28), 0, 1, &vertex->position, &direction, 0.0f,
                vertex->texture_scale, &color, fade, 0);
        }
    }
    build_sprites_end(&data);
}
