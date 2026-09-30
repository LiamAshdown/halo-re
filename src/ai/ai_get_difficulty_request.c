// ai_get_difficulty_request  (Ghidra: ai_get_difficulty_request, renamed)
// address 0x42a950, size 107 bytes
// name confidence: 0.4   rewrite confidence: 0.45
// evidence: phase-4 summary "Returns a difficulty-tuning scalar or a boolean pair selected by
// a small enumerated request code, backed by weapon_get_zoom_fov's difficulty table." weapon_get_zoom_fov
// is already established elsewhere in this module (src/ai/actor_evaluate_combat_state_transition.c):
// float weapon_get_zoom_fov(int32_t selector).
// register convention: AX -> request_code, ECX -> out_flag_a, EDX -> out_flag_b (only used
//   for codes 3/4), ESI -> out_value (only used for codes 1/2/default).
//   // blam-cc: EAX (low 16 bits) -> request_code, ECX -> out_flag_a, EDX -> out_flag_b,
//   ESI -> out_value

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "game.h"
#include "fn_ai.h"
#include "fn_game.h"


    // 0x46fe10, blam-cc: stack -> zoom_table_index, CX -> magnification (every caller passes the difficulty)
extern game_main_globals *main_game_globals; // 0x006b0b80

// blam-cc: EAX -> request_code, ECX -> out_flag_a, EDX -> out_flag_b, ESI -> out_value
void ai_get_difficulty_request(int16_t request_code, uint8_t *out_flag_a, uint8_t *out_flag_b, float *out_value)
{
    switch (request_code) {
    case 1:
        *out_flag_a = 1;
        *out_value = weapon_get_zoom_fov(0x1d, main_game_globals->difficulty);
        break;
    case 2:
        *out_flag_a = 1;
        *out_value = weapon_get_zoom_fov(0x1e, main_game_globals->difficulty);
        break;
    case 3:
        *out_flag_a = 0;
        *out_flag_b = 0;
        break;
    case 4:
        *out_flag_a = 0;
        *out_flag_b = 1;
        break;
    default:
        *out_flag_a = 1;
        *out_value = weapon_get_zoom_fov(0x1c, main_game_globals->difficulty);
        break;
    }
}

#if 0
Original Ghidra decompilation (0x42a950):

void FUN_0042a950(void)

{
  undefined2 in_AX;
  undefined1 *in_ECX;
  undefined1 *in_EDX;
  float *unaff_ESI;
  float10 fVar1;

  switch(in_AX) {
  case 1:
    *in_ECX = 1;
    fVar1 = (float10)FUN_0046fe10(0x1d);
    *unaff_ESI = (float)fVar1;
    return;
  case 2:
    *in_ECX = 1;
    fVar1 = (float10)FUN_0046fe10(0x1e);
    *unaff_ESI = (float)fVar1;
    return;
  case 3:
    *in_ECX = 0;
    *in_EDX = 0;
    return;
  case 4:
    *in_ECX = 0;
    *in_EDX = 1;
    return;
  default:
    *in_ECX = 1;
    fVar1 = (float10)FUN_0046fe10(0x1c);
    *unaff_ESI = (float)fVar1;
    return;
  }
}
#endif
