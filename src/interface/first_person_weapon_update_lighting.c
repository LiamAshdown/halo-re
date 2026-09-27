// first_person_weapon_update_lighting  (Ghidra: FUN_004924b0, unnamed)
// address 0x4924b0, size 568 bytes
// name confidence: 0.3   rewrite confidence: 0.55
// evidence: out/phase4/interface_functions.md "When the local player's weapon HUD elements are
// present, gathers first-person weapon node data and dispatches it (twice, for two different HUD
// interface elements) to a lower-level effect/render routine."; types/units.h unit::flags
// (+0x204), unknown_37c/unknown_380; types/objects.h object::function_out_values (+0x134) and
// object::change_colors (+0x1b8), read here off BOTH the weapon object and the unit object,
// confirming they are generic `object` fields, not unit/weapon-specific; local_player_index_for_
// unit.c's local_player_globals/player_data walk; hud_meter_permute_node_records.c and
// first_person_weapon_interface_initialize.c's weapon-tag/hud-interface-tag chain.
// register convention: none recognized by Ghidra (every input is a global or a freshly-computed
// local). // blam-cc: none
// UNSURE: render_model (module render) is modeled from the call sites only: the model tag
// reference in EAX, the permuted node records in ECX and 11 stack arguments. The stack block
// built here is first_person_light_parameters in types/interface.h.
// Fixed from objdump this pass: the node permute and render calls take the weapon tag +0x468
// reference (first person model) for the weapon case and first_person_interface +0x0c (the
// hands model) for the device case; the earlier rewrite passed weapon tag +0x478 to both.
// reconciled: R44 first_person_light_parameters is the first 0x20 bytes of render.h render_model_effect: armed -> type, unknown_37c/380 -> unit_37c/380, unit_handle -> object_index, camera_x/y/z -> centroid[3], zero -> modifier_shader; 0x4d6fc0 reads 0x28 bytes, so change_colors/function_values come from the next 8 stack bytes

#include <stdint.h>
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

extern int16_t current_local_player_index; // 0x007c3108
extern first_person_weapon_interface *first_person_weapon_interfaces; // 0x006b2d98
extern player_globals *local_player_globals;   // 0x0087a478
extern data_array *player_data;                // 0x0087a480, "players"
extern data_array *object_data; // 0x008603b0, "objects"
extern tag_instance *tag_instances;            // 0x0087bc14
extern Globals *global_globals;                // 0x00746fa0
extern float camera_position_x, camera_position_y, camera_position_z; // 0x007c3114/18/1c

extern void hud_meter_permute_node_records(uint8_t *dest, uint8_t *source,
                                            uint32_t target_tag_ref, int16_t *lookup); // 0x493ea0
extern void *object_get_cached_render_lighting(datum_index object_index, real level_of_detail_pixels); // 0x50ea00, ESI object_index, stack level_of_detail_pixels
extern void render_model(uint32_t model_tag_ref, uint8_t *node_records,
                          int32_t unknown_0, int32_t unknown_1, ColorRGB *change_colors,
                          float *function_out_values, int32_t light_sample, float *camera_position,
                          int32_t unknown_6, first_person_light_parameters *light_params,
                          datum_index weapon_index, int32_t unknown_9, int32_t unknown_10); // 0x4d6fc0
    // blam-cc: EAX -> model_tag_ref, ECX -> node_records, 11 stack arguments (objdump 0x492636..0x49266a)

