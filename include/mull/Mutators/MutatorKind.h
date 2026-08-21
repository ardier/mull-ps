#pragma once

#include <functional>
#include <string>
#include <unordered_set>
#include <vector>

namespace mull {

enum class MutatorKind {
  InvalidKind,

  NegateMutator,

  CXX_RemoveVoidCall,
  CXX_ReplaceScalarCall,

  CXX_AddToSub,
  CXX_AddAssignToSubAssign,
  CXX_PreIncToPreDec,
  CXX_PostIncToPostDec,

  CXX_SubToAdd,
  CXX_SubAssignToAddAssign,
  CXX_PreDecToPreInc,
  CXX_PostDecToPostInc,

  CXX_MulToDiv,
  CXX_MulAssignToDivAssign,

  CXX_DivToMul,
  CXX_DivAssignToMulAssign,

  CXX_RemToDiv,
  CXX_RemAssignToDivAssign,

  CXX_BitwiseNotToNoop,
  CXX_UnaryMinusToNoop,

  CXX_LShiftToRShift,
  CXX_LShiftAssignToRShiftAssign,

  CXX_RShiftToLShift,
  CXX_RShiftAssignToLShiftAssign,

  CXX_Bitwise_OrToAnd,
  CXX_Bitwise_OrAssignToAndAssign,
  CXX_Bitwise_AndToOr,
  CXX_Bitwise_AndAssignToOrAssign,
  CXX_Bitwise_XorToOr,
  CXX_Bitwise_XorAssignToOrAssign,

  CXX_LessThanToLessOrEqual,
  CXX_LessOrEqualToLessThan,
  CXX_GreaterThanToGreaterOrEqual,
  CXX_GreaterOrEqualToGreaterThan,

  CXX_GreaterThanToLessOrEqual,
  CXX_GreaterOrEqualToLessThan,
  CXX_LessThanToGreaterOrEqual,
  CXX_LessOrEqualToGreaterThan,

  CXX_EqualToNotEqual,
  CXX_NotEqualToEqual,

  CXX_AssignConst,
  CXX_InitConst,

  CXX_RemoveNegation,

  /// Halide-native operators. These mutate calls to Halide's own overloaded
  /// operators and schedule directives, matched by Itanium-mangled callee name
  /// (see libirm's HalideReplacement). They are kept in one contiguous block so
  /// junk detection and reporting can range-check on it.
  Halide_ReplaceHalideAddToMulCall,
  Halide_ReplaceHalideAddToSubCall,
  Halide_ReplaceHalideAddToDivCall,

  Halide_ReplaceHalideSubToAddCall,
  Halide_ReplaceHalideSubToMulCall,
  Halide_ReplaceHalideSubToDivCall,

  Halide_ReplaceHalideMulToAddCall,
  Halide_ReplaceHalideMulToSubCall,
  Halide_ReplaceHalideMulToDivCall,

  Halide_ReplaceHalideDivToMulCall,
  Halide_ReplaceHalideDivToSubCall,
  Halide_ReplaceHalideDivToAddCall,

  /// Schedule directives -- no C++/GPL sibling operator exists for these.
  Halide_ReplaceVectorizeToUnrollCall,
  Halide_ReplaceVectorizeToParallelCall,
  Halide_ReplaceUnrollToVectorizeCall,
  Halide_ReplaceUnrollToParallelCall,
  Halide_ReplaceParallelToVectorizeCall,
  Halide_ReplaceParallelToUnrollCall,
  Halide_ReplaceComputeAtToStoreAtCall,
  Halide_ReplaceStoreAtToComputeAtCall,

  /// Generated IR-route swaps: relational, logical, bitwise, remainder,
  /// compound-assignment, unary and min/max. See HalideGeneratedMutators.def.
#define HALIDE_GEN_MUTATOR(KindName, ClassName, IdString, IrmClass, Description) KindName,
#include "mull/Mutators/CXX/HalideGeneratedMutators.def"
  /// Halide::BoundaryConditions family swap. Unlike the block above, these are
  /// produced by the Clang AST route (mull-cxx-frontend), not by matching a
  /// mangled callee name: the idiomatic entry points are function templates
  /// instantiated per buffer element type, so there is no stable symbol for the
  /// IR route to redirect. Kept contiguous for the same range-check reason.
  Halide_BC_RepeatEdgeToRepeatImage,
  Halide_BC_RepeatEdgeToMirrorImage,
  Halide_BC_RepeatEdgeToMirrorInterior,
  Halide_BC_RepeatImageToRepeatEdge,
  Halide_BC_RepeatImageToMirrorImage,
  Halide_BC_RepeatImageToMirrorInterior,
  Halide_BC_MirrorImageToRepeatEdge,
  Halide_BC_MirrorImageToRepeatImage,
  Halide_BC_MirrorImageToMirrorInterior,
  Halide_BC_MirrorInteriorToRepeatEdge,
  Halide_BC_MirrorInteriorToRepeatImage,
  Halide_BC_MirrorInteriorToMirrorImage,
};

std::string MutationKindToString(MutatorKind mutatorKind);

} // namespace mull

namespace std {

template <> struct hash<mull::MutatorKind> {
  std::size_t operator()(const mull::MutatorKind &k) const {
    return static_cast<std::size_t>(k);
  }
};

} // namespace std

namespace mull {
class MutatorKindSet {
public:
  static MutatorKindSet create(std::vector<MutatorKind> mutators);
  bool includesMutator(mull::MutatorKind mutatorKind) const;

private:
  MutatorKindSet(std::unordered_set<mull::MutatorKind> mutators);
  std::unordered_set<mull::MutatorKind> mutators;
};
} // namespace mull
