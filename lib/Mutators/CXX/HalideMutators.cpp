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
