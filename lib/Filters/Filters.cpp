#include "mull/Filters/Filters.h"
#include "mull/Config/Configuration.h"
#include "mull/Diagnostics/Diagnostics.h"
#include "mull/Filters/BlockAddressFunctionFilter.h"
#include "mull/Filters/CoverageFilter.h"
#include "mull/Filters/FilePathFilter.h"
#include "mull/Filters/GitDiffFilter.h"
#include "mull/Filters/LineRangeFilter.h"
#include "mull/Filters/NoDebugInfoFilter.h"
#include "mull/Filters/SliceFilter.h"
#include "mull/Filters/VariadicFunctionFilter.h"
#include <limits>
#include <llvm/Support/FileSystem.h>
#include <sstream>

using namespace mull;
using namespace std::string_literals;

Filters::Filters(const Configuration &configuration, Diagnostics &diagnostics)
    : functionFilters(), mutationFilters(), instructionFilters(), configuration(configuration),
      diagnostics(diagnostics) {}

void Filters::enableNoDebugFilter() {
  auto *filter = new mull::NoDebugInfoFilter;

  mutationFilters.push_back(filter);
  functionFilters.push_back(filter);
  instructionFilters.push_back(filter);
  storage.emplace_back(filter);
}

void Filters::enableFilePathFilter() {
  if (configuration.includePaths.empty() && configuration.excludePaths.empty()) {
    if (configuration.debug.filters) {
      diagnostics.debug("FilePath: both includePaths and excludePaths are empty");
    }
    return;
  }

  auto *filter = new mull::FilePathFilter;
  storage.emplace_back(filter);
  mutationFilters.push_back(filter);
  mutantFilters.push_back(filter);
  functionFilters.push_back(filter);

  for (const auto &regex : configuration.excludePaths) {
    if (configuration.debug.filters) {
      diagnostics.debug("FilePath: excluding: "s + regex);
    }
    auto added = filter->exclude(regex);
    if (!added.first) {
      std::stringstream warningMessage;
      warningMessage << "Invalid regex for exclude-path: '" << regex
                     << "' has been ignored. Error: " << added.second;
      diagnostics.warning(warningMessage.str());
    }
  }
  for (const auto &regex : configuration.includePaths) {
    if (configuration.debug.filters) {
      diagnostics.debug("FilePath: including: "s + regex);
    }
    auto added = filter->include(regex);
    if (!added.first) {
      std::stringstream warningMessage;
      warningMessage << "Invalid regex for include-path: '" << regex
                     << "' has been ignored. Error: " << added.second;
      diagnostics.warning(warningMessage.str());
    }
  }
}

void Filters::enableGitDiffFilter() {
  if (!configuration.gitDiffRef.empty()) {
    if (configuration.gitProjectRoot.empty()) {
      std::stringstream debugMessage;
      debugMessage
          << "-git-diff-ref option has been provided but the path to the Git project root has not "
             "been specified via -git-project-root. The incremental testing will be disabled.";
      diagnostics.warning(debugMessage.str());
    } else if (!llvm::sys::fs::is_directory(configuration.gitProjectRoot)) {
      std::stringstream debugMessage;
      debugMessage << "directory provided by -git-project-root does not exist, ";
      debugMessage << "the incremental testing will be disabled: ";
      debugMessage << configuration.gitProjectRoot;
      diagnostics.warning(debugMessage.str());
    } else {
      std::string gitProjectRoot = configuration.gitProjectRoot;
      llvm::SmallString<256> tmpGitProjectRoot;
      if (!llvm::sys::fs::real_path(gitProjectRoot, tmpGitProjectRoot)) {
        gitProjectRoot = tmpGitProjectRoot.str();

        std::string gitDiffBranch = configuration.gitDiffRef;
        diagnostics.info(std::string("Incremental testing using Git Diff is enabled.\n") +
                         "- Git ref: " + gitDiffBranch + "\n" +
                         "- Git project root: " + gitProjectRoot);
        mull::GitDiffFilter *gitDiffFilter = mull::GitDiffFilter::createFromGitDiff(
            configuration, diagnostics, gitProjectRoot, gitDiffBranch);

        if (gitDiffFilter) {
          storage.emplace_back(gitDiffFilter);
          instructionFilters.push_back(gitDiffFilter);
          mutantFilters.push_back(gitDiffFilter);
        }
      } else {
        diagnostics.warning(
            std::string("could not expand -git-project-root to an absolute path: ") +
            gitProjectRoot);
      }
    }
  }
}

