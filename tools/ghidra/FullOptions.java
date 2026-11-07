// Co-op+ research (2026-11-07, user: "begin decomping the game using Ghidra"): pre-script for the FULL auto-analysis of
// the game exe. Keeps the analyzers that recover structure (RTTI classes and vtables, strings, references, switch
// tables, function starts) and turns off the ones that do not finish on a 327 MB exe in reasonable time (the
// decompiler-driven parameter ID re-decompiles every function; the aggressive finder makes junk functions inside the
// protected .xtls / .tls$ sections).
// @category CoopPlus
import ghidra.app.script.GhidraScript;
import java.util.Map;

public class FullOptions extends GhidraScript {
    @Override
    public void run() throws Exception {
        Map<String, String> opts = getCurrentAnalysisOptionsAndValues(currentProgram);
        String[][] set = {
            {"Decompiler Parameter ID", "false"},
            {"Aggressive Instruction Finder", "false"},
            {"Condense Filler Bytes", "false"},
            {"Windows x86 PE RTTI Analyzer", "true"},
            {"Windows x86 PE Exception Handling", "true"},
            {"Demangler Microsoft", "true"},
            {"Function Start Search", "true"},
            {"ASCII Strings", "true"},
            {"Decompiler Switch Analysis", "true"},
        };
        for (String[] kv : set) {
            if (opts.containsKey(kv[0])) {
                setAnalysisOption(currentProgram, kv[0], kv[1]);
                println("option " + kv[0] + " = " + kv[1]);
            } else {
                println("option not found: " + kv[0]);
            }
        }
    }
}
