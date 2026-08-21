# Corpus-wide mutant-population census (`dumpOnly`)

This directory is the output of running Mull's `dumpMutantsTo` / `dumpOnly` /
`regions.autodetect` feature (commits `c741d3a8`, `c80bf3ee`, `5eb72c8d` on
`mull-ps@wip`; see `../docs/mutant-population-dump.md` for the mechanism) over
all 16 emitted-C++ Halide apps in the corpus, one app at a time.

**Vocabulary.** This census reaches *point identified* only:

    point identified -> mutant generated -> mutant run -> mutant killed

Nothing recorded here was generated, run, or killed. `points_identified` in
`census.tsv` is the count of IR-level mutation points Mull's junk detector
classified before the clone phase; `kept_*` is the subset that survived every
mutation filter (what would have been cloned into mutants if the run had not
stopped at `dumpOnly`); `filtered_*` is everything the filters rejected. Do not
sum kept and filtered into a rate — filtered points never became mutants and
have no kill outcome.

## Why this exists

The IR route clones the enclosing function once per surviving mutation point
and retains every clone, so the clone phase costs
`sum over functions of (points in the function x function size)`. On the two
largest emitted files that exhausts memory before finishing (`bgu` above
60 GB, `lens_blur` around 84 GB), so neither has ever produced a reportable
mutant population before this. `dumpOnly` writes the population right after
filtering and returns before cloning starts, which is why it is cheap: single
digit to low tens of seconds, under 500 MB peak RSS, for every app in the
corpus including those two.

## Source trees, verified per app

Two source trees supply the emitted C++, and both are used in this corpus:

- **`bgu`, `camera_pipe`**: `HM-armc-fix` — `HM-armc`'s copies of these two do
  not compile as plain C++ (`bgu`: 32 errors from a `fast_inverse_f32` call
  resolving against the wrong overload; `camera_pipe`: 1 error from a
  `.prefetch()` call's `void` result assigned to `uint16_t`). `HM-armc-fix`
  carries a minimal, behavior-preserving patch for each, reproduced and
  verified in this run with a bare `clang++ -std=c++17 -fsyntax-only
  -Wno-psabi -ferror-limit=0` (no Mull involved): 32 -> 0 errors for `bgu`,
  1 -> 0 for `camera_pipe`.
- **The other 14 apps**: `HM-armc`, which compiles clean (0 errors) as-is.

Every app's `SOURCE.txt` records which tree it came from, the remote path on
`swsec01`, a sha256 of the local copy actually fed to `clang++`, and the
emitted line count. `bgu` and `lens_blur` were byte-diffed against a fresh
`scp` of their respective remote trees as an independent check (both
identical) before trusting their numbers as the self-check baseline below.

## Self-check: `bgu` and `lens_blur` against the expected baseline

These two were the first apps run (by an earlier session) and were reproduced
here as a self-check before running the other 14. All figures matched exactly:

| | expected | reproduced |
|---|---|---|
| bgu points identified | 51,737 | 51,737 |
| bgu distinct identifiers | 32,372 | 32,372 |
| bgu kept identifiers | 5,565 | 5,565 |
| bgu kept points | 5,579 | 5,579 |
| bgu filtered identifiers | 26,807 | 26,807 |
| bgu regions (kept, by identifier) | 32 / 5,532 / 1 | 32 / 5,532 / 1 |
| bgu multi-point kept identifiers | 14 | 14 |
| lens_blur points identified | 50,298 | 50,298 |
| lens_blur distinct identifiers | 32,494 | 32,494 |
| lens_blur kept identifiers | 6,308 | 6,308 |
| lens_blur kept points | 6,359 | 6,359 |
| lens_blur filtered identifiers | 26,186 | 26,186 |

No discrepancy was found, so the run proceeded to the other 14 apps.

## Files

- `census.tsv` — one row per app: source tree, emitted line count, points
  identified, distinct identifiers, kept/filtered identifiers and points,
  kept-by-region breakdown (`boilerplate` / `generator_specific` /
  `wrapper_metadata` / `unknown`), count of kept identifiers that carry more
  than one point, wall-clock seconds and peak RSS of the `dumpOnly` run, and
  the compiler's exit status.
- `operators.tsv` — one row per (app, mutator id): total/kept/filtered
  identifiers and points for that operator in that app.
- `multi_point_identifiers.tsv` — one row per kept identifier that carries
  more than one mutation point (a single source line inside a template,
  instantiated more than once), naming the region and the instantiated
  functions (`functions` column) that one env-var key would activate
  together.
- `<app>/kept.txt`, `<app>/filtered.txt` — the two identifier sets, one per
  line, `LC_ALL=C sort -u` order, as `MutationPoint::getUserIdentifier()`
  produces them (`<mutatorId>:<absoluteFilePath>:<line>:<column>`).
- `<app>/filtered-by.txt` — `identifier<TAB>rejecting filter name`.
- `<app>/mutants.tsv` — one row per identifier: region, points, kept_points,
  filtered_points, functions, filtered_by.
- `<app>/mull.yml`, `<app>/compile.txt`, `<app>/timing.txt` — the exact config
  used, Mull's own progress log, and `/usr/bin/time -v` output (wall time,
  peak RSS, exit status) for that app's run.
