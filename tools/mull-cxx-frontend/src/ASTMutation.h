#pragma once

#include "ASTMutator.h"

#include <clang/AST/Expr.h>

namespace mull {
namespace cxx {

class ASTMutation {
public:
  virtual void performMutation(ASTMutationPoint &mutation, ASTMutator &mutator) = 0;
  virtual ~ASTMutation() {}
};

class BinaryMutation : public ASTMutation {
public:
  clang::BinaryOperator::Opcode replacementOpCode;

  BinaryMutation(clang::BinaryOperator::Opcode replacementOpCode)
      : replacementOpCode(replacementOpCode) {}
  void performMutation(ASTMutationPoint &mutation, ASTMutator &mutator) {
    mutator.performBinaryMutation(mutation, *this);
  }
  ~BinaryMutation() {}
};

class RemoveVoidMutation : public ASTMutation {
public:
  void performMutation(ASTMutationPoint &mutation, ASTMutator &mutator) {
    mutator.performRemoveVoidMutation(mutation, *this);
  }
  ~RemoveVoidMutation() {}
};

class ReplaceScalarCallMutation : public ASTMutation {
public:
  clang::CallExpr *callExpr;

  ReplaceScalarCallMutation(clang::CallExpr *callExpr) : callExpr(callExpr) {}
  ~ReplaceScalarCallMutation() {}
  void performMutation(ASTMutationPoint &mutation, ASTMutator &mutator) {
    mutator.performReplaceScalarMutation(mutation, *this);
  }
};

class UnaryOperatorOpcodeMutation : public ASTMutation {
public:
  clang::UnaryOperator *unaryOperator;
  clang::UnaryOperator::Opcode replacementOpCode;

  UnaryOperatorOpcodeMutation(clang::UnaryOperator *unaryOperator,
                              clang::UnaryOperator::Opcode replacementOpCode)
      : unaryOperator(unaryOperator), replacementOpCode(replacementOpCode) {}

  void performMutation(ASTMutationPoint &mutation, ASTMutator &mutator) {
    mutator.performUnaryOperatorOpcodeMutation(mutation, *this);
  }

  ~UnaryOperatorOpcodeMutation() {}
};

class UnaryOperatorRemovalMutation : public ASTMutation {
public:
  clang::UnaryOperator *unaryOperator;

  UnaryOperatorRemovalMutation(clang::UnaryOperator *unaryOperator)
      : unaryOperator(unaryOperator) {}
  void performMutation(ASTMutationPoint &mutation, ASTMutator &mutator) {
    mutator.performUnaryOperatorRemovalMutation(mutation, *this);
  }
  ~UnaryOperatorRemovalMutation() {}
};

class ReplaceNumericAssignmentMutation : public ASTMutation {

public:
  clang::BinaryOperator *assignmentBinaryOperator;

  ReplaceNumericAssignmentMutation(clang::BinaryOperator *assignmentBinaryOperator)
      : assignmentBinaryOperator(assignmentBinaryOperator) {}

  void performMutation(ASTMutationPoint &mutation, ASTMutator &mutator) {
    mutator.performReplaceNumericAssignmentMutation(mutation, *this);
  }
};

/// Swaps which function a call expression calls, keeping the argument list
/// untouched: `f(a, b)` becomes `g(a, b)`.
///
/// Used for the Halide::BoundaryConditions family, whose members
/// (repeat_edge / repeat_image / mirror_image / mirror_interior) share an
/// overload set, so the swap is always well-typed. The replacement callee is
/// held by name rather than by FunctionDecl: the idiomatic entry points are
/// function templates, so the target has to be re-resolved through Sema
/// against the actual argument types at mutation time.
class HalideCalleeSwapMutation : public ASTMutation {
public:
  clang::CallExpr *callExpr;
  /// Unqualified name of the replacement callee, looked up in the same
  /// DeclContext as the original callee.
  std::string replacementCalleeName;

  HalideCalleeSwapMutation(clang::CallExpr *callExpr, std::string replacementCalleeName)
      : callExpr(callExpr), replacementCalleeName(std::move(replacementCalleeName)) {}

  void performMutation(ASTMutationPoint &mutation, ASTMutator &mutator) {
    mutator.performHalideCalleeSwapMutation(mutation, *this);
  }
  ~HalideCalleeSwapMutation() {}
};

/// Reorders two arguments of a call expression, keeping the callee: `f(a, b)`
/// becomes `f(b, a)`.
///
/// Used for the Halide API calls whose argument order carries the domain
/// meaning: select(condition, true_value, false_value) and
/// clamp(a, min_val, max_val).
class HalideArgumentSwapMutation : public ASTMutation {
public:
  clang::CallExpr *callExpr;
  unsigned firstArgumentIndex;
  unsigned secondArgumentIndex;

  HalideArgumentSwapMutation(clang::CallExpr *callExpr, unsigned firstArgumentIndex,
                             unsigned secondArgumentIndex)
      : callExpr(callExpr), firstArgumentIndex(firstArgumentIndex),
        secondArgumentIndex(secondArgumentIndex) {}

  void performMutation(ASTMutationPoint &mutation, ASTMutator &mutator) {
    mutator.performHalideArgumentSwapMutation(mutation, *this);
  }
  ~HalideArgumentSwapMutation() {}
};

/// Rewrites a call to Halide::select into the lazy Halide::Internal::Call
/// intrinsic if_then_else, which evaluates only the branch it takes.
///
/// One-directional by necessity: there is no public Halide::if_then_else free
/// function, so no user code can contain a call to swap back from.
class HalideSelectToIfThenElseMutation : public ASTMutation {
public:
  clang::CallExpr *callExpr;

  HalideSelectToIfThenElseMutation(clang::CallExpr *callExpr) : callExpr(callExpr) {}

  void performMutation(ASTMutationPoint &mutation, ASTMutator &mutator) {
    mutator.performHalideSelectToIfThenElseMutation(mutation, *this);
  }
  ~HalideSelectToIfThenElseMutation() {}
};

class ReplaceNumericInitAssignmentMutation : public ASTMutation {

public:
  clang::VarDecl *varDecl;

  ReplaceNumericInitAssignmentMutation(clang::VarDecl *varDecl) : varDecl(varDecl) {}

  void performMutation(ASTMutationPoint &mutation, ASTMutator &mutator) {
    mutator.performReplaceNumericInitAssignmentMutation(mutation, *this);
  }
};

} // namespace cxx
} // namespace mull
