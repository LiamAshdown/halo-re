#pragma once


namespace halo::projectiles {

static_assert(sizeof(projectile_constants) == 4);
static_assert(sizeof(projectile_flags) == 4);
static_assert(sizeof(projectile_definition_flags) == 4);
static_assert(sizeof(projectile_state) == 4);
static_assert(sizeof(projectile_network_state) == 24);
static_assert(sizeof(projectile_data) == 188);
static_assert(sizeof(projectile_object) == 688);
static_assert(sizeof(collision_result_type) == 4);
static_assert(sizeof(collision_result) == 80);
static_assert(sizeof(projectile_creation_message) == 84);
static_assert(sizeof(projectile_detonation_message) == 16);
static_assert(sizeof(projectile_attach_message) == 10);
static_assert(sizeof(projectile_network_update_header) == 7);

}
