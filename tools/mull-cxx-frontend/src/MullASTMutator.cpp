#include <cassert>
#include <cstdlib>
#include <iostream>

#include <llvm/Support/raw_ostream.h>

#include "MullASTMutator.h"

#include <clang/AST/ASTContext.h>
#include <clang/AST/Expr.h>
#include <clang/Sema/Lookup.h>
#include <clang/Sema/Sema.h>
#include <llvm/ADT/SmallVector.h>

namespace mull {
namespace cxx {

void MullASTMutator::instrumentTranslationUnit() {
  instrumentation.instrumentTranslationUnit();
}

void MullASTMutator::performUnaryOperatorOpcodeMutation(
    ASTMutationPoint &mutation, UnaryOperatorOpcodeMutation &unaryOperatorOpcodeMutator) {
  clang::UnaryOperator *oldUnaryOperator =
      clang::dyn_cast_or_null<clang::UnaryOperator>(mutation.mutableStmt);
  clang::UnaryOperator *newUnaryOperator =
      factory.createUnaryOperator(unaryOperatorOpcodeMutator.replacementOpCode,
                                  oldUnaryOperator->getSubExpr(),
                                  oldUnaryOperator->getType(),
                                  oldUnaryOperator->getValueKind());

  clangAstMutator.replaceExpression(
      oldUnaryOperator, newUnaryOperator, mutation.mutationIdentifier);
  instrumentation.addMutantStringDefinition(mutation.mutationBinaryRecord,
                                            static_cast<int>(mutation.mutationType),
                                            mutation.beginLine,
                                            mutation.beginColumn);
}

void MullASTMutator::performUnaryOperatorRemovalMutation(
    ASTMutationPoint &mutation, UnaryOperatorRemovalMutation &unaryNotToNoopMutator) {

  clangAstMutator.replaceExpression(unaryNotToNoopMutator.unaryOperator,
                                    unaryNotToNoopMutator.unaryOperator->getSubExpr(),
                                    mutation.mutationIdentifier);

  instrumentation.addMutantStringDefinition(mutation.mutationBinaryRecord,
                                            static_cast<int>(mutation.mutationType),
                                            mutation.beginLine,
                                            mutation.beginColumn);
}

void MullASTMutator::performBinaryMutation(ASTMutationPoint &mutation,
                                           BinaryMutation &binaryMutator) {
  clang::BinaryOperator *oldBinaryOperator =
      clang::dyn_cast<clang::BinaryOperator>(mutation.mutableStmt);

  clang::BinaryOperator *newBinaryOperator;

  if (clang::CompoundAssignOperator *compoundAssignOperator =
          clang::dyn_cast_or_null<clang::CompoundAssignOperator>(oldBinaryOperator)) {
    newBinaryOperator =
        factory.createCompoundAssignOperator(binaryMutator.replacementOpCode,
                                             oldBinaryOperator->getLHS(),
                                             oldBinaryOperator->getRHS(),
                                             oldBinaryOperator->getType(),
                                             oldBinaryOperator->getValueKind(),
                                             compoundAssignOperator->getComputationLHSType(),
                                             compoundAssignOperator->getComputationResultType());
  } else {
    newBinaryOperator = factory.createBinaryOperator(binaryMutator.replacementOpCode,
                                                     oldBinaryOperator->getLHS(),
                                                     oldBinaryOperator->getRHS(),
                                                     oldBinaryOperator->getType(),
                                                     oldBinaryOperator->getValueKind());
  }

  clangAstMutator.replaceExpression(
      oldBinaryOperator, newBinaryOperator, mutation.mutationIdentifier);
  instrumentation.addMutantStringDefinition(mutation.mutationBinaryRecord,
                                            static_cast<int>(mutation.mutationType),
                                            mutation.beginLine,
                                            mutation.beginColumn);
}

void MullASTMutator::performRemoveVoidMutation(ASTMutationPoint &mutation,
                                               RemoveVoidMutation &removeVoidMutator) {
  clang::CallExpr *callExpr = clang::dyn_cast<clang::CallExpr>(mutation.mutableStmt);
  clangAstMutator.replaceStatement(callExpr, nullptr, mutation.mutationIdentifier);

  instrumentation.addMutantStringDefinition(mutation.mutationBinaryRecord,
                                            static_cast<int>(mutation.mutationType),
                                            mutation.beginLine,
                                            mutation.beginColumn);
}

void MullASTMutator::performReplaceScalarMutation(
    ASTMutationPoint &mutation, ReplaceScalarCallMutation &replaceScalarCallMutator) {
  clang::CallExpr *callExpr = replaceScalarCallMutator.callExpr;

  clang::Expr *replacementLiteral = nullptr;
  if (callExpr->getType() == context.IntTy) {
    replacementLiteral = factory.createIntegerLiteral(42);
  } else if (callExpr->getType() == context.FloatTy) {
    replacementLiteral = factory.createFloatLiteral(42.f);
  } else if (callExpr->getType() == context.DoubleTy) {
    replacementLiteral = factory.createFloatLiteral(42.0);
  } else {
    notImplemented();
  }

  clangAstMutator.replaceExpression(callExpr, replacementLiteral, mutation.mutationIdentifier);
  instrumentation.addMutantStringDefinition(mutation.mutationBinaryRecord,
                                            static_cast<int>(mutation.mutationType),
                                            mutation.beginLine,
                                            mutation.beginColumn);
}

void MullASTMutator::performReplaceNumericAssignmentMutation(
    ASTMutationPoint &mutation, ReplaceNumericAssignmentMutation &replaceNumericAssignmentMutator) {

  clang::Expr *oldAssignedExpr = replaceNumericAssignmentMutator.assignmentBinaryOperator->getRHS();

  clang::Expr *replacementLiteral = nullptr;
  if (oldAssignedExpr->getType() == context.IntTy) {
    replacementLiteral = factory.createIntegerLiteral(42);
  } else if (oldAssignedExpr->getType() == context.FloatTy) {
    replacementLiteral = factory.createFloatLiteral(42.f);
  } else if (oldAssignedExpr->getType() == context.DoubleTy) {
    replacementLiteral = factory.createFloatLiteral(42.0);
  } else {
    notImplemented();
  }

  clangAstMutator.replaceExpression(
      replaceNumericAssignmentMutator.assignmentBinaryOperator->getRHS(),
      replacementLiteral,
      mutation.mutationIdentifier);
  instrumentation.addMutantStringDefinition(mutation.mutationBinaryRecord,
                                            static_cast<int>(mutation.mutationType),
                                            mutation.beginLine,
                                            mutation.beginColumn);
}

void MullASTMutator::performReplaceNumericInitAssignmentMutation(
    ASTMutationPoint &mutation,
    ReplaceNumericInitAssignmentMutation &replaceNumericInitAssignmentMutator) {
  assert(replaceNumericInitAssignmentMutator.varDecl->getKind() == clang::VarDecl::Kind::Var);

  clang::Expr *oldAssignedExpr = replaceNumericInitAssignmentMutator.varDecl->getInit();

  clang::Expr *replacementLiteral = nullptr;
  if (oldAssignedExpr->getType() == context.IntTy) {
    replacementLiteral = factory.createIntegerLiteral(42);
  } else if (oldAssignedExpr->getType() == context.FloatTy) {
    replacementLiteral = factory.createFloatLiteral(42.f);
  } else if (oldAssignedExpr->getType() == context.DoubleTy) {
    replacementLiteral = factory.createFloatLiteral(42.0);
  } else {
    notImplemented();
  }

  clangAstMutator.replaceExpression(
      oldAssignedExpr, replacementLiteral, mutation.mutationIdentifier);
  instrumentation.addMutantStringDefinition(mutation.mutationBinaryRecord,
                                            static_cast<int>(mutation.mutationType),
                                            mutation.beginLine,
                                            mutation.beginColumn);
}

clang::Expr *MullASTMutator::buildCalleeSwappedCall(clang::CallExpr *callExpr,
                                                    const std::string &newCalleeName) {
  clang::FunctionDecl *oldCallee = callExpr->getDirectCallee();
  assert(oldCallee && "callee swap requires a resolved callee");

  /// Look the replacement up in the same DeclContext the original callee lives
  /// in, so the swap can never escape Halide::BoundaryConditions.
  clang::DeclarationNameInfo nameInfo(&context.Idents.get(newCalleeName), callExpr->getBeginLoc());
  clang::LookupResult lookupResult(sema, nameInfo, clang::Sema::LookupOrdinaryName);
  lookupResult.suppressDiagnostics();
  if (!sema.LookupQualifiedName(lookupResult, oldCallee->getDeclContext())) {
    return nullptr;
  }

  clang::CXXScopeSpec scopeSpec;
  clang::ExprResult callee =
      sema.BuildDeclarationNameExpr(scopeSpec, lookupResult, /*NeedsADL=*/false);
  if (callee.isInvalid()) {
    return nullptr;
  }

  /// The arguments are reused as-is. Every function in the family takes its
  /// arguments by const reference, so no argument conversion of the original
  /// call can be invalidated by the swap; Sema still re-runs overload
  /// resolution (and template argument deduction for the Func-like overloads)
  /// against them.
  llvm::SmallVector<clang::Expr *, 4> arguments(callExpr->arguments().begin(),
                                                callExpr->arguments().end());

  clang::ExprResult newCall = sema.BuildCallExpr(/*Scope=*/nullptr,
                                                 callee.get(),
                                                 callExpr->getBeginLoc(),
                                                 arguments,
                                                 callExpr->getRParenLoc());
  if (newCall.isInvalid()) {
    return nullptr;
  }
  return newCall.get();
}

void MullASTMutator::performHalideCalleeSwapMutation(
    ASTMutationPoint &mutation, HalideCalleeSwapMutation &halideCalleeSwapMutator) {
  clang::CallExpr *oldCall = halideCalleeSwapMutator.callExpr;
  clang::Expr *newCall =
      buildCalleeSwappedCall(oldCall, halideCalleeSwapMutator.replacementCalleeName);
  if (newCall == nullptr) {
    llvm::errs() << "mull-cxx-frontend: could not build a call to '"
                 << halideCalleeSwapMutator.replacementCalleeName
                 << "', skipping mutation: " << mutation.mutationIdentifier << "\n";
    return;
  }

  clangAstMutator.replaceExpression(oldCall, newCall, mutation.mutationIdentifier);
  instrumentation.addMutantStringDefinition(mutation.mutationBinaryRecord,
                                            static_cast<int>(mutation.mutationType),
                                            mutation.beginLine,
                                            mutation.beginColumn);
}

[[noreturn]] void MullASTMutator::notImplemented() noexcept {
    std::cerr << "Not implemented\n";
    std::abort();
}

} // namespace cxx
} // namespace mull
