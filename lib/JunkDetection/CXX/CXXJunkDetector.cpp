#include "mull/JunkDetection/CXX/CXXJunkDetector.h"

#include "mull/Diagnostics/Diagnostics.h"
#include "mull/JunkDetection/CXX/Visitors/BinaryVisitor.h"
#include "mull/JunkDetection/CXX/Visitors/NegateConditionVisitor.h"
#include "mull/JunkDetection/CXX/Visitors/RemoveVoidFunctionVisitor.h"
#include "mull/JunkDetection/CXX/Visitors/ReplaceCallVisitor.h"
#include "mull/JunkDetection/CXX/Visitors/ReplaceHalideCallVisitor.h"
#include "mull/JunkDetection/CXX/Visitors/ScalarValueVisitor.h"
#include "mull/JunkDetection/CXX/Visitors/UnaryVisitor.h"
#include "mull/JunkDetection/CXX/Visitors/VarDeclVisitor.h"
#include "mull/MutationPoint.h"
#include "mull/Mutators/CXX/RemoveNegation.h"
#include "mull/Mutators/Mutator.h"

using namespace mull;

CXXJunkDetector::CXXJunkDetector(Diagnostics &diagnostics, ASTStorage &astStorage)
    : diagnostics(diagnostics), astStorage(astStorage) {}

