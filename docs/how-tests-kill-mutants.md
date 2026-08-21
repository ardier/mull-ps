# How a test program links against a mutant and kills it

This walks through the actual mechanism — not the theory — by which an app's test program comes
to run *mutated* Halide-generated code and detects the difference. Everything below was directly
verified end-to-end this session (not just argued from the design) on both a simple generator
(`blur`) and a multi-function one (`camera_pipe`); see the plan file's "final-binary verification"
and "multi-class sanity check" entries for the concrete commands and diffs this document is based on.

## The four artifacts involved

For one mutant, four things get built, each derived from the one before it:

1. **The instrumented generator binary** (`blur.generator`) — the app's `*_generator.cpp`, compiled
   once with Mull's IR-mutation pass plugin (`-fpass-plugin=.../mull-ir-frontend-14`). This single
   binary contains *every* mutant Mull found, each one guarded by a runtime check
   (`if (getenv("<mutator-id>:<file>:<line>:<col>")) { /* mutated path */ } else { /* original */ }`)
   compiled directly into the generator's logic. No mutant is "selected" yet at this stage — they're
   all present, all inert by default.

2. **A mutant-specific static library** (`out-mut/halide_blur.a`) — produced by *running* the
   instrumented generator binary with one specific mutant's environment variable set to `1`:
   ```sh
   env "Halide_add_to_div:/path/to/halide_blur_generator.cpp:39:37"=1 \
     ./blur.generator -g halide_blur -e static_library,h,stmt -o out-mut target=host
   ```
   The generator binary reads that env var at the point the guarded code executes, takes the mutated
   branch *for that one call site only*, and lowers the resulting (mutated) Halide pipeline all the
   way down to a real, compiled, architecture-native static library — exactly the same artifact a
   developer would ship, just built from a one-line-different computation graph. Running the same
   binary with no env var set (or a different mutant's) produces the unmutated baseline library
   instead. This is the only place the mutant "exists" as a choice — everything downstream just
   compiles whatever `.a` it's handed, with no awareness that a mutation happened at all.

3. **The final test binary** (`test-mut`) — the app's own, completely unmodified test/demo program
   (`apps/blur/test.cpp`, `apps/camera_pipe/process.cpp`, ...) compiled and linked directly against
   that mutant-specific `.a`:
   ```sh
   clang++ -std=c++17 -O2 -I out-mut -I tools -I build/include \
     apps/blur/test.cpp out-mut/halide_blur.a -o test-mut -lpthread -ldl
   ```
   The test program's source is never touched or aware a mutation exists — from its point of view it
   is simply calling `halide_blur(...)`, a function declared in the header pulled from `out-mut/`,
   whose *implementation* happens to be the mutated one because that's what got linked in.

4. **Execution** — running `./test-mut` actually exercises the mutated computation. What happens next
   is the "oracle":
   - **O1 (the app's own shipped test)**: the test program computes the result itself, or via a
     reference implementation, and asserts/aborts on mismatch. If the assertion fires, the process
     exits non-zero (or receives `SIGABRT`) — that's a kill. If the mutated computation happens to
     still satisfy every assertion the test checks, the process exits 0 — the mutant *survives* this
     oracle, even though its output is provably different (see below).
   - **O2 (golden-output diff)**: independently of whatever the test program itself checks, the
     driving script runs the mutant binary, captures whatever output artifact it produces (an image,
     a buffer dump, etc.), and byte-compares it against a golden snapshot captured from the unmutated
     baseline run. A mismatch here is a kill, *even if the app's own test passed* — this is precisely
     how this sprint found that e.g. `bilateral_grid`'s BoundaryConditions mutants pass their own test
     100% of the time but are caught 100% of the time by this external check, since the shipped test
     asserts nothing about pixel values at all.

## Why this reliably tracks *one* mutant through every stage

The mutant's identity is never stored in a side-channel that could get lost or confused between the
four artifacts above — it is simply *which environment variable was set* when the generator binary was
invoked in step 2. That choice is baked into the compiled `.a` as ordinary generated code (there is no
mutation-related metadata left in it at all), so steps 3 and 4 have nothing to desynchronize: they are
just building and running whatever code is actually inside that specific `.a`. This is what was
confirmed directly this session by disassembling the actual `test-mut` binary and diffing it against
`test-baseline` — the final, executed machine code differs in exactly the way the mutation predicts, at
every one of the intermediate stages checked (`.stmt` text, the static library's own disassembly, and
the final linked binary's disassembly), including on `camera_pipe`, a generator with real multi-function
internal structure, not just the simplest single-expression case.

## Worked example (from this session's `blur` verification)

```
$ diff out/halide_blur.stmt out-mut/halide_blur.stmt
...
<  ... = (input[t+1] + (input[t+2] + input[t]))/3     # baseline: sum of 3 neighbors, /3
>  ... = (input[t+2] + (input[t]/input[t+1]))/3       # mutant:   one '+' became '/'

$ md5sum out/halide_blur.a out-mut/halide_blur.a
11877ebd...  out/halide_blur.a
4839c3fe...  out-mut/halide_blur.a            # different static library, confirmed by hash

$ objdump -d test-baseline | wc -l ; objdump -d test-mut | wc -l
15631
16038                                          # different final binary, confirmed by disassembly size

$ ./test-mut
Success!                                       # for THIS mutant, the shipped test still passed (O1 survives)
                                                # — but O2 (golden-output diff) would still catch it,
                                                # since the actual output pixels differ.
```

## AST-route mutants work identically, with one difference worth noting

For mutants produced via the Clang-AST route (`mull-cxx-frontend`, e.g. BoundaryConditions, select/
clamp swaps) rather than the IR route, steps 1-2 use `-fplugin=.../libmull-cxx-frontend-14.so` instead
of `-fpass-plugin=`, but the env-var-selection mechanism and everything from step 2 onward (producing
a real `.a`, linking the unmodified test program against it, running it) is exactly the same. The
standalone Clang-LibTooling prototype explored separately (`ardi-experiments@halide-src-rewrite`)
changes step 1-2 more fundamentally — instead of one instrumented binary carrying every mutant behind
runtime dispatch, it emits N separate, already-mutated `*.cpp` files up front, each compiled through
the **completely stock**, unmodified Halide build (no plugin, no env var at all) to produce its own
`.a` directly. From step 3 onward the mechanism is identical either way: an unmodified test program
gets linked against whatever `.a` it's handed and run.
