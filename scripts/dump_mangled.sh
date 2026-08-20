#!/bin/bash
# Regenerate the tables in docs/halide-mangled-names.md.
#
# Compiles each benchmark generator TU against a Halide build tree, extracts the
# Itanium-mangled names of the APIs targeted by the mutation operators, and
# measures how often each lowers to `call` vs `invoke`.
#
#   scripts/dump_mangled.sh --halide /path/to/Halide-mutation \
#                           --llvm   /usr/lib/llvm-14 \
#                           --out    /tmp/halide-mangled
set -euo pipefail

HALIDE=""
LLVM_PREFIX=/usr/lib/llvm-14
OUT=/tmp/halide-mangled

while [ $# -gt 0 ]; do
  case "$1" in
    --halide) HALIDE="$2"; shift 2 ;;
    --llvm)   LLVM_PREFIX="$2"; shift 2 ;;
    --out)    OUT="$2"; shift 2 ;;
    -h|--help) sed -n '2,10p' "$0"; exit 0 ;;
    *) echo "unknown argument: $1" >&2; exit 1 ;;
  esac
done

if [ -z "$HALIDE" ]; then
  echo "error: --halide <path to Halide source tree with a build/ dir> is required" >&2
  exit 1
fi
if [ ! -f "$HALIDE/build/include/Halide.h" ]; then
  echo "error: $HALIDE/build/include/Halide.h not found; build Halide first" >&2
  exit 1
fi

CXX="$LLVM_PREFIX/bin/clang++"
NM="$LLVM_PREFIX/bin/llvm-nm"
mkdir -p "$OUT/ll"

APPS="
apps/bilateral_grid/bilateral_grid_generator.cpp
apps/blur/halide_blur_generator.cpp
apps/camera_pipe/camera_pipe_generator.cpp
apps/harris/harris_generator.cpp
apps/interpolate/interpolate_generator.cpp
apps/local_laplacian/local_laplacian_generator.cpp
apps/unsharp/unsharp_generator.cpp
apps/conv_layer/conv_layer_generator.cpp
apps/iir_blur/iir_blur_generator.cpp
apps/max_filter/max_filter_generator.cpp
apps/lens_blur/lens_blur_generator.cpp
apps/stencil_chain/stencil_chain_generator.cpp
apps/bgu/bgu_generator.cpp
apps/nl_means/nl_means_generator.cpp
apps/depthwise_separable_conv/depthwise_separable_conv_generator.cpp
"

: > "$OUT/all_symbols.txt"
for app in $APPS; do
  [ -f "$HALIDE/$app" ] || { echo "skip (missing): $app"; continue; }
  base=$(basename "$app" .cpp)
  "$CXX" -std=c++17 -O1 -c -fno-discard-value-names \
    -I "$HALIDE/build/include" -I "$HALIDE/tools" \
    "$HALIDE/$app" -o "$OUT/$base.o"
  "$CXX" -std=c++17 -O1 -S -emit-llvm -fno-discard-value-names \
    -I "$HALIDE/build/include" -I "$HALIDE/tools" \
    "$HALIDE/$app" -o "$OUT/ll/$base.ll"
  echo "ok: $app"
done

"$NM" "$OUT"/*.o 2>/dev/null | awk '{print $NF}' | grep '^_Z' | sort -u > "$OUT/all_symbols.txt"
echo "unique mangled symbols: $(wc -l < "$OUT/all_symbols.txt")"

echo
echo "=== arithmetic operators ==="
grep -E '^_ZN6Halide(pl|mi|ml|dv)' "$OUT/all_symbols.txt" \
  | while read -r s; do printf '%-46s %s\n' "$s" "$("$LLVM_PREFIX/bin/llvm-cxxfilt" "$s")"; done

echo
echo "=== schedule directives ==="
grep -E '^_ZN6Halide(4Func|5Stage)(9vectorize|6unroll|8parallel|10compute_at|8store_at)' \
  "$OUT/all_symbols.txt" | sort \
  | while read -r s; do printf '%-72s %s\n' "$s" "$("$LLVM_PREFIX/bin/llvm-cxxfilt" "$s")"; done

echo
echo "=== BoundaryConditions ==="
grep 'BoundaryConditions' "$OUT/all_symbols.txt" \
  | while read -r s; do printf '%s\n    %s\n' "$s" "$("$LLVM_PREFIX/bin/llvm-cxxfilt" "$s")"; done

echo
echo "=== call vs invoke ==="
python3 - "$OUT/ll" <<'PY'
import re, sys, glob, os, collections
lldir = sys.argv[1]
targets = {
    "6Halidepl": "Halide::operator+", "6Halidemi": "Halide::operator-",
    "6Halideml": "Halide::operator*", "6Halidedv": "Halide::operator/",
    "4Func9vectorize": "Func::vectorize", "4Func6unroll": "Func::unroll",
    "4Func8parallel": "Func::parallel", "5Stage9vectorize": "Stage::vectorize",
    "5Stage6unroll": "Stage::unroll", "5Stage8parallel": "Stage::parallel",
    "11repeat_edge": "BoundaryConditions::repeat_edge",
    "4Func10compute_at": "Func::compute_at", "4Func8store_at": "Func::store_at",
}
stats = collections.defaultdict(lambda: [0, 0])
line_re = re.compile(r'^\s*(?:%\S+\s*=\s*)?(call|invoke)\b.*?@([A-Za-z0-9_$.]+)')
for path in sorted(glob.glob(os.path.join(lldir, "*.ll"))):
    for line in open(path, errors="replace"):
        m = line_re.match(line)
        if not m:
            continue
        kind, sym = m.group(1), m.group(2)
        for frag, label in targets.items():
            if frag in sym:
                stats[label][0 if kind == "call" else 1] += 1
                break
print(f"{'API':<34}{'call':>7}{'invoke':>8}{'total':>7}{'  invoke%':>10}")
print("-" * 67)
g = [0, 0]
for label in sorted(stats, key=lambda l: -sum(stats[l])):
    c, i = stats[label]; t = c + i; g[0] += c; g[1] += i
    print(f"{label:<34}{c:>7}{i:>8}{t:>7}{(100.0*i/t):>9.1f}%")
print("-" * 67)
t = g[0] + g[1]
print(f"{'ALL':<34}{g[0]:>7}{g[1]:>8}{t:>7}{(100.0*g[1]/t):>9.1f}%")
PY
