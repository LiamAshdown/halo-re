// game_engine_koth_player_in_hill_bounds  (Ghidra: game_engine_koth_player_in_hill_bounds,
//   already named)
// address 0x46aa60, size 157 bytes
// name confidence: 0.55   rewrite confidence: 0.6
// evidence: types/game.h player::unit (0x34), player_data (0x0087a480); types/objects.h
//   object.bounding_center (0xa0); king_hill_boundary_min_z/_max_z/king_starting_location_count
//   (this batch, 0x006b1060/0x006b105c/0x006b0f50).
// register convention: player index in in_EAX.
//   // blam-cc: EAX -> player_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "game.h"
#include "fn_game.h"

extern data_array *player_data;    // 0x0087a480
extern data_array *object_data; // 0x008603b0
extern float king_hill_boundary_min_z; // 0x006b1060
extern float king_hill_boundary_max_z; // 0x006b105c
extern int32_t king_starting_location_count;       // 0x006b0f50

extern uint8_t polygon2d_point_inside_margin(int16_t vertex_count, Point2D *point, int32_t margin); // 0x4cae60

// blam-cc: EAX -> player_index
// True when the player's unit's bounding-center Z falls within the hill's vertical bounds and
// its (x,y) lies inside the hill boundary polygon.
uint8_t game_engine_koth_player_in_hill_bounds(uint32_t player_index)
{
    player *p;
    datum_index unit;
    object *unit_obj;
    float z;

    if (player_index == 0xffffffff) {
        return 0;
    }
    p = (player *)((uint8_t *)player_data->data + (player_index & 0xffff) * sizeof(player));
    unit = p->unit;
    if (unit == (datum_index)0xffffffff) {
        return 0;
    }
    unit_obj = ((object_header *)object_data->data)[unit & 0xffff].data;
    z = unit_obj->bounding_center.z;
    if (z >= king_hill_boundary_min_z && z < king_hill_boundary_max_z) {
        Point2D point;
        point.x = unit_obj->bounding_center.x;
        point.y = unit_obj->bounding_center.y;
        return polygon2d_point_inside_margin((int16_t)king_starting_location_count, &point, 0);
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x46aa60), from tools/pack.py 0x46aa60:

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 game_engine_koth_player_in_hill_bounds(void)

{
  uint uVar1;
  int iVar2;
  uint in_EAX;
  undefined4 uVar3;
  undefined4 local_8;
  undefined4 local_4;

  if (in_EAX != 0xffffffff) {
    uVar1 = *(uint *)((in_EAX & 0xffff) * 0x200 + *(int *)(DAT_0087a480 + 0x34) + 0x34);
    if (uVar1 != 0xffffffff) {
      iVar2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar1 & 0xffff) * 0xc);
      if (_DAT_006b1060 <= *(float *)(iVar2 + 0xa8)) {
        if (*(float *)(iVar2 + 0xa8) < _DAT_006b105c != (*(float *)(iVar2 + 0xa8) == _DAT_006b105c))
        {
          local_8 = *(undefined4 *)(iVar2 + 0xa0);
          local_4 = *(undefined4 *)(iVar2 + 0xa4);
          uVar3 = polygon2d_point_inside_margin((undefined2)DAT_006b0f50,&local_8,0);
          return uVar3;
        }
      }
    }
  }
  return 0;
}
#endif