static const clang::Stmt *findMutantExpression(MutationPoint *point,
                                               VisitorParameters &visitorParameters,
                                               clang::Decl *decl) {
  switch (point->getMutator()->mutatorKind()) {

  // TODO add mutations
  case MutatorKind::Halide_ReplaceHalideAddToMulCall: {
    //
    ReplaceHalideCallVisitor visitor(visitorParameters);
    visitor.TraverseDecl(decl);
    return visitor.foundMutant();
  }

  case MutatorKind::Halide_ReplaceHalideAddToSubCall: {
    ReplaceHalideCallVisitor visitor(visitorParameters);
    visitor.TraverseDecl(decl);
    return visitor.foundMutant();
  }

  case MutatorKind::Halide_ReplaceHalideAddToDivCall: {
    //
    ReplaceHalideCallVisitor visitor(visitorParameters);
    visitor.TraverseDecl(decl);
    return visitor.foundMutant();
  }

  case MutatorKind::Halide_ReplaceHalideSubToMulCall: {
    //
    ReplaceHalideCallVisitor visitor(visitorParameters);
    visitor.TraverseDecl(decl);
    return visitor.foundMutant();
  }

  case MutatorKind::Halide_ReplaceHalideSubToAddCall: {
    //
    ReplaceHalideCallVisitor visitor(visitorParameters);
    visitor.TraverseDecl(decl);
    return visitor.foundMutant();
  }

  case MutatorKind::Halide_ReplaceHalideSubToDivCall: {
    //
    ReplaceHalideCallVisitor visitor(visitorParameters);
    visitor.TraverseDecl(decl);
    return visitor.foundMutant();
  }

  case MutatorKind::Halide_ReplaceHalideMulToAddCall: {
    //
    ReplaceHalideCallVisitor visitor(visitorParameters);
    visitor.TraverseDecl(decl);
    return visitor.foundMutant();
  }

  case MutatorKind::Halide_ReplaceHalideMulToSubCall: {
    //
    ReplaceHalideCallVisitor visitor(visitorParameters);
    visitor.TraverseDecl(decl);
    return visitor.foundMutant();
  }

  case MutatorKind::Halide_ReplaceHalideMulToDivCall: {
    //
    ReplaceHalideCallVisitor visitor(visitorParameters);
    visitor.TraverseDecl(decl);
    return visitor.foundMutant();
  }

  case MutatorKind::Halide_ReplaceHalideDivToMulCall: {
    //
    ReplaceHalideCallVisitor visitor(visitorParameters);
    visitor.TraverseDecl(decl);
    return visitor.foundMutant();
  }

  case MutatorKind::Halide_ReplaceHalideDivToSubCall: {
    //
    ReplaceHalideCallVisitor visitor(visitorParameters);
    visitor.TraverseDecl(decl);
    return visitor.foundMutant();
  }

  case MutatorKind::Halide_ReplaceHalideDivToAddCall: {
    //
    ReplaceHalideCallVisitor visitor(visitorParameters);
    visitor.TraverseDecl(decl);
    return visitor.foundMutant();
  }

  case MutatorKind::CXX_RemoveVoidCall: {
    RemoveVoidFunctionVisitor visitor(visitorParameters);
    visitor.TraverseDecl(decl);
    return visitor.foundMutant();
  }
  case MutatorKind::CXX_ReplaceScalarCall: {
    ReplaceCallVisitor visitor(visitorParameters);
    visitor.TraverseDecl(decl);
    return visitor.foundMutant();
  }
  case MutatorKind::NegateMutator: {
    NegateConditionVisitor visitor(visitorParameters);
    visitor.TraverseDecl(decl);
    return visitor.foundMutant();
  }
  case MutatorKind::ScalarValueMutator: {
    ScalarValueVisitor visitor(visitorParameters);
    visitor.TraverseDecl(decl);
    return visitor.foundMutant();
  }
  case MutatorKind::CXX_LessThanToLessOrEqual:
    return cxx::BinaryVisitor::findMutant(
        visitorParameters, decl, clang::BinaryOperator::Opcode::BO_LT);

  case MutatorKind::CXX_LessOrEqualToLessThan:
    return cxx::BinaryVisitor::findMutant(
        visitorParameters, decl, clang::BinaryOperator::Opcode::BO_LE);
  case MutatorKind::CXX_GreaterThanToGreaterOrEqual:
    return cxx::BinaryVisitor::findMutant(
        visitorParameters, decl, clang::BinaryOperator::Opcode::BO_GT);
  case MutatorKind::CXX_GreaterOrEqualToGreaterThan:
    return cxx::BinaryVisitor::findMutant(
        visitorParameters, decl, clang::BinaryOperator::Opcode::BO_GE);
  case MutatorKind::CXX_EqualToNotEqual:
    return cxx::BinaryVisitor::findMutant(
        visitorParameters, decl, clang::BinaryOperator::Opcode::BO_EQ);
  case MutatorKind::CXX_NotEqualToEqual:
    return cxx::BinaryVisitor::findMutant(
        visitorParameters, decl, clang::BinaryOperator::Opcode::BO_NE);
  case MutatorKind::CXX_GreaterThanToLessOrEqual:
    return cxx::BinaryVisitor::findMutant(
        visitorParameters, decl, clang::BinaryOperator::Opcode::BO_GT);
  case MutatorKind::CXX_GreaterOrEqualToLessThan:
    return cxx::BinaryVisitor::findMutant(
        visitorParameters, decl, clang::BinaryOperator::Opcode::BO_GE);
  case MutatorKind::CXX_LessThanToGreaterOrEqual:
    return cxx::BinaryVisitor::findMutant(
        visitorParameters, decl, clang::BinaryOperator::Opcode::BO_LT);
  case MutatorKind::CXX_LessOrEqualToGreaterThan:
    return cxx::BinaryVisitor::findMutant(
        visitorParameters, decl, clang::BinaryOperator::Opcode::BO_LE);

  case MutatorKind::CXX_AddToSub:
    return cxx::BinaryVisitor::findMutant(
        visitorParameters, decl, clang::BinaryOperator::Opcode::BO_Add);
  case MutatorKind::CXX_AddAssignToSubAssign:
    return cxx::BinaryVisitor::findMutant(
        visitorParameters, decl, clang::BinaryOperator::Opcode::BO_AddAssign);
  case MutatorKind::CXX_PreIncToPreDec:
    return cxx::UnaryVisitor::findMutant(
        visitorParameters, clang::UnaryOperator::Opcode::UO_PreInc, decl);
  case MutatorKind::CXX_PostIncToPostDec:
    return cxx::UnaryVisitor::findMutant(
        visitorParameters, clang::UnaryOperator::Opcode::UO_PostInc, decl);

  case MutatorKind::CXX_SubToAdd:
    return cxx::BinaryVisitor::findMutant(
        visitorParameters, decl, clang::BinaryOperator::Opcode::BO_Sub);
  case MutatorKind::CXX_SubAssignToAddAssign:
    return cxx::BinaryVisitor::findMutant(
        visitorParameters, decl, clang::BinaryOperator::Opcode::BO_SubAssign);
  case MutatorKind::CXX_PreDecToPreInc:
    return cxx::UnaryVisitor::findMutant(
        visitorParameters, clang::UnaryOperator::Opcode::UO_PreDec, decl);

  case MutatorKind::CXX_PostDecToPostInc:
    return cxx::UnaryVisitor::findMutant(
        visitorParameters, clang::UnaryOperator::Opcode::UO_PostDec, decl);

  case MutatorKind::CXX_MulToDiv:
    return cxx::BinaryVisitor::findMutant(
        visitorParameters, decl, clang::BinaryOperator::Opcode::BO_Mul);
  case MutatorKind::CXX_MulAssignToDivAssign:
    return cxx::BinaryVisitor::findMutant(
        visitorParameters, decl, clang::BinaryOperator::Opcode::BO_MulAssign);

  case MutatorKind::CXX_DivToMul:
    return cxx::BinaryVisitor::findMutant(
        visitorParameters, decl, clang::BinaryOperator::Opcode::BO_Div);
  case MutatorKind::CXX_DivAssignToMulAssign:
    return cxx::BinaryVisitor::findMutant(
        visitorParameters, decl, clang::BinaryOperator::Opcode::BO_DivAssign);

  case MutatorKind::CXX_RemToDiv:
    return cxx::BinaryVisitor::findMutant(
        visitorParameters, decl, clang::BinaryOperator::Opcode::BO_Rem);
  case MutatorKind::CXX_RemAssignToDivAssign:
    return cxx::BinaryVisitor::findMutant(
        visitorParameters, decl, clang::BinaryOperator::Opcode::BO_RemAssign);

  case MutatorKind::CXX_BitwiseNotToNoop:
    return cxx::UnaryVisitor::findMutant(
        visitorParameters, clang::UnaryOperator::Opcode::UO_Not, decl);

  case MutatorKind::CXX_UnaryMinusToNoop:
    return cxx::UnaryVisitor::findMutant(
        visitorParameters, clang::UnaryOperator::Opcode::UO_Minus, decl);

  case MutatorKind::CXX_LShiftToRShift:
    return cxx::BinaryVisitor::findMutant(
        visitorParameters, decl, clang::BinaryOperator::Opcode::BO_Shl);
  case MutatorKind::CXX_LShiftAssignToRShiftAssign:
    return cxx::BinaryVisitor::findMutant(
        visitorParameters, decl, clang::BinaryOperator::Opcode::BO_ShlAssign);
  case MutatorKind::CXX_RShiftToLShift:
    return cxx::BinaryVisitor::findMutant(
        visitorParameters, decl, clang::BinaryOperator::Opcode::BO_Shr);
  case MutatorKind::CXX_RShiftAssignToLShiftAssign:
    return cxx::BinaryVisitor::findMutant(
        visitorParameters, decl, clang::BinaryOperator::Opcode::BO_ShrAssign);

  case MutatorKind::CXX_Bitwise_AndToOr:
    return cxx::BinaryVisitor::findMutant(
        visitorParameters, decl, clang::BinaryOperator::Opcode::BO_And);
  case MutatorKind::CXX_Bitwise_AndAssignToOrAssign:
    return cxx::BinaryVisitor::findMutant(
        visitorParameters, decl, clang::BinaryOperator::Opcode::BO_AndAssign);
  case MutatorKind::CXX_Bitwise_OrToAnd:
    return cxx::BinaryVisitor::findMutant(
        visitorParameters, decl, clang::BinaryOperator::Opcode::BO_Or);
  case MutatorKind::CXX_Bitwise_OrAssignToAndAssign:
    return cxx::BinaryVisitor::findMutant(
        visitorParameters, decl, clang::BinaryOperator::Opcode::BO_OrAssign);
  case MutatorKind::CXX_Bitwise_XorToOr:
    return cxx::BinaryVisitor::findMutant(
        visitorParameters, decl, clang::BinaryOperator::Opcode::BO_Xor);
  case MutatorKind::CXX_Bitwise_XorAssignToOrAssign:
    return cxx::BinaryVisitor::findMutant(
        visitorParameters, decl, clang::BinaryOperator::Opcode::BO_XorAssign);

  case MutatorKind::CXX_AssignConst:
    return cxx::BinaryVisitor::findMutant(
        visitorParameters, decl, clang::BinaryOperator::Opcode::BO_Assign);
  case MutatorKind::CXX_InitConst: {
    cxx::VarDeclVisitor visitor(visitorParameters);
    visitor.TraverseDecl(decl);
    return visitor.foundMutant();
  }
  case MutatorKind::CXX_Logical_AndToOr:
    return cxx::BinaryVisitor::findMutant(
        visitorParameters, decl, clang::BinaryOperator::Opcode::BO_LAnd);
  case MutatorKind::CXX_Logical_OrToAnd:
    return cxx::BinaryVisitor::findMutant(
        visitorParameters, decl, clang::BinaryOperator::Opcode::BO_LOr);

  case MutatorKind::CXX_RemoveNegation:
    return cxx::UnaryVisitor::findMutant(
        visitorParameters, clang::UnaryOperator::Opcode::UO_LNot, decl);

  default:
    return nullptr;
  }
}

