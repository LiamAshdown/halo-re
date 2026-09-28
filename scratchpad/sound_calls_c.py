import os, re
os.chdir(r'C:\Users\Liam-\halo-re')
for d in sorted(os.listdir('src')):
    if not os.path.isdir('src/' + d):
        continue
    for c in sorted(os.listdir('src/' + d)):
        if not c.endswith('.c'):
            continue
        p = 'src/%s/%s' % (d, c)
        s = open(p, encoding='utf-8', errors='replace').read()
        if 'game_engine_queue_multiplayer_sound' not in s:
            continue
        live = s.split('#if 0')[0]
        for n, l in enumerate(live.splitlines(), 1):
            if 'game_engine_queue_multiplayer_sound' in l and '//' not in l.split('game_engine_queue_multiplayer_sound')[0]:
                print('%s:%d: %s' % (p, n, l.strip()))
