# Verified Halide mangled names for the IR mutation operators

Reference data for implementing `HalideReplacement` mapping tables. Every string
here was extracted from a real compilation, not derived by hand from the Itanium
ABI spec.

**Provenance.** Halide `release/16.x` (`027547f71`, `libHalide.so.16.0.0`) built
against LLVM 14.0.6 (`/usr/lib/llvm-14`); generator TUs compiled with

```
/usr/lib/llvm-14/bin/clang++ -std=c++17 -O1 -c \
    -I <halide-build>/include -I <halide-src>/tools \
    apps/<app>/<app>_generator.cpp -o <app>.o
/usr/lib/llvm-14/bin/llvm-nm <app>.o | awk '{print $NF}' | c++filt
```

Corpus: the 15 benchmark generators that compile standalone — `bgu`,
`bilateral_grid`, `blur`, `camera_pipe`, `conv_layer`,
`depthwise_separable_conv`, `harris`, `iir_blur`, `interpolate`, `lens_blur`,
`local_laplacian`, `max_filter`, `nl_means`, `stencil_chain`, `unsharp`.

Regenerate with `scripts/dump_mangled.sh` (see "Reproducing" at the end).

---

## 1. Arithmetic operators — the existing 12 operators

`Halide::Expr` is passed **by value**, so the mangling is `NS_4ExprE`, *not*
`RKNS_4ExprE`. The operator token is the two-letter Itanium code immediately
after `_ZN6Halide`: `pl` `mi` `ml` `dv`.

| Overload | Mangled name |
|---|---|
| `operator+(Expr, Expr)` | `_ZN6HalideplENS_4ExprES0_` |
| `operator+(Expr, int)` | `_ZN6HalideplENS_4ExprEi` |
| `operator+(int, Expr)` | `_ZN6HalideplEiNS_4ExprE` |
| `operator-(Expr, Expr)` | `_ZN6HalidemiENS_4ExprES0_` |
| `operator-(Expr, int)` | `_ZN6HalidemiENS_4ExprEi` |
| `operator-(int, Expr)` | `_ZN6HalidemiEiNS_4ExprE` |
| `operator*(Expr, Expr)` | `_ZN6HalidemlENS_4ExprES0_` |
| `operator*(Expr, int)` | `_ZN6HalidemlENS_4ExprEi` |
| `operator*(int, Expr)` | `_ZN6HalidemlEiNS_4ExprE` |
| `operator/(Expr, Expr)` | `_ZN6HalidedvENS_4ExprES0_` |
| `operator/(Expr, int)` | `_ZN6HalidedvENS_4ExprEi` |
| `operator/(int, Expr)` | `_ZN6HalidedvEiNS_4ExprE` |

The existing mappings (`{"Halidepl", "Halidemi"}` etc.) are substrings that
match all three overloads of a given operator uniformly, and all four tokens are
two characters, so the rewrite is length-preserving here. **Do not generalise
that length invariance** — it does not hold for the schedule directives below.

---

## 2. Schedule directives — highest-yield new operator family

Non-templated, out-of-line, and **signature-identical across all three
directives** in both the 1-argument and 3-argument forms, for both `Func` and
`Stage`. This is the cleanest possible fit for the mangled-name swap.

The name embeds the Itanium length prefix, so the token to swap is
`9vectorize` / `6unroll` / `8parallel` — **the replacement changes the string
length**. `HalideReplacement::mutate` uses `std::string::replace`, which handles
that correctly; any future length-based assumption would break these.

### `Func`, 1-arg — `(VarOrRVar const &)`

| Directive | Mangled name |
|---|---|
| `Func::vectorize` | `_ZN6Halide4Func9vectorizeERKNS_9VarOrRVarE` |
| `Func::unroll` | `_ZN6Halide4Func6unrollERKNS_9VarOrRVarE` |
| `Func::parallel` | `_ZN6Halide4Func8parallelERKNS_9VarOrRVarE` |

### `Func`, 3-arg — `(VarOrRVar const &, Expr const &, TailStrategy)`

| Directive | Mangled name |
|---|---|
| `Func::vectorize` | `_ZN6Halide4Func9vectorizeERKNS_9VarOrRVarERKNS_4ExprENS_12TailStrategyE` |
| `Func::unroll` | `_ZN6Halide4Func6unrollERKNS_9VarOrRVarERKNS_4ExprENS_12TailStrategyE` |
| `Func::parallel` | `_ZN6Halide4Func8parallelERKNS_9VarOrRVarERKNS_4ExprENS_12TailStrategyE` |

