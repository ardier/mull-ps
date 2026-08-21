#pragma once

#include <string>
#include <vector>

#include "mull/Config/ConfigurationOptions.h"

namespace mull {

extern int MullDefaultTimeoutMilliseconds;

class Diagnostics;

struct Configuration {
  std::string pathOnDisk;
  bool debugEnabled;
  bool quiet;
  bool silent;
  bool dryRunEnabled;
  bool captureTestOutput;
  bool captureMutantOutput;
  bool includeNotCovered;
  bool junkDetectionDisabled;

  unsigned timeout;

  IDEDiagnosticsKind diagnostics;

  std::vector<std::string> mutators;
  std::vector<std::string> ignoreMutators;

  std::string executable;

  std::string compilationDatabasePath;
  std::vector<std::string> compilerFlags;

  std::vector<std::string> includePaths;
  std::vector<std::string> excludePaths;

  ParallelizationConfig parallelization;

  std::string gitDiffRef;
  std::string gitProjectRoot;

  SliceConfig slice{};

  /// Confine mutation to these line ranges. Empty means the filter is never
  /// installed. See LineRangeFilter.
  std::vector<LineRangeConfig> lineRanges;

  /// Path *prefix* for the mutant-population dump written after filtering and
  /// before the clone phase. Empty means no dump, which is the default and a
  /// complete no-op. See MutantDump.
  std::string dumpMutantsTo;

  /// Stop after writing the dump, leaving the module unmutated. Only has an
  /// effect together with `dumpMutantsTo`.
  bool dumpOnly;

  /// Region tagging for the dump. Absent means every record is tagged
  /// `unknown` and nothing reads the source files.
  RegionsConfig regions{};

  DebugConfig debug{};

  Configuration();

  static std::string findConfig(Diagnostics &diagnostics);
  static Configuration loadFromDisk(Diagnostics &diagnostics, const std::string &path);
};

} // namespace mull
