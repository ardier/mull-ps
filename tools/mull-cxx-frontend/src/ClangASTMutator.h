#pragma once

#include <string>

namespace clang {
class ASTContext;
class BinaryOperator;
class CallExpr;
class ConditionalOperator;
class FunctionDecl;
class Expr;
class IfStmt;
class Stmt;
} // namespace clang

namespace mull {
namespace cxx {

class ASTInstrumentation;
class ASTNodeFactory;

class ClangASTMutator {
  clang::ASTContext &context;
  ASTNodeFactory &factory;
  ASTInstrumentation &instrumentation;

public:
  ClangASTMutator(clang::ASTContext &context, ASTNodeFactory &factory,
                  ASTInstrumentation &instrumentation)
      : context(context), factory(factory), instrumentation(instrumentation) {}

  /// Splices `newExpr` (guarded by a getenv() check on `identifier`) in place of
  /// `oldExpr`. Returns false when the enclosing node cannot be found, in which
  /// case the AST is left untouched and the mutation must not be recorded.
  bool replaceExpression(clang::Expr *oldExpr, clang::Expr *newExpr, std::string identifier);
  /// Same contract as replaceExpression(), for statements.
  bool replaceStatement(clang::Stmt *oldStmt, clang::Stmt *newStmt, std::string identifier);
  clang::IfStmt *createMutatedStatement(clang::Stmt *oldStmt, clang::Stmt *newStmt,
                                        std::string identifier);
  clang::ConditionalOperator *createMutatedExpression(clang::Expr *oldExpr, clang::Expr *newExpr,
                                                      std::string identifier);
  clang::CallExpr *createGetenvCallExpr(std::string identifier);
};

} // namespace cxx
} // namespace mull