### `Stage`, 1-arg and 3-arg

| Directive | Mangled name |
|---|---|
| `Stage::vectorize` | `_ZN6Halide5Stage9vectorizeERKNS_9VarOrRVarE` |
| `Stage::unroll` | `_ZN6Halide5Stage6unrollERKNS_9VarOrRVarE` |
| `Stage::parallel` | `_ZN6Halide5Stage8parallelERKNS_9VarOrRVarE` † |
| `Stage::vectorize` | `_ZN6Halide5Stage9vectorizeERKNS_9VarOrRVarERKNS_4ExprENS_12TailStrategyE` |
| `Stage::unroll` | `_ZN6Halide5Stage6unrollERKNS_9VarOrRVarERKNS_4ExprENS_12TailStrategyE` |
| `Stage::parallel` | `_ZN6Halide5Stage8parallelERKNS_9VarOrRVarERKNS_4ExprENS_12TailStrategyE` † |

† **Not called by any app in the corpus**, so it never appears as an undefined
symbol in a generator object. Both names were instead confirmed directly against
the built library, and are exact:

```
$ llvm-nm -D --defined-only build/src/libHalide.so.16.0.0 | grep 5Stage8parallel
_ZN6Halide5Stage8parallelERKNS_9VarOrRVarE
_ZN6Halide5Stage8parallelERKNS_9VarOrRVarERKNS_4ExprENS_12TailStrategyE
```

This is exactly the case the `getOrInsertFunction` fallback exists for: mutating
`Stage::vectorize` → `Stage::parallel` requires a target symbol that is absent
from every TU in the corpus, and would otherwise silently no-op.

### `compute_at` / `store_at`

Halide defines both over the **same three parameter lists**, so every mapping
has a counterpart and no swap can synthesise a name `libHalide` does not
define. (An earlier revision of this file claimed `store_at` lacked the `RVar`
overload — it does not; checked against `libHalide.so.16.0.0`.)

| API | Mangled name |
|---|---|
| `Func::compute_at(LoopLevel)` | `_ZN6Halide4Func10compute_atENS_9LoopLevelE` |
| `Func::store_at(LoopLevel)` | `_ZN6Halide4Func8store_atENS_9LoopLevelE` |
| `Func::compute_at(Func const &, Var const &)` | `_ZN6Halide4Func10compute_atERKS0_RKNS_3VarE` |
| `Func::store_at(Func const &, Var const &)` | `_ZN6Halide4Func8store_atERKS0_RKNS_3VarE` |
| `Func::compute_at(Func const &, RVar const &)` | `_ZN6Halide4Func10compute_atERKS0_RKNS_4RVarE` |
| `Func::store_at(Func const &, RVar const &)` | `_ZN6Halide4Func8store_atERKS0_RKNS_4RVarE` |

Because the sets are symmetric, matching just the class and method tokens
(`4Func10compute_at` → `4Func8store_at`) covers all three overloads at once.
That fragment appears in exactly 3 `libHalide` symbols and in nothing outside
`Halide::Func` — notably not in the templated
`Internal::GeneratorOutputBase::compute_at` forwarder, whose mangled name
carries a different class token.

### Implemented mapping fragments

These are the fragments the operators actually use. Each was checked to match
exactly its intended overloads in `libHalide.so.16.0.0` and nothing outside
`Halide::Func`/`Halide::Stage`.

| Operator | match | replace | symbols hit |
|---|---|---|---|
| `Halide_vectorize_to_unroll` | `9vectorizeERKNS_9VarOrRVarE` | `6unrollERKNS_9VarOrRVarE` | 4 |
| `Halide_vectorize_to_parallel` | `9vectorizeERKNS_9VarOrRVarE` | `8parallelERKNS_9VarOrRVarE` | 4 |
| `Halide_unroll_to_vectorize` | `6unrollERKNS_9VarOrRVarE` | `9vectorizeERKNS_9VarOrRVarE` | 4 |
| `Halide_unroll_to_parallel` | `6unrollERKNS_9VarOrRVarE` | `8parallelERKNS_9VarOrRVarE` | 4 |
| `Halide_parallel_to_vectorize` | `8parallelERKNS_9VarOrRVarE` | `9vectorizeERKNS_9VarOrRVarE` | 4 |
| `Halide_parallel_to_unroll` | `8parallelERKNS_9VarOrRVarE` | `6unrollERKNS_9VarOrRVarE` | 4 |
| `Halide_compute_at_to_store_at` | `4Func10compute_at` | `4Func8store_at` | 3 |
| `Halide_store_at_to_compute_at` | `4Func8store_at` | `4Func10compute_at` | 3 |