CoverageFilter *Filters::enableCoverageFilter(const std::string &profileName,
                                              const std::vector<std::string> &objects) {
  auto filter = new CoverageFilter(configuration, diagnostics, profileName, objects);
  storage.emplace_back(filter);
  mutantFilters.push_back(filter);
  return filter;
}

void Filters::enableBlockAddressFilter() {
  auto filter = new mull::BlockAddressFunctionFilter;
  storage.emplace_back(filter);
  functionFilters.push_back(filter);
}

void Filters::enableVariadicFunctionFilter() {
  auto filter = new mull::VariadicFunctionFilter;
  storage.emplace_back(filter);
  functionFilters.push_back(filter);
}

void Filters::enableLineRangeFilter() {
  if (configuration.lineRanges.empty()) {
    if (configuration.debug.filters) {
      diagnostics.debug("LineRange: no 'lineRanges' key in the configuration, line ranges are "
                        "disabled");
    }
    return;
  }

  /// Validated up front rather than per point: a degenerate range would
  /// otherwise just silently keep nothing, which looks exactly like a file with
  /// no mutants in it.
  bool usable = false;
  for (const LineRangeConfig &range : configuration.lineRanges) {
    std::string error;
    llvm::Regex regex(range.file);
    if (!regex.isValid(error)) {
      diagnostics.error("lineRanges: invalid 'file' regex '"s + range.file + "': " + error);
      continue;
    }
    if (range.from == 0) {
      diagnostics.error("lineRanges: 'from' must be at least 1, line numbers are 1-based. "
                        "Omit 'to' to run to the end of the file.");
      continue;
    }
    if (range.from > range.to) {
      std::stringstream errorMessage;
      errorMessage << "lineRanges: 'from' (" << range.from << ") must not be greater than 'to' ("
                   << range.to << ") for file regex '" << range.file << "'";
      diagnostics.error(errorMessage.str());
      continue;
    }

    std::stringstream infoMessage;
    infoMessage << "Line range enabled: keeping mutants in lines " << range.from << "..";
    if (range.to == std::numeric_limits<unsigned>::max()) {
      infoMessage << "end of file";
    } else {
      infoMessage << range.to;
    }
    infoMessage << " of files matching '" << range.file << "'";
    diagnostics.info(infoMessage.str());
    usable = true;
  }

  if (!usable) {
    diagnostics.error("lineRanges: no usable range in the configuration, so every mutant would be "
                      "filtered out. Remove the 'lineRanges' key entirely to disable it.");
    return;
  }

  auto *filter = new mull::LineRangeFilter(configuration.lineRanges);
  storage.emplace_back(filter);
  mutationFilters.push_back(filter);
}

void Filters::enableSliceFilter() {
  if (!configuration.slice.specified) {
    if (configuration.debug.filters) {
      diagnostics.debug("Slice: no 'slice' key in the configuration, slicing is disabled");
    }
    return;
  }

  const unsigned index = configuration.slice.index;
  const unsigned count = configuration.slice.count;

  if (count == 0) {
    diagnostics.error("slice: 'count' must be greater than 0, got 0. "
                      "Remove the 'slice' key entirely to disable slicing.");
    return;
  }
  if (index >= count) {
    std::stringstream errorMessage;
    errorMessage << "slice: 'index' must be less than 'count', got index " << index
                 << " with count " << count << " (valid indices are 0.." << (count - 1) << ")";
    diagnostics.error(errorMessage.str());
    return;
  }

  std::stringstream infoMessage;
  infoMessage << "Mutant slicing is enabled: keeping slice " << index << " of " << count;
  if (count == 1) {
    infoMessage << " (count is 1, so no mutants are skipped)";
  }
  diagnostics.info(infoMessage.str());

  auto *filter = new mull::SliceFilter(index, count);
  storage.emplace_back(filter);
  mutationFilters.push_back(filter);
}
