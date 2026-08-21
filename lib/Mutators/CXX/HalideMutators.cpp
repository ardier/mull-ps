#include "mull/Mutators/CXX/HalideMutators.h"

using namespace mull;
using namespace mull::cxx;

// TODO add mutants

static std::vector<std::unique_ptr<irm::IRMutation>> getReplaceHalideAddToMulCall() {
  std::vector<std::unique_ptr<irm::IRMutation>> mutators;
  mutators.push_back(std::make_unique<irm::Halide_AddToMul>());
  return mutators;
}

std::string ReplaceHalideAddToMulCall::ID() {
  // TODO note that this is the name used to filter the mutator
  return "Halide_add_to_mul";
}

ReplaceHalideAddToMulCall::ReplaceHalideAddToMulCall()
    : TrivialCXXMutator(getReplaceHalideAddToMulCall(),
                        MutatorKind::Halide_ReplaceHalideAddToMulCall,
                        ReplaceHalideAddToMulCall::ID(), "Replaces call to a halide add to mul",
                        "*", "Replaced call to a halide add with mul") {}

static std::vector<std::unique_ptr<irm::IRMutation>> getReplaceHalideAddToSubCall() {
  std::vector<std::unique_ptr<irm::IRMutation>> mutators;
  mutators.push_back(std::make_unique<irm::Halide_AddToSub>());
  return mutators;
}

std::string ReplaceHalideAddToSubCall::ID() {
  // TODO note that this is the name used to filter the mutator
  return "Halide_add_to_sub";
}

ReplaceHalideAddToSubCall::ReplaceHalideAddToSubCall()
    : TrivialCXXMutator(getReplaceHalideAddToSubCall(),
                        MutatorKind::Halide_ReplaceHalideAddToSubCall,
                        ReplaceHalideAddToSubCall::ID(), "Replaces call to a halide add to sub",
                        "-", "Replaced call to a halide add with sub") {}

static std::vector<std::unique_ptr<irm::IRMutation>> getReplaceHalideAddToDivCall() {
  std::vector<std::unique_ptr<irm::IRMutation>> mutators;
  mutators.push_back(std::make_unique<irm::Halide_AddToDiv>());
  return mutators;
}

std::string ReplaceHalideAddToDivCall::ID() {
  // TODO note that this is the name used to filter the mutator
  return "Halide_add_to_div";
}

ReplaceHalideAddToDivCall::ReplaceHalideAddToDivCall()
    : TrivialCXXMutator(getReplaceHalideAddToDivCall(),
                        MutatorKind::Halide_ReplaceHalideAddToDivCall,
                        ReplaceHalideAddToDivCall::ID(), "Replaces call to a halide add to div",
                        "/", "Replaced call to a halide add with div") {}

// sub ->
static std::vector<std::unique_ptr<irm::IRMutation>> getReplaceHalideSubToMulCall() {
  std::vector<std::unique_ptr<irm::IRMutation>> mutators;
  mutators.push_back(std::make_unique<irm::Halide_SubToMul>());
  return mutators;
}

std::string ReplaceHalideSubToMulCall::ID() {
  // TODO note that this is the name used to filter the mutator
  return "Halide_sub_to_mul";
}

ReplaceHalideSubToMulCall::ReplaceHalideSubToMulCall()
    : TrivialCXXMutator(getReplaceHalideSubToMulCall(),
                        MutatorKind::Halide_ReplaceHalideSubToMulCall,
                        ReplaceHalideSubToMulCall::ID(), "Replaces call to a halide sub to mul",
                        "*", "Replaced call to a halide sub with mul") {}

static std::vector<std::unique_ptr<irm::IRMutation>> getReplaceHalideSubToAddCall() {
  std::vector<std::unique_ptr<irm::IRMutation>> mutators;
  mutators.push_back(std::make_unique<irm::Halide_SubToAdd>());
  return mutators;
}

std::string ReplaceHalideSubToAddCall::ID() {
  // TODO note that this is the name used to filter the mutator
  return "Halide_sub_to_add";
}

ReplaceHalideSubToAddCall::ReplaceHalideSubToAddCall()
    : TrivialCXXMutator(getReplaceHalideSubToAddCall(),
                        MutatorKind::Halide_ReplaceHalideSubToAddCall,
                        ReplaceHalideSubToAddCall::ID(), "Replaces call to a halide sub to add",
                        "+", "Replaced call to a halide sub with add") {}

