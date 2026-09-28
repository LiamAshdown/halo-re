import re, subprocess
R = "C:\\Users\\Liam-\\halo-re\\"
src = open(R + "scratchpad\\write_sites.py").read()
for f in re.findall(r"src/\S+\.c", src):
    t = open(R + f.replace("/", "\\"), encoding="utf-8").read(3000)
    m = re.search(r"address 0x([0-9a-f]+), size (\d+)", t)
    a, n = int(m.group(1), 16), int(m.group(2))
    out = subprocess.run(["objdump", "-d", "-M", "intel", "--no-show-raw-insn", "--start-address=0x%x" % a,
                          "--stop-address=0x%x" % (a + n), R + "bin\\halo.exe"], capture_output=True, text=True).stdout
    L = [l.split("\t", 1)[-1].strip() for l in out.split("\n") if re.match(r"\s*[0-9a-f]+:", l)]
    stores = [" ".join(l.split()) for l in L if re.match(r"mov\s+BYTE PTR \[esp\+0x[0-9a-f]+\],", l)]
    print(f.split("/")[-1][:-2], "|", " ; ".join(stores))
