#pragma once

#include "mull/Mutators/CXX/TrivialCXXMutator.h"

namespace mull {
namespace cxx {

// TODO add mutators
// Add ->
class ReplaceHalideAddToMulCall : public TrivialCXXMutator {
public:
  static std::string ID();
  ReplaceHalideAddToMulCall();
};

class ReplaceHalideAddToSubCall : public TrivialCXXMutator {
public:
  static std::string ID();
  ReplaceHalideAddToSubCall();
};

class ReplaceHalideAddToDivCall : public TrivialCXXMutator {
public:
  static std::string ID();
  ReplaceHalideAddToDivCall();
};

// sub ->
class ReplaceHalideSubToMulCall : public TrivialCXXMutator {
public:
  static std::string ID();
  ReplaceHalideSubToMulCall();
};

class ReplaceHalideSubToAddCall : public TrivialCXXMutator {
public:
  static std::string ID();
  ReplaceHalideSubToAddCall();
};

class ReplaceHalideSubToDivCall : public TrivialCXXMutator {
public:
  static std::string ID();
  ReplaceHalideSubToDivCall();
};

// Mul ->
class ReplaceHalideMulToAddCall : public TrivialCXXMutator {
public:
  static std::string ID();
  ReplaceHalideMulToAddCall();
};

class ReplaceHalideMulToSubCall : public TrivialCXXMutator {
public:
  static std::string ID();
  ReplaceHalideMulToSubCall();
};

class ReplaceHalideMulToDivCall : public TrivialCXXMutator {
public:
  static std::string ID();
  ReplaceHalideMulToDivCall();
};

// Div ->
class ReplaceHalideDivToMulCall : public TrivialCXXMutator {
public:
  static std::string ID();
  ReplaceHalideDivToMulCall();
};

class ReplaceHalideDivToSubCall : public TrivialCXXMutator {
public:
  static std::string ID();
  ReplaceHalideDivToSubCall();
};

class ReplaceHalideDivToAddCall : public TrivialCXXMutator {
public:
  static std::string ID();
  ReplaceHalideDivToAddCall();
};

// Schedule-directive swaps. Unlike the arithmetic operators above these have no
// C++ or GPL sibling: reordering vectorize/unroll/parallel, or moving a Func's
// compute level away from its storage level, is only expressible in the DSL.
class ReplaceHalideVectorizeToUnrollCall : public TrivialCXXMutator {
public:
  static std::string ID();
  ReplaceHalideVectorizeToUnrollCall();
};

class ReplaceHalideVectorizeToParallelCall : public TrivialCXXMutator {
public:
  static std::string ID();
  ReplaceHalideVectorizeToParallelCall();
};

class ReplaceHalideUnrollToVectorizeCall : public TrivialCXXMutator {
public:
  static std::string ID();
  ReplaceHalideUnrollToVectorizeCall();
};

class ReplaceHalideUnrollToParallelCall : public TrivialCXXMutator {
public:
  static std::string ID();
  ReplaceHalideUnrollToParallelCall();
};

class ReplaceHalideParallelToVectorizeCall : public TrivialCXXMutator {
public:
  static std::string ID();
  ReplaceHalideParallelToVectorizeCall();
};

class ReplaceHalideParallelToUnrollCall : public TrivialCXXMutator {
public:
  static std::string ID();
  ReplaceHalideParallelToUnrollCall();
};

class ReplaceHalideComputeAtToStoreAtCall : public TrivialCXXMutator {
public:
  static std::string ID();
  ReplaceHalideComputeAtToStoreAtCall();
};

class ReplaceHalideStoreAtToComputeAtCall : public TrivialCXXMutator {
public:
  static std::string ID();
  ReplaceHalideStoreAtToComputeAtCall();
};

// Generated swap operators -- see HalideGeneratedMutators.def.
#define HALIDE_GEN_MUTATOR(KindName, ClassName, IdString, IrmClass, Description)                   \
  class ClassName : public TrivialCXXMutator {                                                     \
  public:                                                                                          \
    static std::string ID();                                                                       \
    ClassName();                                                                                   \
  };
#include "mull/Mutators/CXX/HalideGeneratedMutators.def"

} // namespace cxx
} // namespace mull