bool CXXJunkDetector::isJunk(MutationPoint *point) {

  if (point->getSourceLocation().isNull()) {
    std::cout << "\npoint->getMutatorIdentifier():" << point->getMutatorIdentifier()
              << "\nJunk because source location is null\n"
              << std::endl;
    return true;
  }

  ThreadSafeASTUnit *ast = astStorage.findAST(point->getSourceLocation());
  if (!ast->hasAST()) {
    std::cout << "\npoint->getMutatorIdentifier():" << point->getMutatorIdentifier() << "\n"
              << "\nDoesn't have AST\n"
              << std::endl;
    return true;
  }
  clang::SourceLocation location = ast->getLocation(point->getSourceLocation());
  clang::SourceManager &sourceManager = ast->getSourceManager();

  // if it is  halide mutator, then skip it
  if (ast->isInSystemHeader(location) && point->getMutatorIdentifier() != "Halide_Mutators") {
    std::cout << "\npoint->getMutatorIdentifier():" << point->getMutatorIdentifier()
              << "\nIs in system header\n"
              << std::endl;
    return true;
  }

  clang::Decl *decl = ast->getDecl(location);
  if (!decl) {
    std::cout << "\npoint->getMutatorIdentifier():" << point->getMutatorIdentifier()
              << "\nNot Decl\n"
              << std::endl;
    return true;
  }

  VisitorParameters visitorParameters = { .sourceManager = sourceManager,
                                          .sourceLocation = location,
                                          .astContext = ast->getASTContext() };

  const clang::Stmt *mutantExpression = findMutantExpression(point, visitorParameters, decl);

  if (!mutantExpression) {
    std::cout << "\npoint->getMutatorIdentifier():" << point->getMutatorIdentifier()
              << "\nNot mutant expression\n"
              << std::endl;
    return true;
  }

  clang::SourceLocation mutantExpressionBeginLoc = mutantExpression->getSourceRange().getBegin();
  int beginLine = sourceManager.getExpansionLineNumber(mutantExpressionBeginLoc, nullptr);
  int beginColumn = sourceManager.getExpansionColumnNumber(mutantExpressionBeginLoc);

  int mutationLocationBeginLine = sourceManager.getExpansionLineNumber(location, nullptr);
  int mutationLocationBeginColumn = sourceManager.getExpansionColumnNumber(location);

  /// There are two types of mutated expressions:
  /// 1) Remove-Void, CallExpr example: its mutation location and its getStart() are the same.
  /// 2) Binary Mutation, BinaryOperator example: its mutation location is
  /// BinaryOperator's getOperatorLoc(), i.e. "+", while the
  /// [getSourceRange().getBegin(), getSourceRange().getEnd()] range is the whole "a + b"
  /// expression.
  clang::SourceLocation sourceLocationEnd =
      (beginLine == mutationLocationBeginLine && beginColumn == mutationLocationBeginColumn)
          ? mutantExpression->getSourceRange().getEnd()
          : location;

  // For unary operators we shouldn't consider the location of the whole expression, but only
  // the operator, otherwise it breaks patch reporter in weird ways
  if (auto unary = llvm::dyn_cast<clang::UnaryOperator>(mutantExpression)) {
    sourceLocationEnd = unary->getOperatorLoc();
  }

  /// Clang AST: how to get more precise debug information in certain cases?
  /// http://clang-developers.42468.n3.nabble.com/Clang-AST-how-to-get-more-precise-debug-information-in-certain-cases-td4065195.html
  /// https://stackoverflow.com/questions/11083066/getting-the-source-behind-clangs-ast
  clang::SourceLocation sourceLocationEndActual = ast->getLocForEndOfToken(sourceLocationEnd);

  int endLine = sourceManager.getExpansionLineNumber(sourceLocationEndActual, nullptr);
  int endColumn = sourceManager.getExpansionColumnNumber(sourceLocationEndActual);

  const std::string &sourceFile = point->getSourceLocation().filePath;
  std::string description = MutationKindToString(point->getMutator()->mutatorKind());
  diagnostics.debug(std::string("CXXJunkDetector: mutation \"") + description +
                    "\": " + sourceFile + ":" + std::to_string(mutationLocationBeginLine) + ":" +
                    std::to_string(mutationLocationBeginColumn) +
                    " (end: " + std::to_string(endLine) + ":" + std::to_string(endColumn) + ")");

  point->setEndLocation(endLine, endColumn);

  return false;
}