static std::vector<std::unique_ptr<irm::IRMutation>> getReplaceHalideSubToDivCall() {
  std::vector<std::unique_ptr<irm::IRMutation>> mutators;
  mutators.push_back(std::make_unique<irm::Halide_SubToDiv>());
  return mutators;
}

std::string ReplaceHalideSubToDivCall::ID() {
  // TODO note that this is the name used to filter the mutator
  return "Halide_sub_to_div";
}

ReplaceHalideSubToDivCall::ReplaceHalideSubToDivCall()
    : TrivialCXXMutator(getReplaceHalideSubToDivCall(),
                        MutatorKind::Halide_ReplaceHalideSubToDivCall,
                        ReplaceHalideSubToDivCall::ID(), "Replaces call to a halide sub to div",
                        "/", "Replaced call to a halide sub with div") {}

// Mul ->

static std::vector<std::unique_ptr<irm::IRMutation>> getReplaceHalideMulToAddCall() {
  std::vector<std::unique_ptr<irm::IRMutation>> mutators;
  mutators.push_back(std::make_unique<irm::Halide_MulToAdd>());
  return mutators;
}

std::string ReplaceHalideMulToAddCall::ID() {
  // TODO note that this is the name used to filter the mutator
  return "Halide_mul_to_add";
}

ReplaceHalideMulToAddCall::ReplaceHalideMulToAddCall()
    : TrivialCXXMutator(getReplaceHalideMulToAddCall(),
                        MutatorKind::Halide_ReplaceHalideMulToAddCall,
                        ReplaceHalideMulToAddCall::ID(), "Replaces call to a halide mul to add",
                        "+", "Replaced call to a halide mul with add") {}

static std::vector<std::unique_ptr<irm::IRMutation>> getReplaceHalideMulToSubCall() {

  std::vector<std::unique_ptr<irm::IRMutation>> mutators;
  mutators.push_back(std::make_unique<irm::Halide_MulToSub>());
  return mutators;
}

std::string ReplaceHalideMulToSubCall::ID() {
  // TODO note that this is the name used to filter the mutator
  return "Halide_mul_to_sub";
}

ReplaceHalideMulToSubCall::ReplaceHalideMulToSubCall()
    : TrivialCXXMutator(getReplaceHalideMulToSubCall(),
                        MutatorKind::Halide_ReplaceHalideMulToSubCall,
                        ReplaceHalideMulToSubCall::ID(), "Replaces call to a halide mul to sub",
                        "-", "Replaced call to a halide mul with sub") {}

static std::vector<std::unique_ptr<irm::IRMutation>> getReplaceHalideMulToDivCall() {
  std::vector<std::unique_ptr<irm::IRMutation>> mutators;
  mutators.push_back(std::make_unique<irm::Halide_MulToDiv>());
  return mutators;
}

std::string ReplaceHalideMulToDivCall::ID() {
  // TODO note that this is the name used to filter the mutator
  return "Halide_mul_to_div";
}

ReplaceHalideMulToDivCall::ReplaceHalideMulToDivCall()
    : TrivialCXXMutator(getReplaceHalideMulToDivCall(),
                        MutatorKind::Halide_ReplaceHalideMulToDivCall,
                        ReplaceHalideMulToDivCall::ID(), "Replaces call to a halide mul to div",
                        "/", "Replaced call to a halide mul with div") {}

// Div ->
static std::vector<std::unique_ptr<irm::IRMutation>> getReplaceHalideDivToAddCall() {
  std::vector<std::unique_ptr<irm::IRMutation>> mutators;
  mutators.push_back(std::make_unique<irm::Halide_DivToAdd>());
  return mutators;
}

std::string ReplaceHalideDivToAddCall::ID() {
  // TODO note that this is the name used to filter the mutator
  return "Halide_div_to_add";
}

ReplaceHalideDivToAddCall::ReplaceHalideDivToAddCall()
    : TrivialCXXMutator(getReplaceHalideDivToAddCall(),
                        MutatorKind::Halide_ReplaceHalideDivToAddCall,
                        ReplaceHalideDivToAddCall::ID(), "Replaces call to a halide div to add",
                        "+", "Replaced call to a halide div with add") {}

static std::vector<std::unique_ptr<irm::IRMutation>> getReplaceHalideDivToSubCall() {
  std::vector<std::unique_ptr<irm::IRMutation>> mutators;
  mutators.push_back(std::make_unique<irm::Halide_DivToSub>());
  return mutators;
}

std::string ReplaceHalideDivToSubCall::ID() {
  // TODO note that this is the name used to filter the mutator
  return "Halide_div_to_sub";
}

