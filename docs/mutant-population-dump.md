# Dumping the mutant population without instrumenting

Mull's IR route clones the enclosing function once per surviving mutation point
and retains every clone, so the clone phase costs roughly

    sum over functions of (mutation points in the function x function size)

On the largest machine-generated translation units that product exhausts
memory: `bgu` dies above 60 GB inside `CloneBasicBlock`, `lens_blur` was killed
around 84 GB. Those files then report *no* mutant population at all.

But the population is fully known **before** cloning starts. Junk detection on
`bgu` classifies all 51,737 points in about 1.6 s; only the clone phase
afterwards is expensive. `dumpMutantsTo` writes the population at exactly that
boundary, and `dumpOnly` stops there.

## Configuration keys

All of these are **off by default**. With none of them present, Mull behaves
byte-for-byte as it did before they existed.

```yaml
# Path *prefix* (not a directory) for the dump. Absent = no dump.
dumpMutantsTo: /path/to/out/bgu

# Write the dump and return before the clone phase. Only meaningful together
# with dumpMutantsTo.
dumpOnly: true

# Region tagging. Absent = every record is tagged `unknown` and no source file
# is read.
regions:
  autodetect: true                # derive the boundary from the file itself
  # namespaceCloseMarker: "}  // namespace"     # override if needed
  # functionAttrsMarker: "HALIDE_FUNCTION_ATTRS"
  # boundaries:                   # or pin it by hand, per file, by regex
  #   - file: ".*halide_blur.*"
  #     boilerplateEnd: 3446
  #     generatorSpecificEnd: 4204
```

## What `dumpOnly` leaves behind

`dumpOnly` returns early from the mutation pass; it does **not** call `exit()`.
So:

* clang finishes normally and **exits 0**. A successful dump is not an error and
  callers do not have to special-case it.
* the dump files are flushed and checked before the pass returns, so a truncated
  file is reported as a warning rather than passed off as a complete population.
* an object file **is** produced, and it is a normal, *uninstrumented* one --
  the module is never mutated. It contains no `.mull_mutants` section, so
  `strings <obj> | grep -E '^cxx_[a-z_]+:'` comes back empty. Do not link it and
  expect mutants; use it only as a compile artefact, or discard it.

## Output files

Given `dumpMutantsTo: /out/bgu`:

| file | contents |
| --- | --- |
| `/out/bgu.kept.txt` | one identifier per line: what would have been cloned |
| `/out/bgu.filtered.txt` | one identifier per line: what the filters removed |
| `/out/bgu.filtered-by.txt` | `identifier<TAB>rejecting filter name` |
| `/out/bgu.mutants.tsv` | one row per identifier, with a header (below) |

The two `.txt` files are sets: deduplicated and sorted in byte order, so they do
not depend on how work was spread across parallel workers and can be diffed
directly against a `strings <obj> | sort -u` listing.

The identifier is `MutationPoint::getUserIdentifier()` verbatim:

    <mutatorId>:<absoluteFilePath>:<line>:<column>

which is the same string Mull uses to name the `.mull_mutants` global for a
mutant, and the same string the runner dispatches on. (The six-field encoding
stored *inside* that global appends the mutation's end location; it is
deliberately not emitted here, because it double-counts.)

`mutants.tsv` columns:

    identifier  region  points  kept_points  filtered_points  functions  filtered_by

`points` is how many *mutation points* carry that identifier. It is usually 1,
but a source line inside a template is emitted once per instantiation, so one
identifier can cover several IR mutations that a single mutant key activates
together. On `bgu`, 14 of the 5,565 kept identifiers cover 2 points each --
5,579 kept points in total -- and `functions` names the two instantiations.

## Region tagging

Halide's C backend emits each app as one file: a shared runtime and
SIMD-emulation prefix, then the generator's own lowered pipeline, then the
`_argv` and `_metadata` wrappers. There are no `#line` directives and no
provenance comments, so the split has to be recovered from line ranges, and the
boundary differs per app.

`autodetect` derives it from the file, using the same definition as the existing
analysis scripts: the last line equal to `}  // namespace` before the first line
equal to `HALIDE_FUNCTION_ATTRS` ends the boilerplate, and the second
`HALIDE_FUNCTION_ATTRS` line ends the generator-specific body. At least three
`HALIDE_FUNCTION_ATTRS` lines must be present, which is what distinguishes an
emitted file from ordinary source; anything else is tagged `unknown` rather than
guessed at.

Labels are `boilerplate`, `generator_specific` and `wrapper_metadata` -- the
vocabulary the per-mutant CSVs already use, so a dump joins against them
directly.

Tagging is a label, not a filter: every point is still reported. The boilerplate
prefix is byte-identical source across apps, but each pipeline instantiates and
reaches a different part of it, so the same boilerplate mutant can plausibly
survive in one app and be killed in another. Dropping those points at generation
time would destroy that signal; partitioning after the fact does not.

## Example

```sh
cat > mull.yml <<'EOF'
mutators:
  - cxx_default
timeout: 99999999
quiet: false
includePaths:
  - .*
parallelization:
  workers: 16
dumpMutantsTo: /out/bgu
dumpOnly: true
regions:
  autodetect: true
EOF

MULL_CONFIG=$PWD/mull.yml /usr/lib/llvm-14/bin/clang++ \
  -std=c++17 -O1 -g -grecord-command-line -Wno-psabi \
  -fpass-plugin=<mull-ps>/output/mull-ir-frontend-14 \
  -c bgu.halide_generated.cpp -o /dev/null
```

Note that this reports **points identified** and, for the kept set, the mutants
that *would* be generated. Nothing here is compiled into a binary, run, or
killed.

## Related: confining a run to line ranges

`lineRanges` is the other opt-in cost lever, for files whose clone phase cannot
finish at all. It keeps only the mutation points whose source line falls inside
one of the configured ranges:

```yaml
lineRanges:
  - file: ".*halide_blur\\.halide_generated\\.cpp"   # regex over the point's file path
    from: 3447                                       # 1-based, inclusive
    to: 4204                                         # 1-based, inclusive; omit for end of file
```

Multiple entries are a union: a point is kept if it is inside *any* of them, and
they may overlap. A file matching no entry keeps nothing, so pointing one range
at one file confines the whole run to that part of that file. `from: 0`, `from`
greater than `to`, and an unusable regex are reported as errors rather than
quietly keeping nothing.

Absent key means the filter is never installed. It composes with `slice:` --
running every slice index of a line-range-confined population reproduces that
population exactly.

This is a cost lever, **not** the way to scope what gets tested: a point that is
never generated cannot be recovered later, which is why region *tagging* exists
above. Reach for `lineRanges` only when a file is otherwise unrunnable.
