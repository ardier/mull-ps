#include "mull/Mutators/CXX/HalideSpecialCallMutators.h"

using namespace mull;
using namespace mull::cxx;

/// Performed on the Clang AST by mull-cxx-frontend, so there is no
/// irm::IRMutation behind them; see HalideSpecialCallMutators.h.
static std::vector<std::unique_ptr<irm::IRMutation>> noIRMutations() {
  return {};
}

std::string HalideSelectSwapBranches::ID() {
  return "Halide_select_swap_branches";
}

HalideSelectSwapBranches::HalideSelectSwapBranches()
    : TrivialCXXMutator(noIRMutations(),
                        MutatorKind::Halide_SelectSwapBranches,
                        HalideSelectSwapBranches::ID(),
                        "Swaps the two values of a Halide::select",
                        "select(c, b, a)",
                        "Swapped the two values of a Halide::select") {}

std::string HalideClampSwapBounds::ID() {
  return "Halide_clamp_swap_bounds";
}

HalideClampSwapBounds::HalideClampSwapBounds()
    : TrivialCXXMutator(noIRMutations(),
                        MutatorKind::Halide_ClampSwapBounds,
                        HalideClampSwapBounds::ID(),
                        "Swaps the two bounds of a Halide::clamp",
                        "clamp(x, hi, lo)",
                        "Swapped the two bounds of a Halide::clamp") {}
