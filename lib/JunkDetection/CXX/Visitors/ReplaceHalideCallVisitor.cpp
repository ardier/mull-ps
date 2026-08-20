#include "mull/JunkDetection/CXX/Visitors/ReplaceHalideCallVisitor.h"
#include <iostream>
using namespace mull;

ReplaceHalideCallVisitor::ReplaceHalideCallVisitor(const VisitorParameters &parameters)
    : visitor(parameters) {}

bool ReplaceHalideCallVisitor::VisitCallExpr(clang::CallExpr *callExpression) {
  handleHalideCall(callExpression);
  return true;
}

bool ReplaceHalideCallVisitor::VisitCXXMemberCallExpr(clang::CXXMemberCallExpr *callExpression) {
  handleHalideCall(callExpression);
  return true;
}

bool ReplaceHalideCallVisitor::VisitCXXOperatorCallExpr(
    clang::CXXOperatorCallExpr *callExpression) {
  handleHalideCall(callExpression);
  return true;
}

void ReplaceHalideCallVisitor::handleHalideCall(clang::CallExpr *callExpression) {
  auto *type = callExpression->getType().getTypePtrOrNull();
  if (type && type->isRecordType()) {
    visitor.visitRangeWithASTExpr(callExpression);
  } else {
  }
}

const clang::Stmt *ReplaceHalideCallVisitor::foundMutant() {
  return visitor.getMatchingASTNode();
}
