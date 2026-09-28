// Decompiles the functions at the given addresses, creating a function first where Ghidra never made one (the
// functions reached only through data tables). Run read-only so the project is not modified.
// Usage (headless): -postScript DecompileAt.java <address-list-file> <out-file>
//   address-list-file: one hex address per line ('#' comments allowed)
//@category Halo
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.app.cmd.function.CreateFunctionCmd;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.*;
import java.io.*;
import java.nio.file.*;
import java.util.*;

public class DecompileAt extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        List<String> lines = Files.readAllLines(Paths.get(args[0]));
        DecompInterface ifc = new DecompInterface();
        ifc.setOptions(new DecompileOptions());
        ifc.openProgram(currentProgram);
        FunctionManager fm = currentProgram.getFunctionManager();
        int done = 0, failed = 0;
        try (PrintWriter out = new PrintWriter(new FileWriter(args[1]))) {
            for (String line : lines) {
                line = line.trim();
                if (line.isEmpty() || line.startsWith("#")) continue;
                Address a = toAddr(Long.parseLong(line.replace("0x", ""), 16));
                Function f = fm.getFunctionAt(a);
                if (f == null) {
                    disassemble(a);
                    new CreateFunctionCmd(a).applyTo(currentProgram, monitor);
                    f = fm.getFunctionAt(a);
                }
                out.println("// ===== " + (f == null ? "NOFUNC" : f.getName()) + " @ " + a + " =====");
                if (f == null) { failed++; out.println("// could not create a function"); continue; }
                out.println("// body " + f.getBody().getMinAddress() + ".." + f.getBody().getMaxAddress() +
                    " (" + f.getBody().getNumAddresses() + " bytes)");
                DecompileResults r = ifc.decompileFunction(f, 120, monitor);
                if (r != null && r.decompileCompleted() && r.getDecompiledFunction() != null) {
                    out.println(r.getDecompiledFunction().getC());
                    done++;
                } else {
                    failed++;
                    out.println("// DECOMPILE FAILED: " + (r == null ? "null" : r.getErrorMessage()));
                }
            }
        }
        ifc.dispose();
        println("DecompileAt: " + done + " decompiled, " + failed + " failed");
    }
}
