// encounters_note_hostile_object  (Ghidra: encounters_note_hostile_object; named for this rewrite)
// address 0x435f90, size 306 bytes
// name confidence: 0.35   rewrite confidence: 0.55
// evidence: walks every live encounter and, for each one whose team is hostile to the given
//   object's team (types/game.h team_pair_globals.enemy_bits, the same "a * 10 + b" index
//   teams_are_enemies @src/game uses; in multiplayer the engine substitutes a plain
//   "different team" test), bumps encounter+0x4c. encounter+0x42 / 0x43 / 0x47 are the three
//   flags encounter_recompute_morale @0x437940 and encounter_choose_vocalizations @0x438580
//   maintain, so the counter is a hostile-contact tally gated on "the encounter has a target
//   but is not already fleeing or retreating".
// register convention: EAX -> object_index. Ghidra shows no other live argument; the
//   0x18-byte encounter_iterator in the frame is confirmed by the disassembly
//   (objdump -d -M intel --start-address=0x435f90 --stop-address=0x435ff0 bin/halo.exe).
//   // blam-cc: EAX -> object_index
//
// UNSURE: encounter.unknown_4c is only otherwise written (to 0) by
// encounter_release_stale_props and encounter_choose_vocalizations, and read by
// encounter_choose_vocalizations as a "how long since the last vocalization" gate, so
// "hostile contact tally" is the reading and not a proven name. The phase-4 summary reads
// this function as a zone-label mismatch count, which the code does not support.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "game.h"
#include "ai.h"
#include "fn_ai.h"

extern data_array *object_data;                  // 0x008603b0
extern ai_globals *ai_globals_ptr;               // 0x00880354
extern data_array *encounter_data;               // 0x008802c8
extern team_pair_globals *team_pair_data;        // 0x006b0b84
extern game_engine_definition *current_game_engine; // 0x006f1d20

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, blam-cc: EDI -> iterator

// blam-cc: EAX -> object_index
void encounters_note_hostile_object(datum_index object_index)
{
    object *obj;
    encounter_iterator iterator;
    encounter *enc;
    int16_t object_team;
    int16_t encounter_team;
    int32_t pair;
    char hostile;

    obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    // object + 0xb8 is the owning team. types/objects.h currently calls that offset
    // name_index; types/ai.h already records it as the team ("actor.team ... kept in sync
    // with object+0xb8 and encounter.team"), and the arithmetic below (team * 10 + team
    // into team_pair_globals.enemy_bits, both operands range-checked against 0..9) only
    // makes sense for a team. Accessed raw rather than resolving the cross-module conflict
    // here. See the open questions in src/ai/README.md.
    object_team = ((object *)obj)->owner_team;
    if (object_team == -1) {
        return;
    }

    if (ai_globals_ptr->actors_valid != 0) {
        iterator.data = encounter_data;
        iterator.next_index = 0;
        iterator.index = (datum_index)k_datum_index_none;
        iterator.signature = (uint32_t)encounter_data ^ 0x69746572;
        iterator.active_only = 1;
    }

    while (ai_globals_ptr->actors_valid != 0) {
        do {
            enc = (encounter *)data_iterator_next((data_iterator *)&iterator);
            if (enc == 0 || iterator.active_only == 0) {
                break;
            }
        } while (enc->units_active == 0);
        if (enc == 0) {
            return;
        }

        object_team = ((object *)obj)->owner_team;
        encounter_team = enc->team;
        if (current_game_engine == 0) {
            hostile = 0;
            if (0 <= encounter_team && encounter_team < 10 &&
                0 <= object_team && object_team < 10) {
                pair = (int32_t)object_team + encounter_team * 10;
                hostile = (char)(1 - ((1 << (pair & 0x1f)) &
                    team_pair_data->enemy_bits[pair >> 5]) != 0);
                if (hostile == 0) {
                    continue; // the tally below is skipped only when the pair is friendly
                }
            }
            // out-of-range team ids fall straight through to the tally, exactly as the
            // original does
        } else {
            hostile = (char)(encounter_team != object_team);
            if (hostile == 0) {
                continue;
            }
        }

        if (enc->any_actor_has_target != 0 && enc->no_recent_combat == 0 && enc->vocalizations_chosen == 0) {
            enc->hostile_notice_count = enc->hostile_notice_count + 1;
        }
    }
}

#if 0
Original Ghidra decompilation (0x435f90):

void FUN_00435f90(void)

{
  int iVar1;
  short sVar2;
  short sVar3;
  int iVar4;
  int iVar5;
  uint in_EAX;
  int iVar6;
  char cVar7;
  char local_4;

  iVar5 = DAT_006b0b84;
  iVar4 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  if (*(short *)(iVar4 + 0xb8) != -1) {
    if (*(char *)(DAT_00880354 + 1) != '\0') {
      local_4 = '\x01';
    }
    while (*(char *)(DAT_00880354 + 1) != '\0') {
      do {
        iVar6 = data_iterator_next();
        if ((iVar6 == 0) || (local_4 == '\0')) break;
      } while (*(char *)(iVar6 + 0xd) == '\0');
      if (iVar6 == 0) {
        return;
      }
      sVar2 = *(short *)(iVar4 + 0xb8);
      sVar3 = *(short *)(iVar6 + 2);
      if (DAT_006f1d20 == 0) {
        if ((((-1 < sVar3) && (sVar3 < 10)) && (-1 < sVar2)) && (sVar2 < 10)) {
          iVar1 = (int)sVar2 + sVar3 * 10;
          cVar7 = '\x01' - ((1 << ((byte)iVar1 & 0x1f) & *(uint *)(iVar5 + 0xa4 + (iVar1 >> 5) * 4))
                           != 0);
          goto LAB_00436088;
        }
        goto LAB_00436090;
      }
      cVar7 = sVar3 != sVar2;
LAB_00436088:
      if (cVar7 != '\0') {
LAB_00436090:
        if (((*(char *)(iVar6 + 0x43) != '\0') && (*(char *)(iVar6 + 0x42) == '\0')) &&
           (*(char *)(iVar6 + 0x47) == '\0')) {
          *(short *)(iVar6 + 0x4c) = *(short *)(iVar6 + 0x4c) + 1;
        }
      }
    }
  }
  return;
}
#endif
