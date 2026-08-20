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

clang::Expr *MullASTMutator::buildArgumentSwappedCall(clang::CallExpr *callExpr,
                                                      unsigned firstArgumentIndex,
                                                      unsigned secondArgumentIndex) {
  assert(firstArgumentIndex < callExpr->getNumArgs());
  assert(secondArgumentIndex < callExpr->getNumArgs());

  llvm::SmallVector<clang::Expr *, 4> arguments(callExpr->arguments().begin(),
                                                callExpr->arguments().end());
  std::swap(arguments[firstArgumentIndex], arguments[secondArgumentIndex]);

  /// The callee expression is reused unchanged, so the swap can never resolve
  /// to a different function than the one that was written.
  clang::ExprResult newCall = sema.BuildCallExpr(/*Scope=*/nullptr,
                                                 callExpr->getCallee(),
                                                 callExpr->getBeginLoc(),
                                                 arguments,
                                                 callExpr->getRParenLoc());
  if (newCall.isInvalid()) {
    return nullptr;
  }
  return newCall.get();
}

void MullASTMutator::performHalideArgumentSwapMutation(
    ASTMutationPoint &mutation, HalideArgumentSwapMutation &halideArgumentSwapMutator) {
  clang::CallExpr *oldCall = halideArgumentSwapMutator.callExpr;
  clang::Expr *newCall = buildArgumentSwappedCall(oldCall,
                                                  halideArgumentSwapMutator.firstArgumentIndex,
                                                  halideArgumentSwapMutator.secondArgumentIndex);
  if (newCall == nullptr) {
    llvm::errs() << "mull-cxx-frontend: could not rebuild the call with swapped arguments, "
                    "skipping mutation: "
                 << mutation.mutationIdentifier << "\n";
    return;
  }

  clangAstMutator.replaceExpression(oldCall, newCall, mutation.mutationIdentifier);
  instrumentation.addMutantStringDefinition(mutation.mutationBinaryRecord,
                                            static_cast<int>(mutation.mutationType),
                                            mutation.beginLine,
                                            mutation.beginColumn);
}

/// Finds a single named member of a DeclContext, ignoring overload sets.
template <typename DeclType>
static DeclType *lookupSingle(clang::DeclContext *declContext, clang::ASTContext &context,
                              llvm::StringRef name) {
  const clang::DeclarationName declarationName(&context.Idents.get(name));
  for (clang::NamedDecl *candidate : declContext->lookup(declarationName)) {
    if (auto *found = clang::dyn_cast<DeclType>(candidate)) {
      return found;
    }
  }
  return nullptr;
}

clang::CXXRecordDecl *MullASTMutator::lookupHalideInternalCall() {
  auto *halide = lookupSingle<clang::NamespaceDecl>(
      context.getTranslationUnitDecl(), context, "Halide");
  if (halide == nullptr) {
    return nullptr;
  }
  auto *internal = lookupSingle<clang::NamespaceDecl>(halide, context, "Internal");
  if (internal == nullptr) {
    return nullptr;
  }
  auto *call = lookupSingle<clang::CXXRecordDecl>(internal, context, "Call");
  if (call == nullptr || !call->isCompleteDefinition()) {
    return nullptr;
  }
  return call;
}

clang::Expr *MullASTMutator::buildDeclReference(clang::DeclContext *declContext,
                                                llvm::StringRef name,
                                                clang::SourceLocation location) {
  clang::DeclarationNameInfo nameInfo(&context.Idents.get(name), location);
  clang::LookupResult lookupResult(sema, nameInfo, clang::Sema::LookupOrdinaryName);
  lookupResult.suppressDiagnostics();
  if (!sema.LookupQualifiedName(lookupResult, declContext)) {
    return nullptr;
  }
  clang::CXXScopeSpec scopeSpec;
  clang::ExprResult reference =
      sema.BuildDeclarationNameExpr(scopeSpec, lookupResult, /*NeedsADL=*/false);
  if (reference.isInvalid()) {
    return nullptr;
  }
  return reference.get();
}

clang::Expr *MullASTMutator::buildTypeOfExpr(clang::Expr *value, clang::SourceLocation location) {
  clang::CXXRecordDecl *exprClass = value->getType()->getAsCXXRecordDecl();
  if (exprClass == nullptr) {
    return nullptr;
  }
  clang::DeclarationNameInfo typeNameInfo(&context.Idents.get("type"), location);
  clang::LookupResult typeLookup(sema, typeNameInfo, clang::Sema::LookupMemberName);
  typeLookup.suppressDiagnostics();
  if (!sema.LookupQualifiedName(typeLookup, exprClass)) {
    return nullptr;
  }
  clang::CXXScopeSpec emptyScopeSpec;
  clang::ExprResult typeMember = sema.BuildMemberReferenceExpr(value,
                                                               value->getType(),
                                                               location,
                                                               /*IsArrow=*/false,
                                                               emptyScopeSpec,
                                                               clang::SourceLocation(),
                                                               /*FirstQualifierInScope=*/nullptr,
                                                               typeLookup,
                                                               /*TemplateArgs=*/nullptr,
                                                               /*S=*/nullptr);
  if (typeMember.isInvalid()) {
    return nullptr;
  }
  clang::ExprResult typeCall =
      sema.BuildCallExpr(/*Scope=*/nullptr, typeMember.get(), location, {}, location);
  if (typeCall.isInvalid()) {
    return nullptr;
  }
  return typeCall.get();
}

