#!/bin/bash
#
# End-to-end smoke test for the Halide operators on the new stack:
#   Mull 0.34.0 (Bazel) + LLVM 20 + Halide v21.0.0.
#
# Mirrors the IR-route recipe from mull-ps/docs/linux-build.md section 3, with
# one behavioural change forced by the upgrade:
#
#   Mull <= 0.21 recorded two identifiers per mutation point -- a short
#   `<id>:<file>:<line>:<col>` env-var key and a longer
#   `...:<endline>:<endcol>` reporting identifier. Mull 0.34 collapsed these
#   into a single 6-field identifier (MutationPoint::updateIdentifier), which is
#   both what lands in the .mull_mutants section and what selects a mutant at
#   run time. Any tooling that still greps for the 4-field form finds nothing.
#
# Usage: scripts/halide-smoke-test.sh <halide-build-dir> <workdir>
#   e.g. scripts/halide-smoke-test.sh ../halide-latest /tmp/smoke
set -o pipefail

HALIDE=${1:?usage: halide-smoke-test.sh <halide-src-with-build> <workdir>}
WORK=${2:?usage: halide-smoke-test.sh <halide-src-with-build> <workdir>}
MULL=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
CLANGXX=${CLANGXX:-/usr/lib/llvm-20/bin/clang++}
FRONTEND=${FRONTEND:-$MULL/bazel-bin/mull-ir-frontend-20}
GENERATOR_SRC=${GENERATOR_SRC:-apps/blur/halide_blur_generator.cpp}
GENERATOR_NAME=${GENERATOR_NAME:-halide_blur}

HALIDE=$(cd "$HALIDE" && pwd)
mkdir -p "$WORK"
cd "$HALIDE" || exit 1

cat > "$WORK/mull.yml" <<'EOF'
mutators:
  - halide_mutator
timeout: 99999999
quiet: false
includePaths:
  - .*
EOF

# -grecord-command-line is required: Mull's junk detector re-parses the source
# and reconstructs the compile flags from the recorded command line. Without it
# the include paths are lost, the re-parse fails with 'Halide.h' file not found,
# and every mutation point is discarded as junk.
echo "=== [1/4] instrument the generator TU ==="
MULL_CONFIG=$WORK/mull.yml "$CLANGXX" \
  -std=c++17 -O1 -g -grecord-command-line \
  -fpass-plugin="$FRONTEND" \
  -I build/include -I tools \
  -c "$GENERATOR_SRC" -o "$WORK/gen.o" || exit 1

echo
echo "=== [2/4] mutants recorded in .mull_mutants ==="
strings -a "$WORK/gen.o" \
  | grep -E "^Halide_[a-z_]+:/[^:]+:[0-9]+:[0-9]+:[0-9]+:[0-9]+$" \
  | sort -u > "$WORK/mutants.txt"
echo "distinct mutants: $(wc -l < "$WORK/mutants.txt")"
cut -d: -f1 "$WORK/mutants.txt" | sort | uniq -c | sort -rn

echo
echo "=== [3/4] link and run baseline ==="
"$CLANGXX" -std=c++17 -O1 -g -I build/include -I tools \
  -c tools/GenGen.cpp -o "$WORK/gengen.o" || exit 1
"$CLANGXX" "$WORK/gen.o" "$WORK/gengen.o" -o "$WORK/generator" \
  -L build/src -lHalide -Wl,-rpath,"$HALIDE/build/src" -lpthread -ldl || exit 1
rm -rf "$WORK/out-base"; mkdir -p "$WORK/out-base"
"$WORK/generator" -g "$GENERATOR_NAME" -e static_library,h,stmt,assembly \
  -o "$WORK/out-base" target=host || exit 1

echo
echo "=== [4/4] run each mutant, diff emitted .stmt against baseline ==="
differ=0; same=0; failed=0; i=0
: > "$WORK/results.tsv"
while read -r key; do
  i=$((i+1))
  out="$WORK/out-m$i"; rm -rf "$out"; mkdir -p "$out"
  if env "$key=1" "$WORK/generator" -g "$GENERATOR_NAME" \
        -e static_library,h,stmt,assembly -o "$out" target=host > "$out/gen.log" 2>&1; then
    if diff -q "$WORK/out-base/$GENERATOR_NAME.stmt" "$out/$GENERATOR_NAME.stmt" >/dev/null 2>&1; then
      same=$((same+1)); verdict=SAME
    else
      differ=$((differ+1)); verdict=DIFFERS
    fi
  else
    # A schedule-directive mutant can produce a schedule Halide legitimately
    # rejects (e.g. unroll over a non-constant extent). That is a stage-2
    # rejection, not a tool failure.
    failed=$((failed+1)); verdict=GENFAIL
  fi
  printf '%s\t%s\n' "$verdict" "$key" >> "$WORK/results.tsv"
  rm -rf "$out"
done < "$WORK/mutants.txt"

echo "=== SUMMARY over $i mutants ==="
echo "stmt DIFFERS from baseline (mutation reached emitted Halide IR): $differ"
echo "stmt identical (silent no-op)                                 : $same"
echo "generator rejected the mutated schedule                       : $failed"
echo
awk -F'\t' '{split($2,a,":"); print $1"\t"a[1]}' "$WORK/results.tsv" | sort | uniq -c | sort -k3,3
