#pragma once

#include "mull/Mutators/MutatorKind.h"

#include <unordered_map>

namespace mull {
namespace cxx {

struct MutationIdentifier {
  std::string identifier;
  mull::MutatorKind mutatorKind;
  /// Whether the mutation belongs to the set enabled when no mutators are
  /// configured at all. The novel Halide operators stay opt-in.
  bool enabledByDefault;
  MutationIdentifier(std::string identifier, mull::MutatorKind mutatorKind,
                     bool enabledByDefault = true)
      : identifier(identifier), mutatorKind(mutatorKind), enabledByDefault(enabledByDefault) {}
};

class MutationMap {
  std::unordered_set<mull::MutatorKind> usedMutatorSet;
  std::unordered_map<mull::MutatorKind, std::string> mapKindsToIdentifiers;
  std::unordered_map<std::string, mull::MutatorKind> mapIdentifiersToKinds;

public:
  MutationMap();
  bool isValidMutation(mull::MutatorKind mutatorKind) const;
  std::string getIdentifier(mull::MutatorKind mutatorKind);
  void addMutation(std::string identifier);
  void setDefaultMutationsIfNotSpecified();

  /// True when at least one mutation that can only occur inside a class member
  /// function is enabled. Only then does the plugin descend into namespaces and
  /// class bodies looking for mutation points; without it the traversal stays
  /// exactly where it has always been (top-level function definitions).
  bool needsDeepDeclTraversal() const;
};

} // namespace cxx
} // namespace mull
