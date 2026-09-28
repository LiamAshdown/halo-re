exec(open(r'C:\Users\Liam-\halo-re\scratchpad\hsgen_lib.py').read())

R0 = 'hs_thread_return(0, thread_index);\n'
emit(0x47b9a0, 86, 'sets bit 6 of the unit\'s byte +0x106 (die silently; no check for none); returns 0.', X['object_data'],
     UNIT_FLAGS + '\nunit[0x106] |= 0x40;\n' + R0)
emit(0x47bf50, 66, 'unit_scripting_set_emotion_animation (0x569cf0) with the unit and the name; returns 0.',
     'extern void unit_scripting_set_emotion_animation(uint32_t unit_index, const char *emotion_name); // 0x569cf0, blam-cc: EAX, ECX\n',
     'unit_scripting_set_emotion_animation((uint32_t)arguments[0], (const char *)arguments[1]);\n' + R0)
emit(0x47c340, 81, 'returns the driver (+0x324) of the unit\'s object (object_try_and_get mask 3), -1 when it is not a unit.',
     'extern void *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, blam-cc: ECX, stack\n',
     'uint8_t *unit = (uint8_t *)object_try_and_get((datum_index)arguments[0], 3);\n\n'
     'hs_thread_return(unit != 0 ? *(int32_t *)(unit + 0x324) : -1, thread_index);\n')
print('ok')
