"""dump_group.py MODULE MAXSIZE [MINSIZE] -- disassemble every missing function of MODULE (per scratchpad/missing_functions.txt)
whose Ghidra body is in [MINSIZE, MAXSIZE) bytes, using the DecompileAt body bounds."""
import re, subprocess, sys, os
os.chdir(r'C:\Users\Liam-\halo-re')
module, maxsize = sys.argv[1], int(sys.argv[2])
minsize = int(sys.argv[3]) if len(sys.argv) > 3 else 0
EXE = r'C:\Program Files (x86)\Microsoft Games\Halo\halo.exe'
dec = open('scratchpad/missing_decomp.c', encoding='utf-8', errors='replace').read()
bodies = {int(m.group(1), 16): (int(m.group(2), 16), int(m.group(3)))
          for m in re.finditer(r'// ===== \S+ @ ([0-9a-f]{8}) =====\n// body [0-9a-f]+\.\.([0-9a-f]+) \((\d+) bytes\)', dec)}
wanted = sorted(int(l.split('\t')[0], 16) for l in open('scratchpad/missing_functions.txt')
                if l.startswith('0x') and l.split('\t')[1].strip() == module)
for a in wanted:
    end, size = bodies.get(a, (0, 99999))
    if not (minsize <= size < maxsize):
        continue
    out = subprocess.run(['objdump', '-d', '--no-show-raw-insn', '-M', 'intel', EXE,
                          '--start-address=0x%x' % a, '--stop-address=0x%x' % (end + 1)],
                         capture_output=True, text=True).stdout.splitlines()[7:]
    print('=== %x (%d bytes)' % (a, size))
    for line in out:
        if line.strip():
            print(line[:100])