- `<app>/SOURCE.txt` — provenance: source tree, remote path, sha256 of the
  local copy, emitted line count, and the compile-check result.

## Multi-point identifiers: higher-order mutants in disguise

A source line inside a template that is instantiated more than once is
emitted by Halide's C backend once per instantiation, but Mull assigns it a
single identifier (same mutator, same file, same line, same column) because
the identifier is a source-location key, not an IR-location key. One env-var
mutant key therefore activates every IR mutation sharing that identifier at
once — on `bgu`, exactly the 14 identifiers documented in
`docs/mutant-population-dump.md`, all `halide_cpp_max`/`halide_cpp_min` or
`NativeVectorOps::max`/`min` template bodies instantiated once for `float`
and once for `int`. `multi_point_identifiers.tsv` lists every such identifier
across the corpus; the `functions` column names the instantiations involved.
Per-app counts (kept identifiers with more than one point):

| app | multi-point kept identifiers |
|---|---|
| bgu | 14 |
| bilateral_grid | 4 |
| blur | 0 |
| camera_pipe | 19 |
| conv_layer | 0 |
| depthwise_separable_conv | 0 |
| harris | 0 |
| hist | 4 |
| iir_blur | 0 |
| interpolate | 0 |
| lens_blur | 25 |
| local_laplacian | 20 |
| max_filter | 5 |
| nl_means | 10 |
| stencil_chain | 0 |
| unsharp | 0 |

## Reproducing a run

```sh
cat > mull.yml <<'EOF'
mutators:
  - cxx_default
timeout: 99999999
quiet: false
includePaths:
  - .*
parallelization:
  workers: 12
dumpMutantsTo: /path/to/out/<app>
dumpOnly: true
regions:
  autodetect: true
EOF

MULL_CONFIG=$PWD/mull.yml /usr/lib/llvm-14/bin/clang++ \
  -std=c++17 -O1 -g -grecord-command-line -Wno-psabi \
  -fpass-plugin=<mull-ps>/output/mull-ir-frontend-14 \
  -c <app>.halide_generated.cpp -o /dev/null
```

Confirm the `mull-ir-frontend-14` binary you invoke actually has the new
config keys before trusting a `dumpOnly` run — a stale binary silently
ignores them and falls through to the ordinary clone-everything path, which is
exactly the failure mode this feature exists to avoid:

```sh
strings output/mull-ir-frontend-14 | grep -E 'dumpMutantsTo|dumpOnly'
```

The `.o` file every `dumpOnly` run produces is a normal, uninstrumented
compile artefact (no `.mull_mutants` section — `strings <obj> | grep -c
'^cxx_'` returns 0). It is not committed here; it is not evidence of
anything having been instrumented, and it must not be linked expecting
mutants.

## Run conditions

All 16 runs were on the local 16-core box (not `swsec01`, which was shared
with ~115 other users and another agent's work during this run), sequential,
one app at a time. Load average (1-minute) at the start of the run was 0.2-0.3
and climbed to about 4-7 by the last few (heavier) apps as the runs
themselves consumed cores; see `timing.txt` per app for the actual wall time
each one took under that load. `local_laplacian` and `stencil_chain`, the two
next-largest files after `bgu`/`lens_blur`, took the longest (28.7 s / 26.0 s)
and used the most memory after them (492 MB / 324 MB) — consistent with the
cost model above (points x function size) and nowhere near a memory ceiling.
