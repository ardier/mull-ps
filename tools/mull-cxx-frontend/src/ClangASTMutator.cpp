#include "ASTInstrumentation.h"

#include "ClangASTMutator.h"

#include "ASTNodeFactory.h"

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ParentMapContext.h>
#include <clang/Basic/SourceLocation.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/raw_ostream.h>

namespace mull {
namespace cxx {

/// ASTContext::getParents() builds the parent map lazily, over the translation
/// unit *as it exists at the moment of the very first query*, and then caches it
/// forever. That map goes wrong on us in two distinct ways:
///
///  1. It is incomplete. The mutations are performed from
///     MullASTConsumer::HandleTopLevelDecl(), i.e. while the translation unit is
///     still being parsed. The first mutation builds the map; every declaration
///     parsed after that point is simply absent from it, so getParents() comes
///     back empty and the old code walked off the end of the loop straight into
///     `assert(0 && "Should not reach here")`. Any translation unit with more
///     than one mutable function hits this -- upstream's own test suite never
///     did only because every sample has exactly one (`main` is skipped).
///
///  2. It is stale. Once a mutation has been spliced in, the cached map still
///     reports the *original* parent of the mutated node, which no longer has it
///     among its children. A second mutation of the same expression (`a < b` is
///     both a comparison and a boundary mutation point) then failed the
///     `parentChildrenIterator != child_end()` assertion.
///
/// Both are handled the same way: try the cached map first (the common case, and
/// cheap), and if the parent it names does not actually hold the node any more,
/// drop the map and rebuild it. Rebuilding after a splice also gives nested
/// mutations for free: the parent found on the retry is the conditional built for
/// the previous mutation, so the mutations stack as
///     getenv(id2) ? new2 : (getenv(id1) ? new1 : old)
/// and each mutant stays independently selectable by its own environment
/// variable.
///
/// Returns false if no node holding `oldStmt` could be found even after a
/// rebuild; the caller then skips the mutation rather than aborting the compiler.
static bool spliceReplacement(clang::ASTContext &context, clang::Stmt *oldStmt,
                              clang::Stmt *replacement, bool allowVarDeclParent) {
  for (int attempt = 0; attempt < 2; attempt++) {
    for (auto p : context.getParents(*oldStmt)) {
      if (const clang::Stmt *constParentStmt = p.get<clang::Stmt>()) {
        /// This is where the actual mutation of expression happens and this where
        /// things play against current Clang AST API.
        /// TODO: Find a better way to perform the mutation.
        clang::Stmt *parentStmt = const_cast<clang::Stmt *>(constParentStmt);
        clang::Stmt::child_iterator parentChildrenIterator =
            std::find(parentStmt->child_begin(), parentStmt->child_end(), oldStmt);
        if (parentChildrenIterator == parentStmt->child_end()) {
          /// Stale entry: this used to be the parent, it is not any more.
          continue;
        }
        *parentChildrenIterator = replacement;
        return true;
      }
      if (!allowVarDeclParent) {
        continue;
      }
      if (const clang::VarDecl *constVarDecl = p.get<clang::VarDecl>()) {
        clang::VarDecl *parentVarDecl = const_cast<clang::VarDecl *>(constVarDecl);
        if (parentVarDecl->getInit() != oldStmt) {
          /// Stale entry, as above.
          continue;
        }
        parentVarDecl->setInit(clang::cast<clang::Expr>(replacement));
        return true;
      }
    }
    if (attempt == 0) {
      /// The cached parent map is either incomplete (the node was parsed after
      /// the map was built) or stale (a previous mutation moved the node).
      /// Throw it away; the next getParents() call rebuilds it from the
      /// translation unit as it stands now.
      context.getParentMapContext().clear();
    }
  }
  return false;
}

bool ClangASTMutator::replaceExpression(clang::Expr *oldExpr, clang::Expr *newExpr,
                                        std::string identifier) {
  clang::ConditionalOperator *conditionalExpr =
      createMutatedExpression(oldExpr, newExpr, identifier);

  if (spliceReplacement(context, oldExpr, conditionalExpr, /*allowVarDeclParent=*/true)) {
    return true;
  }
  llvm::errs() << "[warning] mull: skipping mutation " << identifier
               << ": no enclosing AST node found for the mutated expression\n";
  return false;
}

bool ClangASTMutator::replaceStatement(clang::Stmt *oldStmt, clang::Stmt *newStmt,
                                       std::string identifier) {
  clang::IfStmt *ifCondition = createMutatedStatement(oldStmt, newStmt, identifier);

  if (spliceReplacement(context, oldStmt, ifCondition, /*allowVarDeclParent=*/false)) {
    return true;
  }
  llvm::errs() << "[warning] mull: skipping mutation " << identifier
               << ": no enclosing AST node found for the mutated statement\n";
  return false;
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
#if LLVM_VERSION_MAJOR >= 18
      clang::StringLiteralKind::Ordinary,
#elif LLVM_VERSION_MAJOR >= 15
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