Matching the method token together with its first parameter is what lets one
mapping cover a whole overload set: the 3-argument mangled name simply
continues past the matched prefix with `RKNS_4ExprENS_12TailStrategyE`, which
the rewrite leaves untouched. The class token stays outside the match, so a
`Func::` call can never be redirected to a `Stage::` overload.

### Call-site census (source-level, 15 apps)

| Directive | `.vectorize(` | `.unroll(` | `.parallel(` |
|---|---|---|---|
| occurrences | 93 | 147 | 33 |

273 total, present in all 15 apps, with no C++ or GPL sibling operator. Per-app
IR-level counts are in §5.

---

## 3. `BoundaryConditions` — use the AST route, not this one

Only `repeat_edge` appears anywhere in the corpus (11 source-level call sites).
`repeat_image`, `mirror_image`, `mirror_interior` and `constant_exterior` have
**zero** call sites, so they are viable only as mutation *targets*.

The blocking problem is that the idiomatic entry point is a **function template
parameterised on the input type**, so the mangled name embeds the buffer's
element type and dimensionality. A mangled-name swap needs one mapping per
instantiation actually present:

```
_ZN6Halide18BoundaryConditions11repeat_edgeINS_14GeneratorInputINS_6BufferIfLi2EEEEEEENS_4FuncERKT_
    -> repeat_edge<GeneratorInput<Buffer<float, 2>>>(...)
_ZN6Halide18BoundaryConditions11repeat_edgeINS_14GeneratorInputINS_6BufferIfLi3EEEEEEENS_4FuncERKT_
_ZN6Halide18BoundaryConditions11repeat_edgeINS_14GeneratorInputINS_6BufferIhLi3EEEEEEENS_4FuncERKT_
_ZN6Halide18BoundaryConditions11repeat_edgeINS_14GeneratorInputINS_6BufferItLi2EEEEEEENS_4FuncERKT_
_ZN6Halide18BoundaryConditions11repeat_edgeINS_14GeneratorInputINS_6BufferItLi3EEEEEEENS_4FuncERKT_
_ZN6Halide18BoundaryConditions11repeat_edgeINS_14GeneratorInputINS_6BufferIfLi3EEEEEEENS_4FuncERKT_RKSt6vectorINS_5RangeESaISB_EE
```

Five element-type/dimension combinations in 15 apps, and the set grows with
every new benchmark. These are also `linkonce_odr` definitions emitted *into the
generator TU*, not calls resolved from `libHalide`, so a redirect would have to
target a template instantiation that may not exist.

Exactly one non-templated overload is resolved out-of-line and is swappable by
name:

```
_ZN6Halide18BoundaryConditions11repeat_edgeERKNS_4FuncERKSt6vectorINS_5RangeESaIS5_EE
    -> repeat_edge(Func const &, std::vector<Range> const &)
```

This family is the reason the plan routes `BoundaryConditions` through
`mull-cxx-frontend`, where the AST node is matched directly and the template
instantiation is irrelevant.

---

## 4. Not reachable from the IR route at all

| API | Source call sites | Out-of-line symbol? | Note |
|---|---|---|---|
| `select(cond, a, b)` | 14 | **none** | inlined at every site; nothing to redirect |
| `clamp(x, lo, hi)` | 25 | **none** | inlined at every site |
| `RDom` ctor | 16 | variadic template | `RDom<int,int>`, `RDom<int,Expr,int,Expr>`, `RDom<int,GeneratorParam<int>&>`, … — one mangling per argument-type tuple |
| `if_then_else` | **0** | n/a | no public `Halide::if_then_else` exists; it is `Internal::Call::if_then_else`, a compiler intrinsic |
| `likely()` | **0** | n/a | target-only |

`select` and `clamp` produce no undefined symbol in any of the 15 objects: they
are header-inline and the call is gone before the IR pass runs. `RDom`'s
constructor is a variadic template whose mangling encodes the full argument
type-pack. All four families require the AST route.

---

## 5. `call` vs `invoke` — measured, not assumed

`HalideReplacement` originally matched only `llvm::InvokeInst`. Whether a Halide
API call is emitted as `call` or `invoke` depends on whether a destructible
temporary is live at the call site under the Itanium ABI, which is a property of
the surrounding expression rather than of any `try`/`catch`.

Measured over all 15 generator TUs at `-O1`:

