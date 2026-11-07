# Co-op+ research (2026-11-07, user: "the most in depth that you can get"): after full_analysis.ps1 (the base pass),
# on the same database (build\tools\ghidra-full):
#   1. AnalyzeSliced.java deep  every analyzer on, incl. Decompiler Parameter ID and the Aggressive Instruction
#      Finder, in committed + saved 4 MB slices (one big transaction ran out of memory)
#   2. RecoverClassesFromRTTIScript.java (Ghidra's own): C++ classes from RTTI -- class structures, methods put in their
#      classes, inheritance
#   3. ExportIndex.java   functions / calls / classes / strings -> build\decomp
#   4. ExportAllC.java    the C of every function -> build\decomp\c
# Each step is its own headless run, so a failure leaves the earlier ones saved. Logs: build\decomp\deep-*.out.
$root = 'C:\Users\asher\CrimsonDesert-SeamlessCoop'
$env:JAVA_HOME = 'C:\Program Files\Eclipse Adoptium\jdk-25.0.5.8-hotspot'
$env:GHIDRA_HEADLESS_MAXMEM = '24G'
$env:GHIDRA_HEADLESS_JAVA_OPTIONS = '-XX:ParallelGCThreads=6'
$ghidra = "$root\build\tools\ghidra_12.2_PUBLIC\support\analyzeHeadless.bat"
$proj = "$root\build\tools\ghidra-full"
$out = "$root\build\decomp"
# Only our folder: Ghidra's own ghidra_scripts dirs (RecoverClassesFromRTTIScript) are on headless's default path, and
# a ";" list is split by analyzeHeadless.bat ("Bad argument", run of 2026-11-07 11:34).
$sp = "$root\build\tools\scripts"
function Step([string]$name, [string[]]$scriptArgs) {
    $t0 = Get-Date
    "== $name start $t0" | Add-Content "$out\deep.out"
    & $ghidra $proj cd3576full -process CrimsonDesert-1.0.0.3576.exe -noanalysis -scriptPath $sp @scriptArgs `
        -log "$out\deep-$name.log" -scriptlog "$out\deep-$name.script.log" -max-cpu 14 *> "$out\deep-$name.out"
    "== $name end $(Get-Date) after $([int]((Get-Date) - $t0).TotalMinutes) min, exit $LASTEXITCODE" | Add-Content "$out\deep.out"
}
$prog = "$out\sliced-progress.txt"
if (-not (Select-String -Path $prog -SimpleMatch 'SLICED DONE deep' -Quiet -ErrorAction SilentlyContinue)) {
    Step 'pass' @('-postScript', 'AnalyzeSliced.java', 'deep', $prog)
}
Step 'rtti' @('-postScript', 'RttiFix.java')   # the exe's .rodata/.sdata names hid the RTTI from Ghidra's analyzer
Step 'classes' @('-postScript', 'RecoverClassesFromRTTIScript.java')
Step 'index' @('-postScript', 'ExportIndex.java', $out)
Step 'c' @('-postScript', 'ExportAllC.java', $out, '10')
"== all done $(Get-Date)" | Add-Content "$out\deep.out"
