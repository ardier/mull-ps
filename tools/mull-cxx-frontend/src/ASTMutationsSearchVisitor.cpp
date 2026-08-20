#include "ASTMutationsSearchVisitor.h"

#include "MutationMap.h"

#include "mull/AST/ASTConstants.h"
#include "mull/AST/MullClangCompatibility.h"

#include <clang/AST/Decl.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Lex/Lexer.h>

namespace mull {
namespace cxx {

std::vector<std::unique_ptr<ASTMutationPoint>> &ASTMutationsSearchVisitor::getAstMutations() {
  return astMutations;
}

bool ASTMutationsSearchVisitor::VisitFunctionDecl(clang::FunctionDecl *FD) {
  /// No-op. Useful for debugging.
  return true;
}

bool ASTMutationsSearchVisitor::VisitUnaryOperator(clang::UnaryOperator *unaryOperator) {
  if (unaryOperator->getOpcode() == clang::UnaryOperator::Opcode::UO_PostDec &&
      mutationMap.isValidMutation(mull::MutatorKind::CXX_PostDecToPostInc)) {
    std::unique_ptr<UnaryOperatorOpcodeMutation> mutator =
        std::make_unique<UnaryOperatorOpcodeMutation>(unaryOperator,
                                                      clang::UnaryOperator::Opcode::UO_PostInc);
    recordMutationPoint(mull::MutatorKind::CXX_PostDecToPostInc,
                        std::move(mutator),
                        unaryOperator,
                        unaryOperator->getOperatorLoc(),
                        false);
    return true;
  }

  if (unaryOperator->getOpcode() == clang::UnaryOperator::Opcode::UO_PostInc &&
      mutationMap.isValidMutation(mull::MutatorKind::CXX_PostIncToPostDec)) {
    std::unique_ptr<UnaryOperatorOpcodeMutation> mutator =
        std::make_unique<UnaryOperatorOpcodeMutation>(unaryOperator,
                                                      clang::UnaryOperator::Opcode::UO_PostDec);
    recordMutationPoint(mull::MutatorKind::CXX_PostIncToPostDec,
                        std::move(mutator),
                        unaryOperator,
                        unaryOperator->getOperatorLoc(),
                        false);
    return true;
  }

  if (unaryOperator->getOpcode() == clang::UnaryOperator::Opcode::UO_PreInc &&
      mutationMap.isValidMutation(mull::MutatorKind::CXX_PreIncToPreDec)) {
    std::unique_ptr<UnaryOperatorOpcodeMutation> mutator =
        std::make_unique<UnaryOperatorOpcodeMutation>(unaryOperator,
                                                      clang::UnaryOperator::Opcode::UO_PreDec);
    recordMutationPoint(mull::MutatorKind::CXX_PreIncToPreDec,
                        std::move(mutator),
                        unaryOperator,
                        unaryOperator->getOperatorLoc(),
                        false);
    return true;
  }

  if (unaryOperator->getOpcode() == clang::UnaryOperator::Opcode::UO_PreDec &&
      mutationMap.isValidMutation(mull::MutatorKind::CXX_PreDecToPreInc)) {
    std::unique_ptr<UnaryOperatorOpcodeMutation> mutator =
        std::make_unique<UnaryOperatorOpcodeMutation>(unaryOperator,
                                                      clang::UnaryOperator::Opcode::UO_PreInc);
    recordMutationPoint(mull::MutatorKind::CXX_PreDecToPreInc,
                        std::move(mutator),
                        unaryOperator,
                        unaryOperator->getOperatorLoc(),
                        false);
    return true;
  }

  if (unaryOperator->getOpcode() == clang::UnaryOperator::Opcode::UO_LNot &&
      mutationMap.isValidMutation(mull::MutatorKind::CXX_RemoveNegation)) {
    std::unique_ptr<UnaryOperatorRemovalMutation> mutator =
        std::make_unique<UnaryOperatorRemovalMutation>(unaryOperator);
    recordMutationPoint(mull::MutatorKind::CXX_RemoveNegation,
                        std::move(mutator),
                        unaryOperator,
                        unaryOperator->getOperatorLoc(),
                        false);
    return true;
  }

  if (unaryOperator->getOpcode() == clang::UnaryOperator::Opcode::UO_Minus &&
      mutationMap.isValidMutation(mull::MutatorKind::CXX_UnaryMinusToNoop)) {
    std::unique_ptr<UnaryOperatorRemovalMutation> mutator =
        std::make_unique<UnaryOperatorRemovalMutation>(unaryOperator);
    recordMutationPoint(mull::MutatorKind::CXX_UnaryMinusToNoop,
                        std::move(mutator),
                        unaryOperator,
                        unaryOperator->getOperatorLoc(),
                        false);
    return true;
  }

  if (unaryOperator->getOpcode() == clang::UnaryOperator::Opcode::UO_Not &&
      mutationMap.isValidMutation(mull::MutatorKind::CXX_BitwiseNotToNoop)) {
    std::unique_ptr<UnaryOperatorRemovalMutation> mutator =
        std::make_unique<UnaryOperatorRemovalMutation>(unaryOperator);
    recordMutationPoint(mull::MutatorKind::CXX_BitwiseNotToNoop,
                        std::move(mutator),
                        unaryOperator,
                        unaryOperator->getOperatorLoc(),
                        false);
    return true;
  }
  return true;
}

bool ASTMutationsSearchVisitor::VisitBinaryOperator(clang::BinaryOperator *binaryOperator) {
  if (binaryOperator->getOpcode() == clang::BinaryOperator::Opcode::BO_Assign &&
      mutationMap.isValidMutation(mull::MutatorKind::CXX_AssignConst)) {
    if (binaryOperator->getRHS()->getType() == context.IntTy ||
        binaryOperator->getRHS()->getType() == context.FloatTy ||
        binaryOperator->getRHS()->getType() == context.DoubleTy) {
      std::unique_ptr<ReplaceNumericAssignmentMutation> mutator =
          std::make_unique<ReplaceNumericAssignmentMutation>(binaryOperator);
      recordMutationPoint(mull::MutatorKind::CXX_AssignConst,
                          std::move(mutator),
                          binaryOperator,
                          binaryOperator->getOperatorLoc(),
                          true);
    }
    return true;
  }

  for (const std::tuple<clang::BinaryOperator::Opcode,
                        mull::MutatorKind,
                        clang::BinaryOperator::Opcode> &mutation : mull::BinaryMutations) {
    if (binaryOperator->getOpcode() == std::get<0>(mutation) &&
        mutationMap.isValidMutation(std::get<1>(mutation))) {
      std::unique_ptr<BinaryMutation> binaryMutator =
          std::make_unique<BinaryMutation>(std::get<2>(mutation));
      recordMutationPoint(std::get<1>(mutation),
                          std::move(binaryMutator),
                          binaryOperator,
                          binaryOperator->getOperatorLoc(),
                          false);
    }
  }
  return true;
}

bool ASTMutationsSearchVisitor::VisitCallExpr(clang::CallExpr *callExpr) {
  if (callExpr->getType() == context.VoidTy &&
      mutationMap.isValidMutation(mull::MutatorKind::CXX_RemoveVoidCall)) {
    std::unique_ptr<RemoveVoidMutation> removeVoidMutator = std::make_unique<RemoveVoidMutation>();
    recordMutationPoint(mull::MutatorKind::CXX_RemoveVoidCall,
                        std::move(removeVoidMutator),
                        callExpr,
                        ClangCompatibilityStmtGetBeginLoc(*callExpr),
                        true);
  }

  if (callExpr->getType() == context.IntTy &&
      mutationMap.isValidMutation(mull::MutatorKind::CXX_ReplaceScalarCall)) {
    std::unique_ptr<ReplaceScalarCallMutation> mutator =
        std::make_unique<ReplaceScalarCallMutation>(callExpr);
    recordMutationPoint(mull::MutatorKind::CXX_ReplaceScalarCall,
                        std::move(mutator),
                        callExpr,
                        ClangCompatibilityStmtGetBeginLoc(*callExpr),
                        true);
  }

  visitHalideBoundaryConditionsCall(callExpr);

  return true;
}

/// The Halide::BoundaryConditions members that share an overload set and can
/// therefore be swapped for one another at a call site. constant_exterior is
/// excluded on purpose: it takes an additional value argument.
static const struct {
  const char *source;
  const char *target;
  mull::MutatorKind mutatorKind;
} HalideBoundaryConditionSwaps[] = {
  { "repeat_edge", "repeat_image", mull::MutatorKind::Halide_BC_RepeatEdgeToRepeatImage },
  { "repeat_edge", "mirror_image", mull::MutatorKind::Halide_BC_RepeatEdgeToMirrorImage },
  { "repeat_edge", "mirror_interior", mull::MutatorKind::Halide_BC_RepeatEdgeToMirrorInterior },
  { "repeat_image", "repeat_edge", mull::MutatorKind::Halide_BC_RepeatImageToRepeatEdge },
  { "repeat_image", "mirror_image", mull::MutatorKind::Halide_BC_RepeatImageToMirrorImage },
  { "repeat_image", "mirror_interior", mull::MutatorKind::Halide_BC_RepeatImageToMirrorInterior },
  { "mirror_image", "repeat_edge", mull::MutatorKind::Halide_BC_MirrorImageToRepeatEdge },
  { "mirror_image", "repeat_image", mull::MutatorKind::Halide_BC_MirrorImageToRepeatImage },
  { "mirror_image", "mirror_interior", mull::MutatorKind::Halide_BC_MirrorImageToMirrorInterior },
  { "mirror_interior", "repeat_edge", mull::MutatorKind::Halide_BC_MirrorInteriorToRepeatEdge },
  { "mirror_interior", "repeat_image", mull::MutatorKind::Halide_BC_MirrorInteriorToRepeatImage },
  { "mirror_interior", "mirror_image", mull::MutatorKind::Halide_BC_MirrorInteriorToMirrorImage },
};

/// True when `declContext` is exactly the namespace Halide::BoundaryConditions.
/// Checking the namespace chain rather than a printed qualified name keeps
/// Halide::BoundaryConditions::Internal and any same-named user namespace out.
static bool isHalideBoundaryConditionsNamespace(const clang::DeclContext *declContext) {
  const auto *boundaryConditions = clang::dyn_cast_or_null<clang::NamespaceDecl>(declContext);
  if (boundaryConditions == nullptr || boundaryConditions->getName() != "BoundaryConditions") {
    return false;
  }
  const auto *halide =
      clang::dyn_cast_or_null<clang::NamespaceDecl>(boundaryConditions->getParent());
  if (halide == nullptr || halide->getName() != "Halide") {
    return false;
  }
  return halide->getParent() != nullptr && halide->getParent()->isTranslationUnit();
}

void ASTMutationsSearchVisitor::visitHalideBoundaryConditionsCall(clang::CallExpr *callExpr) {
  /// Only fully resolved calls are mutated. Inside the uninstantiated bodies of
  /// Halide's own repeat_edge<T>/... templates the inner call is type-dependent
  /// and has no direct callee, which is precisely what keeps this operator out
  /// of Halide's headers: every mutation point lands on a real call written in
  /// the generator's own source.
  const clang::FunctionDecl *callee = callExpr->getDirectCallee();
  if (callee == nullptr) {
    return;
  }
  if (!isHalideBoundaryConditionsNamespace(callee->getDeclContext())) {
    return;
  }
  /// Halide's Func-like overloads are thin templates forwarding to the
  /// Func-taking overload of the same boundary condition. Those forwarding
  /// calls are Halide's own implementation, not a boundary condition chosen by
  /// the code under test: mutating one rewrites a header body shared by every
  /// call site instead of the call site itself. Only calls made from outside
  /// the namespace are mutated, which is what keeps every mutation point in
  /// the source that actually picked a boundary condition.
  if (enclosingFunction != nullptr &&
      isHalideBoundaryConditionsNamespace(enclosingFunction->getDeclContext())) {
    return;
  }

  const std::string calleeName = callee->getDeclName().getAsString();
  for (const auto &swap : HalideBoundaryConditionSwaps) {
    if (calleeName != swap.source) {
      continue;
    }
    if (!mutationMap.isValidMutation(swap.mutatorKind)) {
      continue;
    }
    /// Reject the swap up front when the replacement is not even declared, so
    /// that no mutation point is reported that cannot be carried out.
    const clang::DeclarationName targetName(&context.Idents.get(swap.target));
    if (callee->getDeclContext()->lookup(targetName).empty()) {
      continue;
    }
    std::unique_ptr<HalideCalleeSwapMutation> mutator =
        std::make_unique<HalideCalleeSwapMutation>(callExpr, swap.target);
    recordMutationPoint(swap.mutatorKind,
                        std::move(mutator),
                        callExpr,
                        ClangCompatibilityStmtGetBeginLoc(*callExpr),
                        true);
  }
}

bool ASTMutationsSearchVisitor::VisitVarDecl(clang::VarDecl *D) {
  /// Function parameter declarations are not mutated.
  if (clang::dyn_cast_or_null<clang::ParmVarDecl>(D)) {
    return true;
  }

  if (mutationMap.isValidMutation(mull::MutatorKind::CXX_InitConst) &&
      (D->getType() == context.IntTy || D->getType() == context.FloatTy ||
       D->getType() == context.DoubleTy)) {
    std::unique_ptr<ReplaceNumericInitAssignmentMutation> mutator =
        std::make_unique<ReplaceNumericInitAssignmentMutation>(D);
    recordMutationPoint(mull::MutatorKind::CXX_InitConst,
                        std::move(mutator),
                        D->getInit(),
                        D->getInit()->getExprLoc(),
                        true);
    return true;
  }
  return true;
}

void ASTMutationsSearchVisitor::recordMutationPoint(mull::MutatorKind mutatorKind,
                                                    std::unique_ptr<ASTMutation> mutation,
                                                    clang::Stmt *stmt,
                                                    clang::SourceLocation mutationLocation,
                                                    bool locationIsExpression) {
  if (sourceManager.isInSystemHeader(mutationLocation)) {
    return;
  }
  if (sourceManager.isInSystemMacro(mutationLocation)) {
    return;
  }

  const std::pair<clang::SourceLocation, clang::SourceLocation> mutationLocationPair =
      getBeginEndMutationLocation(stmt, mutationLocation, locationIsExpression);
  const clang::SourceLocation &mutationBeginLocation = mutationLocationPair.first;
  const clang::SourceLocation &mutationEndLocation = mutationLocationPair.second;

  std::string sourceFilePath = sourceManager.getFilename(mutationBeginLocation).str();

  if (sourceFilePath.empty()) {
    llvm::errs() << "ASTMutationsSearchVisitor: Not implemented: A mutation location has no source "
                    "file path. Parent AST statement dump:\n";
    stmt->dump();
    return;
  }
  if (sourceFilePath.find("include/gtest") != std::string::npos) {
    return;
  }

  int beginLine = sourceManager.getExpansionLineNumber(mutationBeginLocation, nullptr);
  int beginColumn = sourceManager.getExpansionColumnNumber(mutationBeginLocation);

  int endLine = sourceManager.getExpansionLineNumber(mutationEndLocation, nullptr);
  int endColumn = sourceManager.getExpansionColumnNumber(mutationEndLocation);

  std::unique_ptr<ASTMutationPoint> astMutation =
      std::make_unique<ASTMutationPoint>(std::move(mutation),
                                         mutatorKind,
                                         mutationMap.getIdentifier(mutatorKind),
                                         stmt,
                                         sourceFilePath,
                                         beginLine,
                                         beginColumn,
                                         endLine,
                                         endColumn);

  llvm::outs() << "Recording mutation point: " << astMutation->mutationIdentifier
               << " (end: " << std::to_string(endLine) << ":" << std::to_string(endColumn) << ")\n";
  astMutations.emplace_back(std::move(astMutation));
}

std::pair<clang::SourceLocation, clang::SourceLocation>
ASTMutationsSearchVisitor::getBeginEndMutationLocation(clang::Stmt *stmt,
                                                       clang::SourceLocation mutationLocation,
                                                       bool locationIsExpression) {
  /// There are two known types of mutated expressions:
  /// 1) Remove-Void, CallExpr example: its mutation location and its getBegin() are the same, so
  ///    [getBegin(), getEnd()] is what we need to get the mutation's AST information.
  /// 2) Binary Mutation, BinaryOperator example: its mutation location is
  ///    BinaryOperator's getOperatorLoc(), i.e. "+", while the
  ///    [getBegin(), getEnd()] range is the whole "a + b" expression.
  ///    In this case the mutation's AST information is obtained as:
  ///    [getOperatorLoc(), "end of the token of getOperatorLoc()"]
  clang::SourceLocation endLocation =
      locationIsExpression ? stmt->getSourceRange().getEnd() : mutationLocation;

  clang::SourceLocation updatedMutationLocation;
  if (!mutationLocation.isMacroID()) {
    updatedMutationLocation = mutationLocation;
  } else {
    updatedMutationLocation = sourceManager.getSpellingLoc(mutationLocation);
    endLocation = sourceManager.getSpellingLoc(endLocation);
  }

  /// Clang AST: how to get more precise debug information in certain cases?
  /// http://clang-developers.42468.n3.nabble.com/Clang-AST-how-to-get-more-precise-debug-information-in-certain-cases-td4065195.html
  /// https://stackoverflow.com/questions/11083066/getting-the-source-behind-clangs-ast
  clang::SourceLocation sourceLocationEndActual =
      clang::Lexer::getLocForEndOfToken(endLocation, 0, sourceManager, context.getLangOpts());

  return std::make_pair(updatedMutationLocation, sourceLocationEndActual);
}

} // namespace cxx
} // namespace mull
