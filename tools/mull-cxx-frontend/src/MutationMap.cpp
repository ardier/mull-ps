#include "MutationMap.h"

#include <cassert>
#include <string>
#include <utility>
#include <vector>

namespace mull {
namespace cxx {

static const std::vector<MutationIdentifier> MUTATIONS_MAP({
    { "cxx_add_to_sub", mull::MutatorKind::CXX_AddToSub },
    { "cxx_sub_to_add", mull::MutatorKind::CXX_SubToAdd },
    { "cxx_mul_to_div", mull::MutatorKind::CXX_MulToDiv },
    { "cxx_div_to_mul", mull::MutatorKind::CXX_DivToMul },
    { "cxx_rem_to_div", mull::MutatorKind::CXX_RemToDiv },
    { "cxx_eq_to_ne", mull::MutatorKind::CXX_EqualToNotEqual },
    { "cxx_ne_to_eq", mull::MutatorKind::CXX_NotEqualToEqual },
    { "cxx_ge_to_gt", mull::MutatorKind::CXX_GreaterOrEqualToGreaterThan },
    { "cxx_ge_to_lt", mull::MutatorKind::CXX_GreaterOrEqualToLessThan },
    { "cxx_gt_to_ge", mull::MutatorKind::CXX_GreaterThanToGreaterOrEqual },
    { "cxx_gt_to_le", mull::MutatorKind::CXX_GreaterThanToLessOrEqual },
    { "cxx_le_to_gt", mull::MutatorKind::CXX_LessOrEqualToGreaterThan },
    { "cxx_le_to_lt", mull::MutatorKind::CXX_LessOrEqualToLessThan },
    { "cxx_lt_to_ge", mull::MutatorKind::CXX_LessThanToGreaterOrEqual },
    { "cxx_lt_to_le", mull::MutatorKind::CXX_LessThanToLessOrEqual },
    { "cxx_add_assign_to_sub_assign", mull::MutatorKind::CXX_AddAssignToSubAssign },
    { "cxx_sub_assign_to_add_assign", mull::MutatorKind::CXX_SubAssignToAddAssign },
    { "cxx_mul_assign_to_div_assign", mull::MutatorKind::CXX_MulAssignToDivAssign },
    { "cxx_div_assign_to_mul_assign", mull::MutatorKind::CXX_DivAssignToMulAssign },
    { "cxx_rem_assign_to_div_assign", mull::MutatorKind::CXX_RemAssignToDivAssign },
    { "cxx_and_to_or", mull::MutatorKind::CXX_Bitwise_AndToOr },
    { "cxx_or_to_and", mull::MutatorKind::CXX_Bitwise_OrToAnd },
    { "cxx_xor_to_or", mull::MutatorKind::CXX_Bitwise_XorToOr },
    { "cxx_lshift_to_rshift", mull::MutatorKind::CXX_LShiftToRShift },
    { "cxx_rshift_to_lshift", mull::MutatorKind::CXX_RShiftToLShift },
    { "cxx_and_assign_to_or_assign", mull::MutatorKind::CXX_Bitwise_AndAssignToOrAssign },
    { "cxx_or_assign_to_and_assign", mull::MutatorKind::CXX_Bitwise_OrAssignToAndAssign },
    { "cxx_xor_assign_to_or_assign", mull::MutatorKind::CXX_Bitwise_XorAssignToOrAssign },
    { "cxx_lshift_assign_to_rshift_assign", mull::MutatorKind::CXX_LShiftAssignToRShiftAssign },
    { "cxx_rshift_assign_to_lshift_assign", mull::MutatorKind::CXX_RShiftAssignToLShiftAssign },
    { "cxx_post_inc_to_post_dec", mull::MutatorKind::CXX_PostIncToPostDec },
    { "cxx_post_dec_to_post_inc", mull::MutatorKind::CXX_PostDecToPostInc },
    { "cxx_pre_inc_to_pre_dec", mull::MutatorKind::CXX_PreIncToPreDec },
    { "cxx_pre_dec_to_pre_inc", mull::MutatorKind::CXX_PreDecToPreInc },
    { "cxx_logical_and_to_or", mull::MutatorKind::CXX_Logical_AndToOr },
    { "cxx_logical_or_to_and", mull::MutatorKind::CXX_Logical_OrToAnd },
    { "cxx_remove_void_call", mull::MutatorKind::CXX_RemoveVoidCall },
    { "cxx_minus_to_noop", mull::MutatorKind::CXX_UnaryMinusToNoop },
    { "cxx_bitwise_not_to_noop", mull::MutatorKind::CXX_BitwiseNotToNoop },
    { "cxx_remove_negation", mull::MutatorKind::CXX_RemoveNegation },
    { "cxx_assign_const", mull::MutatorKind::CXX_AssignConst },
    { "cxx_init_const", mull::MutatorKind::CXX_InitConst },
    { "cxx_replace_scalar_call", mull::MutatorKind::CXX_ReplaceScalarCall },

    /// Halide BoundaryConditions family swap. Opt-in only (group
    /// halide_boundary_conditions), hence enabledByDefault = false.
    { "Halide_repeat_edge_to_repeat_image", mull::MutatorKind::Halide_BC_RepeatEdgeToRepeatImage, false },
    { "Halide_repeat_edge_to_mirror_image", mull::MutatorKind::Halide_BC_RepeatEdgeToMirrorImage, false },
    { "Halide_repeat_edge_to_mirror_interior", mull::MutatorKind::Halide_BC_RepeatEdgeToMirrorInterior, false },
    { "Halide_repeat_image_to_repeat_edge", mull::MutatorKind::Halide_BC_RepeatImageToRepeatEdge, false },
    { "Halide_repeat_image_to_mirror_image", mull::MutatorKind::Halide_BC_RepeatImageToMirrorImage, false },
    { "Halide_repeat_image_to_mirror_interior", mull::MutatorKind::Halide_BC_RepeatImageToMirrorInterior, false },
    { "Halide_mirror_image_to_repeat_edge", mull::MutatorKind::Halide_BC_MirrorImageToRepeatEdge, false },
    { "Halide_mirror_image_to_repeat_image", mull::MutatorKind::Halide_BC_MirrorImageToRepeatImage, false },
    { "Halide_mirror_image_to_mirror_interior", mull::MutatorKind::Halide_BC_MirrorImageToMirrorInterior, false },
    { "Halide_mirror_interior_to_repeat_edge", mull::MutatorKind::Halide_BC_MirrorInteriorToRepeatEdge, false },
    { "Halide_mirror_interior_to_repeat_image", mull::MutatorKind::Halide_BC_MirrorInteriorToRepeatImage, false },
    { "Halide_mirror_interior_to_mirror_image", mull::MutatorKind::Halide_BC_MirrorInteriorToMirrorImage, false },

    /// Halide special-function-call argument swaps. Opt-in only (group
    /// halide_special_calls), hence enabledByDefault = false.
    { "Halide_select_swap_branches", mull::MutatorKind::Halide_SelectSwapBranches, false },
    { "Halide_clamp_swap_bounds", mull::MutatorKind::Halide_ClampSwapBounds, false },
});

MutationMap::MutationMap() : usedMutatorSet(), mapKindsToIdentifiers(), mapIdentifiersToKinds() {
  for (const MutationIdentifier &mutationIdentifier : MUTATIONS_MAP) {
    mapKindsToIdentifiers[mutationIdentifier.mutatorKind] = mutationIdentifier.identifier;
    mapIdentifiersToKinds[mutationIdentifier.identifier] = mutationIdentifier.mutatorKind;
  }
  assert(mapKindsToIdentifiers.count(mull::MutatorKind::CXX_AddToSub) > 0);
}

bool MutationMap::isValidMutation(mull::MutatorKind mutatorKind) const {
  return usedMutatorSet.count(mutatorKind) > 0;
}

std::string MutationMap::getIdentifier(mull::MutatorKind mutatorKind) {
  return mapKindsToIdentifiers[mutatorKind];
}

/// Mutator groups this frontend understands by name. mull.yml is shared with
/// mull-ir-frontend and mull-runner, whose groups live in MutatorsFactory; that
/// lives in libmull, which a Clang plugin does not link, so the groups whose
/// members are implemented here are mirrored. Keep in sync with
/// lib/Mutators/MutatorsFactory.cpp.
static const std::vector<std::pair<std::string, std::vector<std::string>>> MUTATION_GROUPS({
    { "halide_boundary_conditions",
      { "Halide_repeat_edge_to_repeat_image",
        "Halide_repeat_edge_to_mirror_image",
        "Halide_repeat_edge_to_mirror_interior",
        "Halide_repeat_image_to_repeat_edge",
        "Halide_repeat_image_to_mirror_image",
        "Halide_repeat_image_to_mirror_interior",
        "Halide_mirror_image_to_repeat_edge",
        "Halide_mirror_image_to_repeat_image",
        "Halide_mirror_image_to_mirror_interior",
        "Halide_mirror_interior_to_repeat_edge",
        "Halide_mirror_interior_to_repeat_image",
        "Halide_mirror_interior_to_mirror_image" } },
    { "halide_special_calls", { "Halide_select_swap_branches", "Halide_clamp_swap_bounds" } },
    /// Group members may themselves be group names: addMutation recurses.
    { "halide_ast", { "halide_boundary_conditions", "halide_special_calls" } },
});

void MutationMap::addMutation(std::string identifier) {
  for (const auto &group : MUTATION_GROUPS) {
    if (group.first != identifier) {
      continue;
    }
    for (const std::string &member : group.second) {
      addMutation(member);
    }
    return;
  }

  /// Identifiers naming a mutator only the IR frontend implements are silently
  /// ignored: both frontends read the same mull.yml.
  auto mutatorKind = mapIdentifiersToKinds.find(identifier);
  if (mutatorKind == mapIdentifiersToKinds.end()) {
    return;
  }
  usedMutatorSet.insert(mutatorKind->second);
}

void MutationMap::setDefaultMutationsIfNotSpecified() {
  if (!usedMutatorSet.empty()) {
    return;
  }
  for (const MutationIdentifier &mutationIdentifier : MUTATIONS_MAP) {
    if (!mutationIdentifier.enabledByDefault) {
      continue;
    }
    usedMutatorSet.insert(mutationIdentifier.mutatorKind);
  }
}

bool MutationMap::needsDeepDeclTraversal() const {
  for (const MutationIdentifier &mutationIdentifier : MUTATIONS_MAP) {
    if (mutationIdentifier.enabledByDefault) {
      continue;
    }
    if (usedMutatorSet.count(mutationIdentifier.mutatorKind) > 0) {
      return true;
    }
  }
  return false;
}

} // namespace cxx
} // namespace mull
