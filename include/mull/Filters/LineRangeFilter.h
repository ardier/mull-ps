#pragma once

#include "mull/Config/ConfigurationOptions.h"
#include "mull/Filters/MutationPointFilter.h"

#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Regex.h>

#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace mull {

class MutationPoint;

/// Keeps only the mutation points whose source line falls inside one of a set
/// of configured line ranges.
///
/// This exists as a cost lever, not as a way of scoping what gets tested. The
/// clone phase copies the enclosing function once per surviving point and
/// retains every copy, so on the largest machine-generated translation units it
/// cannot finish at all; confining a run to part of a file is one of the few
/// ways to get such a file through. It is off by default and should stay off
/// unless a file is otherwise unrunnable, because a point that is never
/// generated is a result that can never be recovered later.
///
/// Line ranges rather than paths because the files this is for have no internal
/// structure a path filter can see: a machine-generated translation unit is one
/// file, with no `#line` directives and no provenance comments, so a line range
/// is the only handle on a sub-part of it.
///
/// A point is kept if it falls inside ANY range, so ranges are a union and may
/// overlap. Points with no line information are dropped, since they cannot be
/// shown to be inside any range -- in a normal pipeline the no-debug-info filter
/// has already removed them.
class LineRangeFilter : public MutationPointFilter {
public:
  /// Callers are expected to have validated the ranges;
  /// Filters::enableLineRangeFilter does. Invalid regexes are dropped at
  /// construction time (with `valid` set false) rather than matching nothing
  /// silently.
  explicit LineRangeFilter(const std::vector<LineRangeConfig> &ranges);

  bool shouldSkip(MutationPoint *point) override;

  /// Whether `line` in `filePath` is inside any configured range. Exposed so
  /// the decision can be tested without building a Module.
  bool contains(const std::string &filePath, size_t line);

  std::string name() override;
  ~LineRangeFilter() override = default;

private:
  struct Range {
    llvm::Regex regex;
    unsigned from;
    unsigned to;
  };

  std::vector<Range> ranges;
  size_t configuredCount;

  /// One file supplies tens of thousands of points, so the regex work is done
  /// once per path. Guarded like FilePathFilter's cache: the filter runs on
  /// every worker at once.
  std::unordered_map<std::string, std::vector<std::pair<unsigned, unsigned>>> cache;
  std::mutex cacheMutex;
};

} // namespace mull
