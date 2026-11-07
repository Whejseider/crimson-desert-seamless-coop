// Co-op+ research: decompile chosen functions of the game exe, nothing else (no full auto-analysis: the exe is
// 327 MB). Arguments: an output file, then RVAs in hex (image base 0x140000000 added). For each: the function is made
// there if needed, decompiled, and its C goes to the output with the addresses it calls (to follow next).
// @category CoopPlus
import ghidra.app.script.GhidraScript;
import ghidra.app.cmd.function.CreateFunctionCmd;
import ghidra.app.cmd.disassemble.DisassembleCommand;
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.program.model.address.Address;
import ghidra.program.model.address.AddressSet;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.InstructionIterator;
import ghidra.program.model.symbol.Reference;
import java.io.FileWriter;
import java.io.PrintWriter;
import java.util.TreeSet;

public class Decomp extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        PrintWriter out = new PrintWriter(new FileWriter(args[0], true));
        DecompInterface di = new DecompInterface();
        di.openProgram(currentProgram);
        long base = 0x140000000L;
        for (int i = 1; i < args.length; i++) {
            long rva = Long.parseLong(args[i], 16);
            Address a = toAddr(base + rva);
            Function f = getFunctionAt(a);
            if (f == null) {
                new DisassembleCommand(a, null, true).applyTo(currentProgram, monitor);
                new CreateFunctionCmd(a).applyTo(currentProgram, monitor);
                f = getFunctionAt(a);
            }
            out.println("// ===== exe+0x" + Long.toHexString(rva).toUpperCase() + (f == null ? " (no function)" : " " + f.getName()));
            if (f == null) continue;
            TreeSet<String> calls = new TreeSet<>();
            InstructionIterator it = currentProgram.getListing().getInstructions(f.getBody(), true);
            while (it.hasNext()) {
                Instruction ins = it.next();
                if (!ins.getFlowType().isCall()) continue;
                for (Reference r : ins.getReferencesFrom()) {
                    long t = r.getToAddress().getOffset() - base;
                    if (t > 0 && t < 0x20000000L) calls.add("0x" + Long.toHexString(t).toUpperCase());
                }
            }
            DecompileResults res = di.decompileFunction(f, 120, monitor);
            out.println(res.decompileCompleted() ? res.getDecompiledFunction().getC() : "// decompile failed: " + res.getErrorMessage());
            out.println("// calls: " + String.join(" ", calls));
            out.println();
            out.flush();
        }
        out.close();
        di.dispose();
    }
}
