# Pcode Weaver Experiments

This directory keeps the small binaries used to demonstrate Pcode Weaver
rewrite rules.  Each target has its original C source for reference and the
compiled ELF that should be imported into Ghidra for the experiment.

## Layout

```text
experiment/
  targets/
    01_obfuscated/    obfuscated arithmetic expression
    02_obfuscated2/   call/id and arithmetic rewrite demo
    03_memset/        loop-to-usercall memset-like rewrite demo
    04_bitmap/        bit operation rewrite demo
    05_logerror/      negative example for current DSL limits
  rules/
    01_obfuscated/    rules for targets/01_obfuscated
    02_obfuscated2/   rules for targets/02_obfuscated2
    03_memset/        rules for targets/03_memset
    04_bitmap/        rules for targets/04_bitmap
    05_logerror/      intentionally has no rewrite rules
```

The numbered directories are aligned: `targets/03_memset` is paired with
`rules/03_memset`, and so on.  The fifth target is included as an experiment
case that the current DSL cannot reasonably handle, so it has only a note
instead of rule files.

## Experiment Environment

The experiments were checked with
[Ghidra 12.0](https://github.com/NationalSecurityAgency/ghidra/releases/tag/Ghidra_12.0_build).

### Target Compiler

The target binaries were compiled from the included C sources with the system
GCC:

```text
gcc (Ubuntu 11.4.0-1ubuntu1~22.04.3) 11.4.0
```

You do not need to rebuild these C sources to use the experiments.  The compiled
ELF files are the real experiment inputs; sources and compiler details are kept
only as reference information.

## Using the Experiments

Build or obtain `compiler.elf` first.  From the repository root, compile a
target's rule set into a temporary output directory:

```sh
mkdir -p tmp/compiled-rules
src/compiler/build/bin/compiler.elf -d tmp/compiled-rules experiment/rules/01_obfuscated/*
```

Then point the plugin at those compiled rules before starting ReOxide/Ghidra:

```sh
export PCODE_WEAVER_RULE_DIR="$PWD/tmp/compiled-rules"
```

Import the matching ELF from `experiment/targets/<target>/` into Ghidra and
decompile the relevant function.  The `.c` file beside each ELF is reference
material only; the ELF is the main experiment target.

## About User-Defined Ops

Some rules replace matched p-code with `USERDEFINED` operations.  These
operations are only readable markers for the experiment output, not recovered C
function calls with the same names.  For example, a bitmap rule can shrink a
macro expansion into a `set`-like `USERDEFINED` op so the decompiled result is
easier to inspect.

## Target Notes

- `01_obfuscated`: small example from the RULECOMPILE article:
  https://msm.lt/re/ghidra/rulecompile/.
- `02_obfuscated2`: harder version of the first binary, with all expression
  reorder variants listed explicitly.
- `03_memset`: example with optimized `memset`-like code.
- `04_bitmap`: example of shrinking bitmap macros.
- `05_logerror`: negative example.  There are no rules because this case is too
  hard to express with the current DSL.
