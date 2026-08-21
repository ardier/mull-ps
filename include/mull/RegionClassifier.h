#pragma once

#include "mull/Config/ConfigurationOptions.h"

#include <map>
#include <string>

namespace mull {

class Diagnostics;

/// Which part of a machine-generated translation unit a source line falls in.
///
/// The three names are the vocabulary the emitted-C++ analysis already uses,
/// and are reproduced verbatim so a dump joins against the existing per-mutant
/// CSVs instead of introducing a second set of labels.
enum class Region {
  /// No boundary is known for the file: either region tagging is switched off
  /// or the markers could not be found.
  Unknown,
  /// The shared runtime / SIMD-emulation prefix. Byte-identical source across
  /// every emitted file, but each pipeline exercises a different part of it,
  /// so these points are not redundant between apps.
  Boilerplate,
  /// The generator's own lowered pipeline.
  GeneratorSpecific,
  /// The trailing `_argv` and `_metadata` wrappers.
  WrapperMetadata,
};

/// The label as written into a dump. Stable: downstream data joins on it.
const char *regionName(Region region);

/// First and last line of each region, 1-based and inclusive. A region is empty
/// when its end equals the previous region's end.
struct RegionBoundaries {
  size_t boilerplateEnd = 0;
  size_t generatorSpecificEnd = 0;
};

/// Maps (file, line) to a region.
///
/// Boundaries come from one of two places, never from a constant in the source:
///
///   1. an explicit `boundaries` entry in the configuration whose `file` regex
///      matches the path, or
///   2. `autodetect`, which reads the file and applies the same marker-based
///      definition the analysis scripts use.
///
/// Explicit boundaries win, so a file whose markers are unusual can always be
/// pinned by hand. With neither, every line classifies as Unknown, which is the
/// default and reads no files at all.
///
/// Detection results are cached per path. Not thread-safe: it is used from the
/// single-threaded dump step.
class RegionClassifier {
public:
  RegionClassifier(Diagnostics &diagnostics, const RegionsConfig &config);

  Region classify(const std::string &filePath, size_t line);

  /// The boundaries in force for `filePath`, detecting them if necessary.
  /// `known` is false when the file has no usable boundary.
  const RegionBoundaries *boundariesFor(const std::string &filePath);

  /// The marker-based definition, factored out so it can be tested against a
  /// literal file body: the last `namespaceCloseMarker` line before the first
  /// `functionAttrsMarker` line ends the boilerplate, and the second
  /// `functionAttrsMarker` line ends the generator-specific body. At least
  /// three `functionAttrsMarker` lines must be present (the pipeline entry
  /// point, the `_argv` wrapper and the `_metadata` accessor), which is what
  /// makes this recognisably an emitted file rather than ordinary source.
  ///
  /// Returns false when the markers are not found, leaving `boundaries`
  /// untouched.
  static bool detectBoundaries(const std::string &contents, const std::string &namespaceCloseMarker,
                               const std::string &functionAttrsMarker,
                               RegionBoundaries &boundaries);

  /// Classification given known boundaries. Exposed for testing.
  static Region classify(size_t line, const RegionBoundaries &boundaries);

private:
  Diagnostics &diagnostics;
  const RegionsConfig &config;
  std::map<std::string, RegionBoundaries> detected;
  std::map<std::string, bool> known;
};

} // namespace mull
