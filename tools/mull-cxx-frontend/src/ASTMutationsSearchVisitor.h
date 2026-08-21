#pragma once

#include "ASTMutationPoint.h"

#include <clang/AST/Decl.h>
#include <clang/AST/RecursiveASTVisitor.h>

namespace mull {
namespace cxx {

class MutationMap;

class ASTMutationsSearchVisitor : public clang::RecursiveASTVisitor<ASTMutationsSearchVisitor> {
  clang::ASTContext &context;
  clang::SourceManager &sourceManager;
  std::vector<std::unique_ptr<ASTMutationPoint>> astMutations;
  MutationMap &mutationMap;
  const clang::FunctionDecl *enclosingFunction;

public:
  /// `enclosingFunction` is the function whose body is being searched. Some
  /// mutations need it to tell a call written by the user apart from a call
  /// that a library makes to itself.
  ASTMutationsSearchVisitor(clang::ASTContext &context, MutationMap &mutationMap,
                            const clang::FunctionDecl *enclosingFunction = nullptr)
      : context(context), sourceManager(context.getSourceManager()), astMutations(),
        mutationMap(mutationMap), enclosingFunction(enclosingFunction) {}

  std::vector<std::unique_ptr<ASTMutationPoint>> &getAstMutations();

  bool VisitFunctionDecl(clang::FunctionDecl *FD);
  bool VisitUnaryOperator(clang::UnaryOperator *unaryOperator);
  bool VisitBinaryOperator(clang::BinaryOperator *binaryOperator);
  bool VisitCallExpr(clang::CallExpr *callExpr);
  bool VisitVarDecl(clang::VarDecl *D);

private:
  /// Records the Halide::BoundaryConditions family swaps available at
  /// `callExpr`, if any.
  void visitHalideBoundaryConditionsCall(clang::CallExpr *callExpr);

  /// Records the mutations available at `callExpr` for the Halide API calls
  /// whose argument order carries domain semantics, plus the eager-to-lazy
  /// select rewrite.
  void visitHalideSpecialCall(clang::CallExpr *callExpr);

  /// True when the body being searched is part of Halide itself rather than
  /// part of the code under test.
  bool isInsideHalideItself() const;

  bool isValidMutation(mull::MutatorKind mutatorKind);
  void recordMutationPoint(mull::MutatorKind mutatorKind, std::unique_ptr<ASTMutation> mutation,
                           clang::Stmt *stmt, clang::SourceLocation mutationLocation,
                           bool locationIsExpression);
  std::pair<clang::SourceLocation, clang::SourceLocation>
  getBeginEndMutationLocation(clang::Stmt *stmt, clang::SourceLocation mutationLocation,
                              bool locationIsExpression);
};

} // namespace cxx
} // namespace mull
