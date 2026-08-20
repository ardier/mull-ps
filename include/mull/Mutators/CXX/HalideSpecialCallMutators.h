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

/// Halide::select(c, a, b) evaluates both values and picks one; the
/// Halide::Internal::Call intrinsic if_then_else evaluates only the branch it
/// takes. Rewriting one into the other moves the expression along the
/// eager/lazy axis, which has no analogue in C++ mutation: the host language's
/// ?: is already lazy, and there is no operator to swap it with.
///
/// One-directional. There is no public Halide::if_then_else free function --
/// the intrinsic is reachable only as Internal::Call::if_then_else -- so no
/// user code contains a call to mutate in the other direction.
class HalideSelectToIfThenElse : public TrivialCXXMutator {
public:
  static std::string ID();
  HalideSelectToIfThenElse();
};

} // namespace cxx
} // namespace mull
