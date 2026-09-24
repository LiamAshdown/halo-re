// Dumps every function's decompiled C plus a function index CSV.
// Usage (headless): -postScript ExportDecompiledC.java <outdir>
//@category Halo
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.listing.*;
import java.io.*;

public class ExportDecompiledC extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        File outDir = new File(args.length > 0 ? args[0] : ".");
        outDir.mkdirs();
        DecompInterface ifc = new DecompInterface();
        DecompileOptions opts = new DecompileOptions();
        ifc.setOptions(opts);
        ifc.openProgram(currentProgram);
        FunctionManager fm = currentProgram.getFunctionManager();
        int total = fm.getFunctionCount(), n = 0, failed = 0;
        try (PrintWriter c = new PrintWriter(new FileWriter(new File(outDir, "halo_decompiled.c")));
             PrintWriter csv = new PrintWriter(new FileWriter(new File(outDir, "functions.csv")))) {
            csv.println("address,name,size,calls_out,is_thunk");
            for (Function f : fm.getFunctions(true)) {
                if (monitor.isCancelled()) break;
                n++;
                if (n % 500 == 0) monitor.setMessage("decompiling " + n + "/" + total);
                csv.printf("%s,%s,%d,%d,%b%n", f.getEntryPoint(), f.getName(),
                    f.getBody().getNumAddresses(), f.getCalledFunctions(monitor).size(), f.isThunk());
                if (f.isThunk() || f.isExternal()) continue;
                DecompileResults r = ifc.decompileFunction(f, 60, monitor);
                c.println("// ===== " + f.getName() + " @ " + f.getEntryPoint() + " =====");
                if (r != null && r.decompileCompleted() && r.getDecompiledFunction() != null) {
                    c.println(r.getDecompiledFunction().getC());
                } else {
                    failed++;
                    c.println("// DECOMPILE FAILED: " + (r == null ? "null" : r.getErrorMessage()));
                }
            }
        }
        ifc.dispose();
        println("Exported " + n + " functions (" + failed + " failed) to " + outDir);
    }
}
