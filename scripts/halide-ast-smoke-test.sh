#!/bin/bash
#
# End-to-end smoke test for the AST-route (mull-cxx-frontend) Halide operators
# on the new stack: Mull 0.34.0 (Bazel) + LLVM 20 + Halide v21.0.0.
#
# Companion to halide-smoke-test.sh, which covers the IR route. The two differ
# in three ways:
#
#   * the plugin is loaded with -fplugin (a Clang AST plugin), not
#     -fpass-plugin (an LLVM pass plugin);
#   * no junk detection is involved -- the AST route gets exact source
#     locations by construction, so -grecord-command-line is not load-bearing
#     here the way it is for the IR route;
#   * -I src/runtime is required. apps/*/..._generator.cpp pull in
#     tools/halide_trace_config.h, which includes HalideRuntime.h; that header
#     is not copied into build/include, so without it the TU fails to parse
#     before the plugin ever sees a mutation point.
#
# Defaults target apps/bilateral_grid, whose generator has exactly one
# BoundaryConditions call site (repeat_edge, line 18) -- the smallest real
# subject for the halide_boundary_conditions operator family.
#
# Validated results on Halide v21.0.0 / LLVM 20 / Mull 0.34.0:
#
#   bilateral_grid, halide_boundary_conditions
#     3 points, all at 18:24-18:70; 3 DIFFERS / 0 SAME / 0 rejected
#   bilateral_grid, halide_special_calls
#     2 points (clamp at 23 and 50); 2 DIFFERS / 0 SAME / 0 rejected
#   camera_pipe, halide_special_calls
#     24 points -- 9 select sites x 2 operators, plus 6 clamp sites;
#     20 DIFFERS / 4 SAME / 0 rejected. select->if_then_else is 7 DIFFERS /
#     2 SAME, reproducing the old stack's 7-of-9 exactly.
#
# The two SAME select->if_then_else mutants are genuinely IR-equivalent, not
# failures: Halide lowers the if_then_else intrinsic back into a Select when it
# can prove both branches are safe. Report this operator per-site rather than
# pooling its yield.
#
# Usage: scripts/halide-ast-smoke-test.sh <halide-src-with-build> <workdir>
#   e.g. scripts/halide-ast-smoke-test.sh ../halide-latest /tmp/ast-smoke
set -o pipefail

HALIDE=${1:?usage: halide-ast-smoke-test.sh <halide-src-with-build> <workdir>}
WORK=${2:?usage: halide-ast-smoke-test.sh <halide-src-with-build> <workdir>}
MULL=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
CLANGXX=${CLANGXX:-/usr/lib/llvm-20/bin/clang++}
PLUGIN=${PLUGIN:-$MULL/bazel-bin/libmull-cxx-ast-frontend-20.so}
GENERATOR_SRC=${GENERATOR_SRC:-apps/bilateral_grid/bilateral_grid_generator.cpp}
GENERATOR_NAME=${GENERATOR_NAME:-bilateral_grid}
MUTATORS=${MUTATORS:-halide_boundary_conditions}

HALIDE=$(cd "$HALIDE" && pwd)
mkdir -p "$WORK"
cd "$HALIDE" || exit 1

cat > "$WORK/mull.yml" <<EOF
mutators:
  - $MUTATORS
timeout: 99999999
quiet: false
includePaths:
  - .*
EOF

echo "=== [1/4] instrument the generator TU (Clang AST plugin) ==="
MULL_CONFIG=$WORK/mull.yml "$CLANGXX" \
  -std=c++17 -O1 -g -grecord-command-line \
  -fplugin="$PLUGIN" \
  -I build/include -I tools -I src/runtime \
  -c "$GENERATOR_SRC" -o "$WORK/gen.o" > "$WORK/instr.log" 2>&1 || {
    echo "instrumentation FAILED:"; grep -E "error|Assertion" "$WORK/instr.log" | head; exit 1; }

echo "functions traversed : $(grep -c 'Looking at function' "$WORK/instr.log")"
echo "mutation points     : $(grep -c 'Recording mutation point' "$WORK/instr.log")"
grep 'Recording mutation point' "$WORK/instr.log" | sed 's/^/  /'
# A mutation point landing in a Halide header rather than the generator's own
# source would mean the operator is matching library internals.
if grep 'Recording mutation point' "$WORK/instr.log" | grep -q 'Halide\.h'; then
  echo "WARNING: mutation points found inside Halide.h"
fi

echo
echo "=== [2/4] mutants recorded in .mull_mutants ==="
strings -a "$WORK/gen.o" \
  | grep -E "^[A-Za-z_]+:[^:]+:[0-9]+:[0-9]+:[0-9]+:[0-9]+$" \
  | sort -u > "$WORK/mutants.txt"
echo "distinct mutants: $(wc -l < "$WORK/mutants.txt")"

echo
echo "=== [3/4] link and run baseline ==="
"$CLANGXX" -std=c++17 -O1 -g -I build/include -I tools -I src/runtime \
  -c tools/GenGen.cpp -o "$WORK/gengen.o" || exit 1
"$CLANGXX" "$WORK/gen.o" "$WORK/gengen.o" -o "$WORK/generator" \
  -L build/src -lHalide -Wl,-rpath,"$HALIDE/build/src" -lpthread -ldl || exit 1
rm -rf "$WORK/out-base"; mkdir -p "$WORK/out-base"
"$WORK/generator" -g "$GENERATOR_NAME" -e static_library,h,stmt,assembly \
  -o "$WORK/out-base" target=host || exit 1

# Control: an unmutated re-run must be byte-identical, otherwise a "DIFFERS"
# verdict below could just be nondeterminism in the generator.
rm -rf "$WORK/out-ctl"; mkdir -p "$WORK/out-ctl"
"$WORK/generator" -g "$GENERATOR_NAME" -e stmt -o "$WORK/out-ctl" target=host >/dev/null 2>&1
if cmp -s "$WORK/out-base/$GENERATOR_NAME.stmt" "$WORK/out-ctl/$GENERATOR_NAME.stmt"; then
  echo "control: unmutated re-run byte-identical"
else
  echo "control: FAILED -- generator is nondeterministic, verdicts below are unreliable"
fi

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
      same=$((same+1)); verdict=SAME; n=0
    else
      differ=$((differ+1)); verdict=DIFFERS
      n=$(diff "$WORK/out-base/$GENERATOR_NAME.stmt" "$out/$GENERATOR_NAME.stmt" | grep -c '^[<>]')
    fi
  else
    failed=$((failed+1)); verdict=GENFAIL; n=0
  fi
  printf '%s\t%s\t%s\n' "$verdict" "$n" "$key" >> "$WORK/results.tsv"
  printf '  %-10s %5s changed .stmt lines  %s\n' "$verdict" "$n" "${key%%:*}"
  rm -rf "$out"
done < "$WORK/mutants.txt"

echo
echo "=== SUMMARY over $i mutants ==="
echo "stmt DIFFERS from baseline (mutation reached emitted Halide IR): $differ"
echo "stmt identical (silent no-op)                                 : $same"
echo "generator rejected the mutated pipeline                       : $failed"
