#pragma once

#include "mull/Mutators/CXX/TrivialCXXMutator.h"

namespace mull {
namespace cxx {

/// Halide "special function call" operators.
///
/// These target Halide API calls whose meaning lives in the arguments rather
/// than in which function is called, so they are operand rewrites, not callee
/// swaps. That is what puts them on the Clang AST route: irm::IRMutation's
/// single-instruction interface cannot express reordering a call's arguments.
/// Neither select nor clamp leaves an out-of-line symbol anyway -- both are
/// header-inline at every call site in the corpus -- so the IR route has
/// nothing to redirect. Like the BoundaryConditions family, these carry no
/// low-level IR mutation and exist so the identifiers, the
/// halide_special_calls group and the reporters resolve.

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

/// Rewrites an eager Halide::select, which evaluates both values and then
/// picks one, into the lazy Internal::Call::if_then_else intrinsic, which
/// evaluates only the branch it takes. One-directional by necessity: there is
/// no public Halide::if_then_else free function, so no user code contains a
/// call to mutate the other way.
class HalideSelectToIfThenElse : public TrivialCXXMutator {
public:
  static std::string ID();
  HalideSelectToIfThenElse();
};

} // namespace cxx
} // namespace mull