| API | `call` | `invoke` | invoke % |
|---|---|---|---|
| `Halide::operator+` | 0 | 290 | 100.0% |
| `Halide::operator*` | 0 | 239 | 100.0% |
| `Halide::operator-` | 0 | 130 | 100.0% |
| `Func::unroll` | 0 | 116 | 100.0% |
| `Func::compute_at` | 0 | 94 | 100.0% |
| `Halide::operator/` | 0 | 86 | 100.0% |
| `Func::vectorize` | 0 | 83 | 100.0% |
| `Func::parallel` | 0 | 33 | 100.0% |
| `Stage::unroll` | 0 | 31 | 100.0% |
| `BoundaryConditions::repeat_edge` | **1** | 20 | 95.2% |
| `Func::store_at` | 0 | 17 | 100.0% |
| `Stage::vectorize` | 0 | 9 | 100.0% |
| **all targeted APIs** | **1** | **1148** | **99.9%** |

So on this corpus, with exceptions enabled, the `InvokeInst`-only restriction
costs almost nothing — the single `call` is a `repeat_edge` used as the first
statement of a generator body, which is precisely the predicted blind spot.

The restriction is catastrophic under `-fno-exceptions`. Measured on
`apps/blur/halide_blur_generator.cpp`:

| Flags | `call` | `invoke` |
|---|---|---|
| `-O0` | 0 | 19 |
| `-O1` | 0 | 19 |
| `-O2` | 0 | 19 |
| `-O1 -fno-exceptions` | **19** | **0** |

Every site becomes a plain `call`, so an `InvokeInst`-only mechanism yields
**zero mutants and no diagnostic**. This is why `canMutate`/`mutate` match
`llvm::CallBase`.

### Schedule-directive call sites per app (IR level, `call` + `invoke`)

| app | `Func::vec` | `Func::unr` | `Func::par` | `Stage::vec` | `Stage::unr` | total |
|---|---|---|---|---|---|---|
| depthwise_separable_conv | 5 | 21 | 1 | 2 | 11 | 40 |
| conv_layer | 4 | 14 | 3 | 1 | 9 | 31 |
| lens_blur | 13 | 10 | 5 | 1 | 2 | 31 |
| bgu | 8 | 9 | 5 | 1 | 3 | 26 |
| camera_pipe | 7 | 17 | 1 | 0 | 0 | 25 |
| interpolate | 4 | 16 | 2 | 0 | 0 | 22 |
| harris | 7 | 11 | 1 | 0 | 0 | 19 |
| bilateral_grid | 7 | 5 | 2 | 0 | 2 | 16 |
| local_laplacian | 6 | 0 | 5 | 0 | 0 | 11 |
| nl_means | 5 | 2 | 1 | 1 | 2 | 11 |
| stencil_chain | 4 | 5 | 1 | 0 | 0 | 10 |
| blur | 5 | 2 | 2 | 0 | 0 | 9 |
| unsharp | 4 | 4 | 1 | 0 | 0 | 9 |
| iir_blur | 1 | 0 | 2 | 2 | 2 | 7 |
| max_filter | 3 | 0 | 1 | 1 | 0 | 5 |
| **total** | **83** | **116** | **33** | **9** | **31** | **272** |

`Stage::parallel` is 0 in every app.

---

## 6. Reproducing

`scripts/dump_mangled.sh` regenerates every table above. It needs a Halide build
tree (for the generated `Halide.h`) and LLVM 14:

```
scripts/dump_mangled.sh --halide /path/to/Halide-mutation \
                        --llvm   /usr/lib/llvm-14 \
                        --out    /tmp/halide-mangled
```

To check a single name against the built library:

```
llvm-nm -D --defined-only <halide-build>/src/libHalide.so \
  | awk '{print $NF}' | grep '^_ZN6Halide4Func9vectorize'
```

---

## 7. Full operator census (relational, logical, bitwise, assignment, unary)

Taken from `libHalide.so.16.0.0`'s symbol table with an exact Itanium
length-prefix parse. A loose `\d+[A-Za-z_]*` class-name pattern over-matches
badly -- it reports 158 `operator<=` overloads where there are 3, by letting the
class name absorb part of the following identifier.

### What `Halide::Expr` actually overloads

