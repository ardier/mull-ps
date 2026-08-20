#pragma once

#include "mull/Mutators/CXX/TrivialCXXMutator.h"

namespace mull {
namespace cxx {

/// Halide "special function call" mutators: calls to Halide API functions that
/// carry domain semantics in their argument order.
///
/// Halide::select(condition, true_value, false_value) picks a value per lane
/// and, unlike C's ?:, evaluates both sides -- swapping the two values inverts
/// the choice everywhere the condition is true.
///
/// Halide::clamp(a, min_val, max_val) expands to max(min(a, max_val), min_val),
/// so swapping the two bounds collapses the clamp onto a single bound. Both are
/// operand rewrites rather than callee swaps, which is what puts them on the
/// Clang AST route: irm::IRMutation's single-instruction interface cannot
/// express reordering the arguments of a call.
///
/// Like the BoundaryConditions swaps these are produced exclusively by
/// mull-cxx-frontend and carry no low-level IR mutation. select and clamp are
/// header-inline at every call site in the benchmark corpus and leave no
/// out-of-line symbol at all (docs/halide-mangled-names.md, section 4), so
/// there is nothing for the IR route to redirect.

class HalideSelectSwapBranches : public TrivialCXXMutator {
public:
  static std::string ID();
  HalideSelectSwapBranches();
};

class HalideClampSwapBounds : public TrivialCXXMutator {
public:
  static std::string ID();
  HalideClampSwapBounds();
};

} // namespace cxx
} // namespace mull
