// unit_dispatch_reaction_animation  (Ghidra: FUN_005614a0)
// address 0x5614a0, size 355 bytes
// name confidence: 0.3   rewrite confidence: 0.85
// REWRITTEN from objdump 0x5614a0..0x561602. Ghidra split the function at its jump table (0x561604) and the draft
//   called the table entries, which are labels inside this function. ESI = unit, stack: reaction code 0..5, which
//   picks a scream / vocalization index (0: 0xa; 1: 0x27 or 0xb at random; 2: 0xb; 3: 0xc; 4: 0xd; 5: 0xb7).
//   With a dialogue tag (+0x384) whose entry for it (+0x1c + index * 16) has a sound, the speech priority check
//   (0x560d00: EAX unit, DL 1, stack priority 9, 0, 0, &index, &sound) decides; a positive result commits a
//   speech {priority 9, index, sound, tail 7 ticks, the rest -1} (0x560f20: EAX unit, ECX speech, DX result).
//   Returns whether one was committed.
// blam-cc: ESI -> unit_index, stack -> reaction_code

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include <string.h>

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern uint32_t random_seed_global; // 0x00719cd0

extern int32_t unit_animation_change_priority_check(uint32_t unit_index, uint8_t follow_fallback,
    int16_t requested_priority, uint8_t allow_repeat, uint32_t *out_unknown_3f0, int16_t *dialogue_index,
    int32_t *chain_value); // 0x560d00, EAX, DL, stack
extern int32_t unit_commit_speech(uint32_t unit_index, const unit_speech *source, int16_t mode); // 0x560f20, EAX, ECX, DX

uint8_t unit_dispatch_reaction_animation(int32_t unit_index, int16_t reaction_code)
{
    uint8_t *obj = (uint8_t *)((object_header *)object_data->data)[unit_index & 0xffff].data;
    int16_t index;
    datum_index dialogue = ((unit_object *)obj)->unit.dialogue_tag_index;
    int32_t sound;
    int32_t result;
    unit_speech speech;

    switch (reaction_code) {
    case 0:
        index = 0xa;
        break;
    case 1:
        random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
        index = ((float)(random_seed_global >> 16) * 1.5259022e-05f < 0.5f) ? 0x27 : 0xb;
        break;
    case 2:
        index = 0xb;
        break;
    case 3:
        index = 0xc;
        break;
    case 4:
        index = 0xd;
        break;
    case 5:
        index = 0xb7;
        break;
    default:
        return 0; // 0x5614c2: the table has six entries
    }
    if (dialogue == k_datum_index_none) {
        return 0;
    }
    sound = *(int32_t *)((uint8_t *)tag_instances[dialogue & 0xffff].data + index * 16 + 0x1c);
    if (sound == -1) {
        return 0;
    }
    result = unit_animation_change_priority_check((uint32_t)unit_index, 1, 9, 0, 0, &index, &sound);
    if ((int16_t)result <= 0) {
        return 0;
    }
    memset(&speech, 0, sizeof(speech));
    speech.scream_type = index;
    speech.sound_tag = (datum_index)sound;
    speech.priority = 9;
    speech.tail_ticks = 7;
    speech.unknown_10 = -1;
    speech.unknown_14 = -1;
    speech.ai_line_index = -1;
    speech.unknown_18 = -1;
    unit_commit_speech((uint32_t)unit_index, &speech, (int16_t)result);
    return 1;
}

#if 0
Original Ghidra decompilation (0x5614a0):

void FUN_005614a0(short param_1)

{
                    /* WARNING: Could not recover jumptable at 0x005614c2. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  (*(code *)(&PTR_LAB_00561604)[param_1])();
  return;
}
#endif
