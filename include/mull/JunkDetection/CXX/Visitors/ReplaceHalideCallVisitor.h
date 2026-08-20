#pragma once

#include "InstructionRangeVisitor.h"
#include "VisitorParameters.h"

#include <clang/AST/RecursiveASTVisitor.h>

namespace mull {

class ReplaceHalideCallVisitor : public clang::RecursiveASTVisitor<ReplaceHalideCallVisitor> {
public:
  ReplaceHalideCallVisitor(const VisitorParameters &parameters);

  bool VisitCallExpr(clang::CallExpr *callExpression);
  bool VisitCXXMemberCallExpr(clang::CXXMemberCallExpr *callExpression);
  bool VisitCXXOperatorCallExpr(clang::CXXOperatorCallExpr *callExpression);
  void handleHalideCall(clang::CallExpr *callExpression);
  const clang::Stmt *foundMutant();

private:
  InstructionRangeVisitor visitor;
};

} // namespace mull