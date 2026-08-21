#pragma once

#include <limits>
#include <string>
#include <vector>

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

/// One line range to confine mutation to. `file` is a regex matched against the
/// mutation point's source file path; `from` and `to` are 1-based and
/// inclusive. `to` defaults to "the end of the file", so it may be omitted.
///
/// Off by default and meant as a cost lever for files whose clone phase cannot
/// finish at all -- not as a way of scoping what gets tested, since a point
/// that is never generated cannot be recovered afterwards.
struct LineRangeConfig {
  std::string file;
  unsigned from = 0;
  unsigned to = std::numeric_limits<unsigned>::max();
};

/// One explicit boilerplate/generator boundary, for a set of files matched by
/// regex. Line numbers are 1-based and inclusive, matching the line numbers in
/// a mutation point's user identifier.
struct RegionBoundaryConfig {
  std::string file;
  unsigned boilerplateEnd = 0;
  unsigned generatorSpecificEnd = 0;
};

/// Region tagging for the mutant dump. Halide's C backend emits each app as a
/// single file whose leading ~3.4k lines are a byte-identical SIMD-emulation
/// prefix shared by every app, followed by the generator's own pipeline code
/// and finally the argv/metadata wrappers. There are no #line directives and no
/// provenance comments, so the split has to be recovered from line ranges.
///
/// The boundary differs per file (blur and camera_pipe are not harris), so it
/// is never hardcoded: it either comes from `boundaries` or is derived from the
/// file itself by `autodetect`, which reproduces the marker-based definition
/// already used by the analysis scripts -- the last `namespaceCloseMarker` line
/// before the first `functionAttrsMarker` line ends the boilerplate, and the
/// second `functionAttrsMarker` line ends the generator-specific body.
struct RegionsConfig {
  bool specified = false;
  bool autodetect = false;
  std::string namespaceCloseMarker = "}  // namespace";
  std::string functionAttrsMarker = "HALIDE_FUNCTION_ATTRS";
  std::vector<RegionBoundaryConfig> boundaries;
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
