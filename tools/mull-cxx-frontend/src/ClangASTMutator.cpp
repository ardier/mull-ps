#include "ASTInstrumentation.h"

#include "ClangASTMutator.h"

#include "ASTNodeFactory.h"

#include <clang/AST/ASTContext.h>
#include <clang/AST/ParentMapContext.h>
#include <clang/Basic/SourceLocation.h>
#include <llvm/Support/Casting.h>

namespace mull {
namespace cxx {

/// Replaces `oldExpr` by `newExpr` among the direct children of `parentStmt`.
/// Returns false when `oldExpr` is not (or no longer) a child of it.
static bool replaceChild(clang::Stmt *parentStmt, clang::Expr *oldExpr, clang::Expr *newExpr) {
  clang::Stmt::child_iterator childIterator =
      std::find(parentStmt->child_begin(), parentStmt->child_end(), oldExpr);
  if (childIterator == parentStmt->child_end()) {
    return false;
  }
  *childIterator = newExpr;
  return true;
}

void ClangASTMutator::replaceExpression(clang::Expr *oldExpr, clang::Expr *newExpr,
                                        std::string identifier) {
  clang::ConditionalOperator *conditionalExpr =
      createMutatedExpression(oldExpr, newExpr, identifier);

  /// A single expression can carry more than one mutation (`a < b` is both a
  /// comparison and a boundary mutation point; a BoundaryConditions call can be
  /// swapped for each of the three other members of its family). Once the first
  /// mutation has been spliced in, ASTContext's parent map is stale -- it still
  /// reports the original parent, which no longer has `oldExpr` among its
  /// children -- so the mutations after the first one are nested inside the
  /// conditional built for the previous one instead:
  ///     getenv(id2) ? new2 : (getenv(id1) ? new1 : old)
  /// Each mutant stays independently selectable by its own environment variable.
  clang::Stmt *previousHolder = mutationHolders.lookup(oldExpr);
  if (previousHolder != nullptr) {
    bool replaced = replaceChild(previousHolder, oldExpr, conditionalExpr);
    assert(replaced && "mutated expression vanished from its own mutation conditional");
    (void)replaced;
    mutationHolders[oldExpr] = conditionalExpr;
    return;
  }

  for (auto p : context.getParents(*oldExpr)) {
    if (const clang::Stmt *constParentStmt = p.get<clang::Stmt>()) {
      /// This is where the actual mutation of expression happens and this where
      /// things play against current Clang AST API.
      /// TODO: Find a better way to perform the mutation.
      clang::Stmt *parentStmt = (clang::Stmt *)constParentStmt;
      bool replaced = replaceChild(parentStmt, oldExpr, conditionalExpr);
      assert(replaced);
      (void)replaced;
      mutationHolders[oldExpr] = conditionalExpr;
      return;
    } else if (const clang::VarDecl *constVarDecl = p.get<clang::VarDecl>()) {
      clang::VarDecl *parentVarDecl = (clang::VarDecl *)constVarDecl;
      parentVarDecl->setInit(conditionalExpr);
      mutationHolders[oldExpr] = conditionalExpr;
      return;
    } else {
      assert(0 && "error: not implemented");
    }
  }
  assert(0 && "Should not reach here");
}

void ClangASTMutator::replaceStatement(clang::Stmt *oldStmt, clang::Stmt *newStmt,
                                       std::string identifier) {
  clang::IfStmt *ifCondition = createMutatedStatement(oldStmt, newStmt, identifier);

  for (auto p : context.getParents(*oldStmt)) {
    const clang::Stmt *constParentStmt = p.get<clang::Stmt>();
    clang::Stmt *parentStmt = (clang::Stmt *)constParentStmt;
    assert(parentStmt);

    clang::Stmt::child_iterator parentChildrenIterator =
        std::find(parentStmt->child_begin(), parentStmt->child_end(), oldStmt);
    assert(parentChildrenIterator != parentStmt->child_end());

    *parentChildrenIterator = ifCondition;
    return;
  }
  assert(0 && "Should not reach here");
}

clang::IfStmt *ClangASTMutator::createMutatedStatement(clang::Stmt *oldStmt, clang::Stmt *newStmt,
                                                       std::string identifier) {
  clang::CallExpr *getenvCallExpr = createGetenvCallExpr(identifier);

  clang::ImplicitCastExpr *implicitCastExpr =
      clang::ImplicitCastExpr::Create(context,
                                      context.BoolTy,
                                      clang::CastKind::CK_PointerToBoolean,
                                      getenvCallExpr,
                                      nullptr,
#if LLVM_VERSION_MAJOR >= 13
                                      clang::VK_PRValue,
#else
                                      clang::VK_RValue,
#endif
                                      clang::FPOptionsOverride());

  std::vector<clang::Stmt *> thenStmtsVec = {};
  if (newStmt) {
    thenStmtsVec.push_back(newStmt);
  }
  llvm::ArrayRef<clang::Stmt *> thenStmts = thenStmtsVec;
  clang::CompoundStmt *compoundThenStmt = clang::CompoundStmt::Create(context,
                                                                      thenStmts,
#if LLVM_VERSION_MAJOR >= 15
                                                                      clang::FPOptionsOverride(),
#endif
                                                                      NULL_LOCATION,
                                                                      NULL_LOCATION);

  llvm::MutableArrayRef<clang::Stmt *> elseStmts = { oldStmt };
  clang::CompoundStmt *compoundElseStmt = clang::CompoundStmt::Create(context,
                                                                      elseStmts,
#if LLVM_VERSION_MAJOR >= 15
                                                                      clang::FPOptionsOverride(),
#endif
                                                                      NULL_LOCATION,
                                                                      NULL_LOCATION);

  clang::IfStmt *ifStmt =
      factory.createIfStmt(implicitCastExpr, compoundThenStmt, compoundElseStmt);

  return ifStmt;
}

clang::ConditionalOperator *ClangASTMutator::createMutatedExpression(clang::Expr *oldExpr,
                                                                     clang::Expr *newExpr,
                                                                     std::string identifier) {
  clang::CallExpr *mullShouldMutateCallExpr = createGetenvCallExpr(identifier);

  clang::ImplicitCastExpr *implicitCastExpr3 =
      clang::ImplicitCastExpr::Create(context,
                                      context.BoolTy,
                                      clang::CastKind::CK_PointerToBoolean,
                                      mullShouldMutateCallExpr,
                                      nullptr,
#if LLVM_VERSION_MAJOR >= 13
                                      clang::VK_PRValue,
#else
                                      clang::VK_RValue,
#endif
                                      clang::FPOptionsOverride());

  clang::ConditionalOperator *conditionalOperator =
      new (context) clang::ConditionalOperator(implicitCastExpr3,
                                               NULL_LOCATION,
                                               newExpr,
                                               NULL_LOCATION,
                                               oldExpr,
                                               newExpr->getType(),
                                               newExpr->getValueKind(),
                                               newExpr->getObjectKind());

  return conditionalOperator;
}

clang::CallExpr *ClangASTMutator::createGetenvCallExpr(std::string identifier) {
  clang::FunctionDecl *_getenvFuncDecl = instrumentation.getGetenvFuncDecl();
  clang::DeclRefExpr *declRefExpr = clang::DeclRefExpr::Create(context,
                                                               _getenvFuncDecl->getQualifierLoc(),
                                                               NULL_LOCATION,
                                                               _getenvFuncDecl,
                                                               false,
                                                               NULL_LOCATION,
                                                               _getenvFuncDecl->getType(),
                                                               clang::VK_LValue);

  clang::ImplicitCastExpr *implicitCastExpr =
      factory.createImplicitCastExpr(declRefExpr,
                                     context.getPointerType(_getenvFuncDecl->getType()),
                                     clang::CastKind::CK_FunctionToPointerDecay,
#if LLVM_VERSION_MAJOR >= 13
                                     clang::VK_PRValue
#else
                                     clang::VK_RValue
#endif
      );

  clang::StringLiteral *stringLiteral = clang::StringLiteral::Create(
      context,
      identifier,
#if LLVM_VERSION_MAJOR >= 15
      clang::StringLiteral::StringKind::Ordinary,
#else
      clang::StringLiteral::StringKind::Ascii,
#endif
      false,
      factory.getStringLiteralArrayType(context.CharTy, identifier.size()),
      clang::SourceLocation());

  clang::ImplicitCastExpr *implicitCastExpr2 =
      clang::ImplicitCastExpr::Create(context,
                                      context.getPointerType(context.getConstType(context.CharTy)),
                                      clang::CastKind::CK_ArrayToPointerDecay,
                                      stringLiteral,
                                      nullptr,
#if LLVM_VERSION_MAJOR >= 13
                                      clang::VK_PRValue,
#else
                                      clang::VK_RValue,
#endif
                                      clang::FPOptionsOverride());

  clang::CallExpr *callExpr = factory.createCallExprSingleArg(implicitCastExpr,
                                                              implicitCastExpr2,
                                                              _getenvFuncDecl->getReturnType(),
#if LLVM_VERSION_MAJOR >= 13
                                                              clang::VK_PRValue
#else
                                                              clang::VK_RValue
#endif
  );

  return callExpr;
}

} // namespace cxx
} // namespace mull
