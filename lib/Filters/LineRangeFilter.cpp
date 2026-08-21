#include "mull/Filters/LineRangeFilter.h"

#include "mull/MutationPoint.h"

#include <sstream>

using namespace mull;

LineRangeFilter::LineRangeFilter(const std::vector<LineRangeConfig> &configured)
    : configuredCount(configured.size()) {
  for (const LineRangeConfig &entry : configured) {
    llvm::Regex regex(entry.file);
    std::string error;
    if (!regex.isValid(error)) {
      /// Dropped rather than kept as a never-matching range: a range that
      /// silently matches nothing would quietly delete an entire file's worth
      /// of mutants. Filters::enableLineRangeFilter reports the error.
      continue;
    }
    ranges.push_back(Range{ std::move(regex), entry.from, entry.to });
  }
}

bool LineRangeFilter::contains(const std::string &filePath, size_t line) {
  if (line == 0) {
    return false;
  }

  std::lock_guard<std::mutex> lock(cacheMutex);
  auto cached = cache.find(filePath);
  if (cached == cache.end()) {
    std::vector<std::pair<unsigned, unsigned>> matching;
    for (Range &range : ranges) {
      if (range.regex.match(filePath)) {
        matching.emplace_back(range.from, range.to);
      }
    }
    cached = cache.emplace(filePath, std::move(matching)).first;
  }

  for (const auto &range : cached->second) {
    if (line >= range.first && line <= range.second) {
      return true;
    }
  }
  return false;
}

bool LineRangeFilter::shouldSkip(MutationPoint *point) {
  const SourceLocation &location = point->getSourceLocation();
  return !contains(location.filePath, location.line);
}

std::string LineRangeFilter::name() {
  std::stringstream message;
  message << "line range";
  if (configuredCount != ranges.size()) {
    message << " (" << ranges.size() << " of " << configuredCount << " ranges usable)";
  }
  return message.str();
}
