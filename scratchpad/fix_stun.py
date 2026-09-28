import os
os.chdir(r'C:\Users\Liam-\halo-re')
p = 'src/ai/ai_object_list_initialize_shield_stun_thresholds.c'
lines = open(p, encoding='utf-8').read().split('\n')
assert lines[41].strip() == 'float *override_max_body_vitality, float *override_max_shield_vitality)', lines[41]
lines[41] = '    float override_max_body_vitality, float override_max_shield_vitality)'
assert 'override_max_body_vitality, override_max_shield_vitality);' in lines[64]
lines[64] = lines[64].replace('override_max_body_vitality, override_max_shield_vitality);',
                              '&override_max_body_vitality, &override_max_shield_vitality);')
lines.insert(19, '// FIXED 2026-09-28 (retail-independence loop): the two overrides are float VALUES on the stack, not pointers --\n'
                 '//   objdump 0x561b05..0x561b3c copies them into two locals and passes their addresses (ESI body, EDI\n'
                 '//   shield) to object_initialize_shield_stun_thresholds; the only caller, hs units_set_maximum_vitality\n'
                 '//   (0x47c060), pushes the evaluated real arguments.')
open(p, 'w', encoding='utf-8').write('\n'.join(lines))
print('patched')
