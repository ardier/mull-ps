#pragma once

#include "ASTInstrumentation.h"
#include "ASTMutationPoint.h"
#include "ASTMutator.h"
#include "ASTNodeFactory.h"
#include "ClangASTMutator.h"

namespace clang {
class ASTContext;
class FunctionDecl;
} // namespace clang

namespace mull {
namespace cxx {

class MullASTMutator : public ASTMutator {
public:
  MullASTMutator(clang::ASTContext &context, clang::Sema &sema)
      : context(context), sema(sema), factory(context),
        instrumentation(context, sema, factory),
        clangAstMutator(context, factory, instrumentation) {}

  void instrumentTranslationUnit();
  void performBinaryMutation(ASTMutationPoint &mutation, BinaryMutation &binaryMutator) override;
  void performRemoveVoidMutation(ASTMutationPoint &mutation,
                                 RemoveVoidMutation &removeVoidMutator) override;
  void performReplaceScalarMutation(ASTMutationPoint &mutation,
                                    ReplaceScalarCallMutation &replaceScalarCallMutator) override;
  void performUnaryOperatorOpcodeMutation(
      ASTMutationPoint &mutation, UnaryOperatorOpcodeMutation &unaryOperatorOpcodeMutator) override;
  void
  performUnaryOperatorRemovalMutation(ASTMutationPoint &mutation,
                                      UnaryOperatorRemovalMutation &unaryNotToNoopMutator) override;
  void performReplaceNumericAssignmentMutation(
      ASTMutationPoint &mutation,
      ReplaceNumericAssignmentMutation &replaceNumericAssignmentMutator) override;
  void performReplaceNumericInitAssignmentMutation(
      ASTMutationPoint &mutation,
      ReplaceNumericInitAssignmentMutation &replaceNumericInitAssignmentMutator) override;
  void
  performHalideCalleeSwapMutation(ASTMutationPoint &mutation,
                                  HalideCalleeSwapMutation &halideCalleeSwapMutator) override;

private:
  /// Re-resolves `callExpr`'s callee to `newCalleeName` in the callee's own
  /// DeclContext, keeping the original arguments. Returns nullptr when the
  /// name does not resolve or overload resolution fails, in which case the
  /// mutation is skipped rather than producing ill-formed AST.
  clang::Expr *buildCalleeSwappedCall(clang::CallExpr *callExpr,
                                      const std::string &newCalleeName);

  clang::ASTContext &context;
  clang::Sema &sema;
  ASTNodeFactory factory;
  ASTInstrumentation instrumentation;
  ClangASTMutator clangAstMutator;

  [[noreturn]] static void notImplemented() noexcept;
};

} // namespace cxx
} // namespace mull