ReplaceHalideDivToSubCall::ReplaceHalideDivToSubCall()
    : TrivialCXXMutator(getReplaceHalideDivToSubCall(),
                        MutatorKind::Halide_ReplaceHalideDivToSubCall,
                        ReplaceHalideDivToSubCall::ID(), "Replaces call to a halide div to sub",
                        "-", "Replaced call to a halide div with sub") {}

static std::vector<std::unique_ptr<irm::IRMutation>> getReplaceHalideDivToMulCall() {

  std::vector<std::unique_ptr<irm::IRMutation>> mutators;
  mutators.push_back(std::make_unique<irm::Halide_DivToMul>());
  return mutators;
}

std::string ReplaceHalideDivToMulCall::ID() {
  // TODO note that this is the name used to filter the mutator
  return "Halide_div_to_mul";
}

ReplaceHalideDivToMulCall::ReplaceHalideDivToMulCall()
    : TrivialCXXMutator(getReplaceHalideDivToMulCall(),
                        MutatorKind::Halide_ReplaceHalideDivToMulCall,
                        ReplaceHalideDivToMulCall::ID(), "Replaces call to a halide div to mul",
                        "*", "Replaced call to a halide div with mul") {}

static std::vector<std::unique_ptr<irm::IRMutation>> getReplaceHalideVectorizeToUnrollCall() {
  std::vector<std::unique_ptr<irm::IRMutation>> mutators;
  mutators.push_back(std::make_unique<irm::Halide_vectorize_to_unroll>());
  return mutators;
}

std::string ReplaceHalideVectorizeToUnrollCall::ID() {
  return "Halide_vectorize_to_unroll";
}

ReplaceHalideVectorizeToUnrollCall::ReplaceHalideVectorizeToUnrollCall()
    : TrivialCXXMutator(getReplaceHalideVectorizeToUnrollCall(), MutatorKind::Halide_ReplaceVectorizeToUnrollCall, ReplaceHalideVectorizeToUnrollCall::ID(),
                        "Replaces a halide vectorize with unroll", "*",
                        "Replaced a halide vectorize with unroll") {}

static std::vector<std::unique_ptr<irm::IRMutation>> getReplaceHalideVectorizeToParallelCall() {
  std::vector<std::unique_ptr<irm::IRMutation>> mutators;
  mutators.push_back(std::make_unique<irm::Halide_vectorize_to_parallel>());
  return mutators;
}

std::string ReplaceHalideVectorizeToParallelCall::ID() {
  return "Halide_vectorize_to_parallel";
}

ReplaceHalideVectorizeToParallelCall::ReplaceHalideVectorizeToParallelCall()
    : TrivialCXXMutator(getReplaceHalideVectorizeToParallelCall(), MutatorKind::Halide_ReplaceVectorizeToParallelCall, ReplaceHalideVectorizeToParallelCall::ID(),
                        "Replaces a halide vectorize with parallel", "*",
                        "Replaced a halide vectorize with parallel") {}

static std::vector<std::unique_ptr<irm::IRMutation>> getReplaceHalideUnrollToVectorizeCall() {
  std::vector<std::unique_ptr<irm::IRMutation>> mutators;
  mutators.push_back(std::make_unique<irm::Halide_unroll_to_vectorize>());
  return mutators;
}

std::string ReplaceHalideUnrollToVectorizeCall::ID() {
  return "Halide_unroll_to_vectorize";
}

ReplaceHalideUnrollToVectorizeCall::ReplaceHalideUnrollToVectorizeCall()
    : TrivialCXXMutator(getReplaceHalideUnrollToVectorizeCall(), MutatorKind::Halide_ReplaceUnrollToVectorizeCall, ReplaceHalideUnrollToVectorizeCall::ID(),
                        "Replaces a halide unroll with vectorize", "*",
                        "Replaced a halide unroll with vectorize") {}

static std::vector<std::unique_ptr<irm::IRMutation>> getReplaceHalideUnrollToParallelCall() {
  std::vector<std::unique_ptr<irm::IRMutation>> mutators;
  mutators.push_back(std::make_unique<irm::Halide_unroll_to_parallel>());
  return mutators;
}

std::string ReplaceHalideUnrollToParallelCall::ID() {
  return "Halide_unroll_to_parallel";
}

ReplaceHalideUnrollToParallelCall::ReplaceHalideUnrollToParallelCall()
    : TrivialCXXMutator(getReplaceHalideUnrollToParallelCall(), MutatorKind::Halide_ReplaceUnrollToParallelCall, ReplaceHalideUnrollToParallelCall::ID(),
                        "Replaces a halide unroll with parallel", "*",
                        "Replaced a halide unroll with parallel") {}