// blam-cc: none
// If the local player's first-person weapon is attached with a valid unit and weapon and that
// weapon's hud_interface tag is assigned: samples the cluster ambient light at the unit's
// position, builds a small "dynamic light" params block (armed only while the unit has flag
// 0x10 set or unknown_37c is positive, in which case it carries unknown_37c/380 and the camera
// position), then for each of the weapon HUD element and (if the globals have first-person hands
// assigned) the device HUD element, gathers the matching permuted node records and dispatches
// them to render_model along with the weapon's own change_colors/function_out_values (for the
// weapon-HUD case) or the unit's (for the device-HUD case).
void first_person_weapon_update_lighting(void)
{
    first_person_weapon_interface *fp;
    uint32_t player_handle;
    uint32_t unit_handle;
    player *player_record;
    object *unit_obj;
    object *weapon_obj;
    char *weapon_tag_data;
    char *hud_interface_tag_data;
    GlobalsFirstPersonInterface *first_person_interface;
    int32_t light_sample;
    uint8_t node_scratch[3332];

    first_person_light_parameters light_params; // types/interface.h, 0x20 bytes at esp+0x20

    if (current_local_player_index == -1) {
        return;
    }
    fp = &first_person_weapon_interfaces[current_local_player_index];

    if (current_local_player_index >= 1) {
        return;
    }
    player_handle = local_player_globals->local_players[current_local_player_index];
    if (player_handle == 0xffffffff) {
        return;
    }
    player_record = (player *)((char *)player_data->data + (uint16_t)player_handle * player_data->size);
    unit_handle = player_record->unit;
    if (unit_handle == 0xffffffff) {
        return;
    }
    if (fp->attached == 0 || fp->unit_index == (datum_index)0xffffffff) {
        return;
    }
    if (fp->weapon_index == (datum_index)0xffffffff) {
        return;
    }

    unit_obj = *(object **)((char *)object_data->data + 8 + (uint16_t)unit_handle * 0xc);
    weapon_obj = *(object **)((char *)object_data->data + 8 + (uint16_t)fp->weapon_index * 0xc);
    weapon_tag_data = (char *)tag_instances[(uint16_t)weapon_obj->definition_tag].data;
    if (*(int32_t *)(weapon_tag_data + 0x478) == -1) { // UNSURE: same offset as first_person_
                                                        // weapon_interface_initialize.c
        return;
    }

    first_person_interface = (GlobalsFirstPersonInterface *)global_globals->first_person_interface.pointer;
    // 0x49258d..0x492596: ESI = the player unit (0x49250c), stack FLT_MAX. The draft passed 0x7f7fffff as the
    // object index and dropped the level of detail.
    light_sample = (int32_t)(uintptr_t)object_get_cached_render_lighting((datum_index)unit_handle, 3.4028235e+38f);
    light_params.modifier_shader = 0;

    if ((*(uint8_t *)((char *)unit_obj + 0x204) & 0x10) != 0 ||
        *(float *)((char *)unit_obj + 0x37c) > 0.0f) {
        light_params.unit_37c = *(float *)((char *)unit_obj + 0x37c);
        light_params.unit_380 = *(float *)((char *)unit_obj + 0x380);
        light_params.type = 1;
        light_params.centroid[0] = camera_position_x;
        light_params.centroid[1] = camera_position_y;
        light_params.centroid[2] = camera_position_z;
        light_params.object_index = unit_handle;
    } else {
        light_params.type = 0;
    }

    if (fp->weapon_hud_valid != 0 && *(int32_t *)(weapon_tag_data + 0x468) != -1) {
        uint32_t model_tag_ref = *(uint32_t *)(weapon_tag_data + 0x468); // objdump 0x492612

        hud_meter_permute_node_records(node_scratch, fp->unknown_108c, model_tag_ref,
                                        fp->weapon_hud_element);
        render_model(model_tag_ref, node_scratch, 0, 0, (ColorRGB *)((char *)weapon_obj + 0x1b8),
                     (float *)((char *)weapon_obj + 0x134), light_sample, &camera_position_x, 0,
                     &light_params, fp->weapon_index, 0, 8);
    }
    if (fp->device_hud_valid != 0 &&
        *(int32_t *)((char *)first_person_interface + 0xc) != -1) {
        uint32_t model_tag_ref = *(uint32_t *)((char *)first_person_interface + 0xc); // objdump 0x492680

        hud_meter_permute_node_records(node_scratch, fp->unknown_108c, model_tag_ref,
                                        fp->device_hud_element);
        render_model(model_tag_ref, node_scratch, 0, 0, (ColorRGB *)((char *)unit_obj + 0x1b8),
                     (float *)((char *)unit_obj + 0x134), light_sample, &camera_position_x, 0,
                     &light_params, fp->weapon_index, 0, 8);
    }
}

#if 0
Original Ghidra decompilation (0x4924b0):

void FUN_004924b0(void)

{
  uint uVar1;
  int iVar2;
  uint *puVar3;
  int iVar4;
  int iVar5;
  undefined4 uVar6;
  char *pcVar7;
  undefined2 local_d2c [2];
  undefined4 local_d28;
  undefined4 local_d24;
  uint local_d20;
  undefined4 local_d1c;
  undefined4 local_d18;
  undefined4 local_d14;
  undefined4 local_d10;
  undefined1 local_d04 [3332];

  if (DAT_007c3108 != -1) {
    pcVar7 = (char *)(DAT_007c3108 * 0x1ea0 + DAT_006b2d98);
    if (((((DAT_007c3108 < 1) &&
          (uVar1 = *(uint *)(DAT_0087a478 + 4 + DAT_007c3108 * 4), uVar1 != 0xffffffff)) &&
         (uVar1 = *(uint *)((uVar1 & 0xffff) * 0x200 + 0x34 + *(int *)(DAT_0087a480 + 0x34)),
         uVar1 != 0xffffffff)) && ((*pcVar7 != '\0' && (*(int *)(pcVar7 + 4) != -1)))) &&
       (*(uint *)(pcVar7 + 8) != 0xffffffff)) {
      iVar2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar1 & 0xffff) * 0xc);
      puVar3 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (*(uint *)(pcVar7 + 8) & 0xffff) * 0xc
                         );
      iVar4 = *(int *)((*puVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
      if (*(int *)(iVar4 + 0x478) != -1) {
        iVar5 = *(int *)(DAT_00746fa0 + 0x180);
        uVar6 = render_get_cluster_ambient_light_sample(0x7f7fffff);
        local_d10 = 0;
        if (((*(byte *)(iVar2 + 0x204) & 0x10) != 0) || (0.0 < *(float *)(iVar2 + 0x37c))) {
          local_d28 = *(undefined4 *)(iVar2 + 0x37c);
          local_d24 = *(undefined4 *)(iVar2 + 0x380);
          local_d2c[0] = 1;
          local_d1c = DAT_007c3114;
          local_d18 = DAT_007c3118;
          local_d14 = DAT_007c311c;
          local_d20 = uVar1;
        }
        else {
          local_d2c[0] = 0;
        }
        if ((pcVar7[0x1d8c] != '\0') && (*(int *)(iVar4 + 0x468) != -1)) {
          FUN_00493ea0(local_d04,pcVar7 + 0x108c);
          FUN_004d6fc0(0,0,puVar3 + 0x6e,puVar3 + 0x4d,uVar6,&DAT_007c3114,0,local_d2c,
                       *(undefined4 *)(pcVar7 + 8),0,8);
        }
        if ((pcVar7[0x1e0e] != '\0') && (*(int *)(iVar5 + 0xc) != -1)) {
          FUN_00493ea0(local_d04,pcVar7 + 0x108c);
          FUN_004d6fc0(0,0,iVar2 + 0x1b8,iVar2 + 0x134,uVar6,&DAT_007c3114,0,local_d2c,
                       *(undefined4 *)(pcVar7 + 8),0,8);
        }
      }
    }
  }
  return;
}
#endif
