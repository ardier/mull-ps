#pragma once

namespace mull {

enum class IDEDiagnosticsKind { None, Survived, Killed, All };

struct ParallelizationConfig {
  unsigned workers;
  unsigned executionWorkers;
  ParallelizationConfig();
  static ParallelizationConfig defaultConfig();
  void normalize();
  bool exceedsHardware();
};

/// Mutant slicing: keep only the mutation points that hash into slice `index`
/// of `count`. `specified` is set by the YAML parser when the `slice:` key is
/// present at all -- without it a `count` of 0 would be indistinguishable from
/// an absent key, and an explicit `count: 0` must be a hard error rather than a
/// silent no-op.
struct SliceConfig {
  unsigned index = 0;
  unsigned count = 0;
  bool specified = false;
};

struct DebugConfig {
  bool printIR = false;
  bool printIRBefore = false;
  bool printIRAfter = false;
  bool printIRToFile = false;
  bool traceMutants = false;
  bool coverage = false;
  bool gitDiff = false;
  bool filters = false;
  bool slowIRVerification = false;
};

} // namespace mull