static std::vector<std::unique_ptr<irm::IRMutation>> getReplaceHalideParallelToVectorizeCall() {
  std::vector<std::unique_ptr<irm::IRMutation>> mutators;
  mutators.push_back(std::make_unique<irm::Halide_parallel_to_vectorize>());
  return mutators;
}

std::string ReplaceHalideParallelToVectorizeCall::ID() {
  return "Halide_parallel_to_vectorize";
}

ReplaceHalideParallelToVectorizeCall::ReplaceHalideParallelToVectorizeCall()
    : TrivialCXXMutator(getReplaceHalideParallelToVectorizeCall(), MutatorKind::Halide_ReplaceParallelToVectorizeCall, ReplaceHalideParallelToVectorizeCall::ID(),
                        "Replaces a halide parallel with vectorize", "*",
                        "Replaced a halide parallel with vectorize") {}

static std::vector<std::unique_ptr<irm::IRMutation>> getReplaceHalideParallelToUnrollCall() {
  std::vector<std::unique_ptr<irm::IRMutation>> mutators;
  mutators.push_back(std::make_unique<irm::Halide_parallel_to_unroll>());
  return mutators;
}

std::string ReplaceHalideParallelToUnrollCall::ID() {
  return "Halide_parallel_to_unroll";
}

ReplaceHalideParallelToUnrollCall::ReplaceHalideParallelToUnrollCall()
    : TrivialCXXMutator(getReplaceHalideParallelToUnrollCall(), MutatorKind::Halide_ReplaceParallelToUnrollCall, ReplaceHalideParallelToUnrollCall::ID(),
                        "Replaces a halide parallel with unroll", "*",
                        "Replaced a halide parallel with unroll") {}

static std::vector<std::unique_ptr<irm::IRMutation>> getReplaceHalideComputeAtToStoreAtCall() {
  std::vector<std::unique_ptr<irm::IRMutation>> mutators;
  mutators.push_back(std::make_unique<irm::Halide_compute_at_to_store_at>());
  return mutators;
}

std::string ReplaceHalideComputeAtToStoreAtCall::ID() {
  return "Halide_compute_at_to_store_at";
}

ReplaceHalideComputeAtToStoreAtCall::ReplaceHalideComputeAtToStoreAtCall()
    : TrivialCXXMutator(getReplaceHalideComputeAtToStoreAtCall(), MutatorKind::Halide_ReplaceComputeAtToStoreAtCall, ReplaceHalideComputeAtToStoreAtCall::ID(),
                        "Replaces a halide compute_at with store_at", "*",
                        "Replaced a halide compute_at with store_at") {}

static std::vector<std::unique_ptr<irm::IRMutation>> getReplaceHalideStoreAtToComputeAtCall() {
  std::vector<std::unique_ptr<irm::IRMutation>> mutators;
  mutators.push_back(std::make_unique<irm::Halide_store_at_to_compute_at>());
  return mutators;
}

std::string ReplaceHalideStoreAtToComputeAtCall::ID() {
  return "Halide_store_at_to_compute_at";
}

ReplaceHalideStoreAtToComputeAtCall::ReplaceHalideStoreAtToComputeAtCall()
    : TrivialCXXMutator(getReplaceHalideStoreAtToComputeAtCall(), MutatorKind::Halide_ReplaceStoreAtToComputeAtCall, ReplaceHalideStoreAtToComputeAtCall::ID(),
                        "Replaces a halide store_at with compute_at", "*",
                        "Replaced a halide store_at with compute_at") {}

/// Generated swap operators. Each expands to the same shape as the hand-written
/// mutators above: a factory returning the irm mutation, an ID, and a
/// TrivialCXXMutator constructor.
#define HALIDE_GEN_MUTATOR(KindName, ClassName, IdString, IrmClass, Description)                   \
  static std::vector<std::unique_ptr<irm::IRMutation>> get##ClassName() {                          \
    std::vector<std::unique_ptr<irm::IRMutation>> mutators;                                        \
    mutators.push_back(std::make_unique<irm::IrmClass>());                                         \
    return mutators;                                                                               \
  }                                                                                                \
  std::string ClassName::ID() {                                                                    \
    return IdString;                                                                               \
  }                                                                                                \
  ClassName::ClassName()                                                                           \
      : TrivialCXXMutator(get##ClassName(), MutatorKind::KindName, ClassName::ID(), Description,    \
                          "*", Description) {}
#include "mull/Mutators/CXX/HalideGeneratedMutators.def"
