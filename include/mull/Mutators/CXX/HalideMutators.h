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

} // namespace cxx
} // namespace mull