| C++ operator | Itanium | overloads | note |
|---|---|---|---|
| `+ - * /` | `pl mi ml dv` | 6, 5, 6, 5 | |
| `%` | `rm` | 5 | |
| `< > <= >=` | `lt gt le ge` | 3, 3, 3, 3 | `>=` uses `const Expr &` for its mixed forms |
| `== !=` | `eq ne` | 4, 3 | `==` also has an `(Expr, float)` form |
| `&& \|\|` | `aa oo` | 3, 3 | mixed forms take `bool`, not `int` |
| `& \| ^` | `an or eo` | 3, 3, 3 | |
| `<< >>` | `ls rs` | 31, 2 | 29 of `<<`'s are `std::ostream` overloads |
| `+= -= *= /=` | `pL mI mL dV` | 5 each | free form plus `FuncRef` and `FuncTupleElementRef` |
| `! - ~` (unary) | `nt ng co` | 1, 1, 1 | all three are `Expr(Expr)`, so mutually swappable |
| `min` / `max` | — | 3, 3 | plain functions, fully symmetric |

### What it does not overload

| C++ operator | Itanium | symbols |
|---|---|---|
| `++` / `--` | `pp` / `mm` | **0** |
| `%=` | `rM` | **0** |
| `&=` `\|=` `^=` | `aN` `oR` `eO` | **0** |
| `<<=` `>>=` | `lS` `rS` | **0** |

So Mull's `cxx_post_inc_to_post_dec`, `cxx_pre_inc_to_pre_dec`,
`cxx_post_dec_to_post_inc`, `cxx_pre_dec_to_pre_inc`,
`cxx_rem_assign_to_div_assign`, `cxx_and_assign_to_or_assign`,
`cxx_or_assign_to_and_assign`, `cxx_xor_assign_to_or_assign`,
`cxx_lshift_assign_to_rshift_assign` and `cxx_rshift_assign_to_lshift_assign`
have **no Halide analogue at all** -- not a gap in this work, an absence in the
DSL's API.

Mull's `cxx_bitwise_not_to_noop`, `cxx_minus_to_noop` and `cxx_remove_negation`
replace an expression with its operand. `irm::IRMutation` can only redirect a
call to another function of the same type, so a removal is not expressible on
this route. The three unary operators are swapped against each other instead,
which reaches the same call sites with a same-signature mutation.
`cxx_assign_const` / `cxx_init_const` mutate constants rather than calls and
have no call-swap form.

### Implemented operators and their mapping counts

| operator | swap | mappings |
|---|---|---|
| `Halide_lt_to_ge` | < to >= | 1 |
| `Halide_lt_to_le` | < to <= | 3 |
| `Halide_le_to_gt` | <= to > | 3 |
| `Halide_le_to_lt` | <= to < | 3 |
| `Halide_gt_to_ge` | > to >= | 1 |
| `Halide_gt_to_le` | > to <= | 3 |
| `Halide_ge_to_gt` | >= to > | 1 |
| `Halide_ge_to_lt` | >= to < | 1 |
| `Halide_eq_to_ne` | == to != | 3 |
| `Halide_ne_to_eq` | != to == | 3 |
| `Halide_logical_and_to_or` | && to || | 3 |
| `Halide_logical_or_to_and` | || to && | 3 |
| `Halide_and_to_or` | & to | | 3 |
| `Halide_or_to_and` | | to & | 3 |
| `Halide_xor_to_or` | ^ to | | 3 |
| `Halide_lshift_to_rshift` | << to >> | 2 |
| `Halide_rshift_to_lshift` | >> to << | 2 |
| `Halide_rem_to_div` | % to / | 5 |
| `Halide_add_assign_to_sub_assign` | += to -= | 5 |
| `Halide_sub_assign_to_add_assign` | -= to += | 5 |
| `Halide_mul_assign_to_div_assign` | *= to /= | 5 |
| `Halide_div_assign_to_mul_assign` | /= to *= | 5 |
| `Halide_not_to_negate` | ! to unary - | 1 |
| `Halide_not_to_bitwise_not` | ! to ~ | 1 |
| `Halide_negate_to_not` | unary - to ! | 1 |
| `Halide_negate_to_bitwise_not` | unary - to ~ | 1 |
| `Halide_bitwise_not_to_not` | ~ to ! | 1 |
| `Halide_bitwise_not_to_negate` | ~ to unary - | 1 |
| `Halide_min_to_max` | min to max | 3 |
| `Halide_max_to_min` | max to min | 3 |

A swap is emitted only where both overloads share a parameter encoding, which
is why the counts differ. `lt_to_ge` gets 1 rather than 3 because `operator>=`
takes `const Expr &` for its mixed forms; `lshift_to_rshift` gets 2 rather than
31 because `operator>>` has none of `<<`'s `std::ostream` overloads.
