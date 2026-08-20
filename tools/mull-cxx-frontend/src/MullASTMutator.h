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
  void
  performHalideArgumentSwapMutation(ASTMutationPoint &mutation,
                                    HalideArgumentSwapMutation &halideArgumentSwapMutator) override;
  void performHalideSelectToIfThenElseMutation(
      ASTMutationPoint &mutation,
      HalideSelectToIfThenElseMutation &halideSelectToIfThenElseMutator) override;

private:
  /// Re-resolves `callExpr`'s callee to `newCalleeName` in the callee's own
  /// DeclContext, keeping the original arguments. Returns nullptr when the
  /// name does not resolve or overload resolution fails, in which case the
  /// mutation is skipped rather than producing ill-formed AST.
  clang::Expr *buildCalleeSwappedCall(clang::CallExpr *callExpr,
                                      const std::string &newCalleeName);

  /// Rebuilds `callExpr` with the arguments at the two given indices
  /// exchanged. Returns nullptr when overload resolution fails on the
  /// reordered arguments, in which case the mutation is skipped.
  clang::Expr *buildArgumentSwappedCall(clang::CallExpr *callExpr, unsigned firstArgumentIndex,
                                        unsigned secondArgumentIndex);

  /// Builds
  ///   Halide::Internal::Call::make(<value>.type(),
  ///                                Halide::Internal::Call::if_then_else,
  ///                                { cond, true_value, false_value },
  ///                                Halide::Internal::Call::PureIntrinsic)
  /// from the arguments of a Halide::select call. Returns nullptr when any part
  /// of Halide::Internal::Call fails to resolve or Sema rejects the result.
  clang::Expr *buildIfThenElseCall(clang::CallExpr *selectCallExpr);

  /// Looks `name` up in `declContext` and builds a reference to it. `asCallee`
  /// keeps an overload set unresolved so the caller's BuildCallExpr can pick.
  clang::Expr *buildDeclReference(clang::DeclContext *declContext, llvm::StringRef name,
                                  clang::SourceLocation location);

  /// The Halide::Internal::Call class, or nullptr when it is not declared in
  /// this translation unit.
  clang::CXXRecordDecl *lookupHalideInternalCall();

  /// Builds `<value>.type()`, the Halide type of a Halide::Expr.
  clang::Expr *buildTypeOfExpr(clang::Expr *value, clang::SourceLocation location);

  /// Builds `Halide::cast(<halideType>, <value>)`.
  clang::Expr *buildHalideCast(clang::Expr *halideType, clang::Expr *value,
                               clang::SourceLocation location);

  clang::ASTContext &context;
  clang::Sema &sema;
  ASTNodeFactory factory;
  ASTInstrumentation instrumentation;
  ClangASTMutator clangAstMutator;

  [[noreturn]] static void notImplemented() noexcept;
};

} // namespace cxx
} // namespace mull