clang::Expr *MullASTMutator::buildHalideCast(clang::Expr *halideType, clang::Expr *value,
                                             clang::SourceLocation location) {
  auto *halide =
      lookupSingle<clang::NamespaceDecl>(context.getTranslationUnitDecl(), context, "Halide");
  if (halide == nullptr) {
    return nullptr;
  }
  clang::Expr *castCallee = buildDeclReference(halide, "cast", location);
  if (castCallee == nullptr) {
    return nullptr;
  }
  llvm::SmallVector<clang::Expr *, 2> castArguments = { halideType, value };
  clang::ExprResult castCall =
      sema.BuildCallExpr(/*Scope=*/nullptr, castCallee, location, castArguments, location);
  if (castCall.isInvalid()) {
    return nullptr;
  }
  return castCall.get();
}

clang::Expr *MullASTMutator::buildIfThenElseCall(clang::CallExpr *selectCallExpr) {
  assert(selectCallExpr->getNumArgs() == 3);
  const clang::SourceLocation location = selectCallExpr->getBeginLoc();

  clang::CXXRecordDecl *callClass = lookupHalideInternalCall();
  if (callClass == nullptr) {
    return nullptr;
  }

  clang::Expr *condition = selectCallExpr->getArg(0);
  clang::Expr *trueValue = selectCallExpr->getArg(1);
  clang::Expr *falseValue = selectCallExpr->getArg(2);

  /// Call::make needs the result Halide type, and the two values must already
  /// carry it: Halide lowers the if_then_else intrinsic back into a Select,
  /// which asserts that both values have the same type.
  ///
  /// Halide::select reaches that type by running match_types over its two
  /// values, and match_types is not a pure function of either value alone --
  /// select(cond, 0, uint8_expr) is uint8, not int32. The only expression that
  /// reproduces Halide's answer exactly is the original select's own .type(),
  /// so it is what the replacement uses, with both values cast to it exactly as
  /// match_types would.
  ///
  /// The price is that the mutant evaluates the original select expression
  /// three times, once per use of its type. Halide Exprs are immutable,
  /// refcounted, pure values and generator code builds them without side
  /// effects, so the extra evaluations only allocate IR nodes that are dropped
  /// immediately. It is still a real cost of expressing this operator on the
  /// AST: a mutator inside Halide's own IR would have the type in hand.
  clang::Expr *resultType = buildTypeOfExpr(selectCallExpr, location);
  clang::Expr *trueValueType = buildTypeOfExpr(selectCallExpr, location);
  clang::Expr *falseValueType = buildTypeOfExpr(selectCallExpr, location);
  if (resultType == nullptr || trueValueType == nullptr || falseValueType == nullptr) {
    return nullptr;
  }

  clang::Expr *castTrueValue = buildHalideCast(trueValueType, trueValue, location);
  clang::Expr *castFalseValue = buildHalideCast(falseValueType, falseValue, location);
  if (castTrueValue == nullptr || castFalseValue == nullptr) {
    return nullptr;
  }
  trueValue = castTrueValue;
  falseValue = castFalseValue;

  clang::Expr *intrinsicOp = buildDeclReference(callClass, "if_then_else", location);
  clang::Expr *callType = buildDeclReference(callClass, "PureIntrinsic", location);
  clang::Expr *makeCallee = buildDeclReference(callClass, "make", location);
  if (intrinsicOp == nullptr || callType == nullptr || makeCallee == nullptr) {
    return nullptr;
  }

  /// { cond, true_value, false_value }, list-initialising the
  /// const std::vector<Expr> & parameter.
  llvm::SmallVector<clang::Expr *, 3> intrinsicArguments = { condition, trueValue, falseValue };
  auto *argumentList = new (context) clang::InitListExpr(context,
                                                         location,
                                                         intrinsicArguments,
                                                         selectCallExpr->getRParenLoc());
  /// A syntactic initializer list carries a void placeholder type until
  /// initialization gives it a real one, exactly as Sema::ActOnInitList leaves
  /// it. Without the placeholder, overload resolution dereferences a null type.
  argumentList->setType(context.VoidTy);

  llvm::SmallVector<clang::Expr *, 4> makeArguments = {
    resultType, intrinsicOp, argumentList, callType
  };
  clang::ExprResult ifThenElseCall = sema.BuildCallExpr(/*Scope=*/nullptr,
                                                        makeCallee,
                                                        location,
                                                        makeArguments,
                                                        selectCallExpr->getRParenLoc());
  if (ifThenElseCall.isInvalid()) {
    return nullptr;
  }
  return ifThenElseCall.get();
}

void MullASTMutator::performHalideSelectToIfThenElseMutation(
    ASTMutationPoint &mutation, HalideSelectToIfThenElseMutation &halideSelectToIfThenElseMutator) {
  clang::CallExpr *oldCall = halideSelectToIfThenElseMutator.callExpr;
  clang::Expr *newCall = buildIfThenElseCall(oldCall);
  if (newCall == nullptr) {
    llvm::errs() << "mull-cxx-frontend: could not build a Halide::Internal::Call::make("
                    "if_then_else) expression, skipping mutation: "
                 << mutation.mutationIdentifier << "\n";
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
