# Ghidra scripts for the full decomp of the 3576 exe

Copies of the scripts the full decomp ran with (the working copies live in the git-ignored `build/tools/scripts/`,
the driver in `build/tools/deep_analysis.ps1`, which expects that layout). Ghidra 12.2, JDK 25, 24 GB heap.

| Script | What it does |
|---|---|
| `AnalyzeSliced.java` | Auto-analysis in 4 MB slices, each its own transaction, so one huge change set does not run out of memory. Progress in `build/decomp/sliced-progress.txt`; a rerun goes on from the next slice. |
| `RttiFix.java` | Renames this exe's data blocks to the roles Ghidra's RTTI analyzer looks for (`.rodata` -> `.data`, `.sdata` -> `.data_s`, `.arch` -> `.rdata`), removes "RTTI Found", reruns the RTTI analyzer. |
| `ExportIndex.java` | `functions.tsv`, `calls.tsv`, `strings.tsv`, `classes.tsv`. |
| `ExportAllC.java` | Every function as C, in shards, on several threads. |
| `Decomp.java` | Chosen RVAs only (the small project). |
| `DeepPass.java`, `FullOptions.java`, `StripProtected.java` | Earlier steps (deep analysis options, leaving out the protected `.tls$` code). |

Class recovery uses Ghidra's own `RecoverClassesFromRTTIScript.java`. Notes: PROGRESS.md (2026-11-07) and
docs/research/reverse-engineering-3576.md.
