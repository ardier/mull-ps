# Building and running the Halide mutation toolchain on Linux

Verified on Ubuntu 24.04, GCC 13.3.0, CMake 3.28.3, LLVM 14.0.6
(`/usr/lib/llvm-14`, from the distribution packages).

LLVM 14 is the anchor version: it is what Mull recommends, what the Halide
mutation work was originally built against, and what Halide `release/16.x`
still supports (that branch accepts LLVM 14.0–17.0). Halide `main` (~v18)
requires LLVM ≥ 16 and will not build here.

| Repo | Branch | Role |
|---|---|---|
| `mull-ps` | `fse2027` (off `origin/halide`) | Mull fork; mutation frontends and runner |
| `libirm-halide` | `fse2027` (off `origin/halide-mutants`) | IR mutation primitives, incl. `HalideReplacement` |
| `Halide-mutation` | `fse2027` (off `origin/release/16.x`) | Halide + the benchmark apps |

## Prerequisites

```sh
sudo apt install llvm-14-dev libclang-14-dev clang-14 cmake ninja-build
```

## 1. Build Mull

`vendor/libirm` tracks `../libirm-halide` at `fse2027`. The relative URL
resolves against the superproject's remote, so point it at a local clone with:

```sh
cd mull-ps
git config submodule.vendor/libirm.url /path/to/libirm-halide
make            # = clean + mull + yml
```

`make mull` alone configures and builds, then stages into `output/`:

| Binary | Purpose |
|---|---|
| `mull-ir-frontend-14` | LLVM IR mutation pass, used via `-fpass-plugin=` |
| `libmull-cxx-frontend-14.so` | Clang AST mutation plugin, used via `-fplugin=` |
| `mull-runner-14` | mutant runner / reporter |

Override the LLVM installation with `make LLVM_PREFIX=/usr/lib/llvm-16`; binary
names follow `llvm-config --version`.

Run the unit tests, including the `HalideReplacement` regression suite:

```sh
make test       # currently 36/36
```

### Build with GCC, not Clang

Configure with the default system GCC. Building Mull with `clang-14` fails:
clang 14 cannot parse libstdc++ 13's `<bits/stl_tree.h>` (`_M_erase` passes a
`_Link_type` where a `_Base_ptr` is expected), so every `std::map`
instantiation errors out. GCC 13 also matches how the distro LLVM 14 packages
were built.

## 2. Build Halide

```sh
cd Halide-mutation
cmake -G Ninja -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DHalide_REQUIRE_LLVM_VERSION=14 \
  -DHalide_SHARED_LLVM=YES \
  -DCMAKE_PREFIX_PATH="/usr/lib/llvm-14/lib/cmake/llvm;/usr/lib/llvm-14/lib/cmake/clang;/usr/lib/llvm-14/lib/cmake/lld" \
  -DWITH_TESTS=OFF -DWITH_TUTORIALS=OFF -DWITH_DOCS=OFF \
  -DWITH_PYTHON_BINDINGS=OFF -DWITH_UTILS=OFF
ninja -C build
```

`Halide_SHARED_LLVM=YES` is required: Ubuntu's LLD is linked against the shared
LLVM, and Halide otherwise fails configuration with

> LLD was linked to shared LLVM (see: LLVM_LINK_LLVM_DYLIB), but static LLVM was
> requested.

Produces `build/src/libHalide.so.16.0.0` and the generated `build/include/Halide.h`.

## 3. Mutating a generator (IR route)

Stage 1 — instrument **one** translation unit into an object file:

```sh
cat > mull.yml <<'EOF'
mutators:
  - halide_mutator
timeout: 99999999
quiet: false
includePaths:
  - .*
EOF

MULL_CONFIG=$PWD/mull.yml /usr/lib/llvm-14/bin/clang++ \
  -std=c++17 -O1 -g -grecord-command-line \
  -fpass-plugin=<mull-ps>/output/mull-ir-frontend-14 \
  -I build/include -I tools \
  -c apps/blur/halide_blur_generator.cpp -o gen.o
```

Two flags matter:

- **`-grecord-command-line`** is required. Mull's junk detector re-parses the
  source with Clang and reconstructs the compile flags from the recorded
  command line. Without it the include paths are lost, the re-parse fails with
  `'Halide.h' file not found`, and every mutation point is discarded as junk —
  yielding zero mutants with only a warning.
- **Compile a single source file per invocation.** Mull rejects a recorded
  command line that produces more than one compiler job
  (`expected exactly one compiler job`), so the generator TU and `GenGen.cpp`
  must be compiled separately and then linked.

Link the generator:

```sh
/usr/lib/llvm-14/bin/clang++ -std=c++17 -O1 -g -I build/include -I tools \
  -c tools/GenGen.cpp -o gengen.o
/usr/lib/llvm-14/bin/clang++ gen.o gengen.o -o blur.generator \
  -L build/src -lHalide -Wl,-rpath,$PWD/build/src -lpthread -ldl
```

List the mutants — the env-var key is `<mutator-id>:<file>:<line>:<column>`:

```sh
strings gen.o | grep -E '^Halide_[a-z_]+:/[^:]+:[0-9]+:[0-9]+$' | sort -u
```

Note the binary also contains a longer `…:<line>:<col>:<endline>:<endcol>` form.
That is Mull's reporting identifier, **not** an env-var key; matching it too
double-counts the mutant population.

Stage 2 — run the generator with one mutant enabled:

```sh
./blur.generator -g halide_blur -e static_library,h,stmt,assembly -o out target=host          # baseline
env "Halide_add_to_sub:$PWD/apps/blur/halide_blur_generator.cpp:39:37"=1 \
  ./blur.generator -g halide_blur -e static_library,h,stmt,assembly -o out-mut target=host
diff out/halide_blur.stmt out-mut/halide_blur.stmt
```

Stage 3 — build and run the app's own test against the mutant:

```sh
/usr/lib/llvm-14/bin/clang++ -std=c++17 -O2 -I out-mut -I tools -I build/include \
  apps/blur/test.cpp out-mut/halide_blur.a -o test-mut -lpthread -ldl
./test-mut          # exit 134 (abort) => killed
```

## 4. Mutating a generator (AST route)

```sh
/usr/lib/llvm-14/bin/clang++ -std=c++17 \
  -fplugin=<mull-ps>/output/libmull-cxx-frontend-14.so \
  -I build/include -I tools \
  -c apps/blur/halide_blur_generator.cpp -o gen-ast.o
```

The plugin prints `Recording mutation point: <id>:<file>:<line>:<col>` as it
runs, and mutants are selected by the same env-var mechanism. Paths in the key
are recorded exactly as they appeared on the command line, so a relative path
in the compile line means a relative path in the key.

Stock C++ operators find no mutation points inside a Halide generator's
`generate()` body: the arithmetic there is entirely `Halide::Expr` operator
overloads (`CXXOperatorCallExpr`), while `cxx_add_to_sub` and friends match
builtin `BinaryOperator` nodes. On `apps/blur` all 76 points land in
`Halide.h`. Reaching generator-level Halide constructs requires the
Halide-specific AST mutators.

## Known behaviours worth remembering

- Mull's junk detector exempts Halide mutation points from the system-header
  filter by matching the `Halide_` identifier prefix. Most Halide points do
  resolve into headers, since the mutated operators are Halide's own inline and
  templated API, so without that exemption they are all discarded.
- `stdout` is lost when a subject calls `abort()`; use `stdbuf -o0` when you
  need to see which assertion fired.
