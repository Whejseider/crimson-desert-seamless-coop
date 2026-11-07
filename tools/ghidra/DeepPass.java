// Co-op+ research (2026-11-07, user: "I want the most in depth that you can get"): the SECOND, deep analysis pass on
// the already analysed exe. Turns on every analyzer, including the slow ones the base pass skipped (Decompiler
// Parameter ID: real parameters and types for every function; Aggressive Instruction Finder: code not reached by
// references), with their most thorough options where Ghidra has them, then re-runs the whole analysis.
// @category CoopPlus
import ghidra.app.script.GhidraScript;
import java.util.Map;

public class DeepPass extends GhidraScript {
    @Override
    public void run() throws Exception {
        Map<String, String> opts = getCurrentAnalysisOptionsAndValues(currentProgram);
        int on = 0;
        // Every analyzer switch ("<name>" with a true/false value and no '.' sub-option) on.
        for (Map.Entry<String, String> e : opts.entrySet()) {
            String k = e.getKey(), v = e.getValue();
            if (k.contains(".")) continue;
            if ("false".equals(v)) {
                // Left off: PDB loading needs a .pdb we do not have; "Condense Filler Bytes" only changes the listing.
                if (k.startsWith("PDB") || k.contains("Condense Filler")) continue;
                setAnalysisOption(currentProgram, k, "true");
                println("on: " + k);
                on++;
            }
        }
        // Thorough sub-options where they exist.
        String[][] deep = {
            {"Decompiler Parameter ID.Commit Data Types", "true"},
            {"Decompiler Parameter ID.Commit Void Return Values", "true"},
            {"Decompiler Parameter ID.Analysis Decompiler Timeout (sec)", "120"},
            {"Decompiler Switch Analysis.Decompiler Timeout (sec)", "120"},
            {"Stack.Create Local Variables", "true"},
            {"Stack.Create Param Variables", "true"},
            {"Stack.useNewFunctionStackAnalysis", "true"},
            {"Data Reference.Create Address Tables", "true"},
            {"Data Reference.Respect Execute Flag", "true"},
            {"Windows x86 PE RTTI Analyzer.Create RTTI Classes", "true"},
            {"Function Start Search.Search Data Blocks", "true"},
        };
        for (String[] kv : deep) {
            if (opts.containsKey(kv[0])) {
                setAnalysisOption(currentProgram, kv[0], kv[1]);
                println("set: " + kv[0] + " = " + kv[1]);
            }
        }
        println("analyzers switched on: " + on + "; re-analysing everything (this is the long part)");
        long t0 = System.currentTimeMillis();
        analyzeAll(currentProgram);
        println("deep pass done in " + (System.currentTimeMillis() - t0) / 60000 + " min");
    }
}
