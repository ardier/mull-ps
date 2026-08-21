#include "mull/MutantDump.h"

#include "mull/Diagnostics/Diagnostics.h"
#include "mull/MutationPoint.h"

#include <llvm/IR/Function.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>

#include <algorithm>
#include <sstream>
#include <unordered_set>
#include <utility>

using namespace mull;
using namespace std::string_literals;

MutantDump::MutantDump(Diagnostics &diagnostics, std::string pathPrefix,
                       const RegionsConfig &regions)
    : diagnostics(diagnostics), pathPrefix(std::move(pathPrefix)), regions(diagnostics, regions) {}

std::vector<std::string> MutantDump::sortedIdentifiers(const std::vector<MutationPoint *> &points) {
  std::vector<std::string> identifiers;
  identifiers.reserve(points.size());
  for (auto *point : points) {
    identifiers.push_back(point->getUserIdentifier());
  }
  /// Byte order, not locale order: the dump is compared against files produced
  /// by other tools (strings | sort), and a locale-dependent collation would
  /// make that comparison depend on the environment.
  std::sort(identifiers.begin(), identifiers.end());
  identifiers.erase(std::unique(identifiers.begin(), identifiers.end()), identifiers.end());
  return identifiers;
}

MutantDump::Record &MutantDump::recordFor(MutationPoint *point) {
  Record &record = records[point->getUserIdentifier()];
  if (record.line == 0) {
    record.filePath = point->getSourceLocation().filePath;
    record.line = point->getSourceLocation().line;
  }
  llvm::Function *function = point->getOriginalFunction();
  if (function != nullptr) {
    record.functions.insert(function->getName().str());
  }
  return record;
}

void MutantDump::recordPopulation(const std::vector<MutationPoint *> &points) {
  for (auto *point : points) {
    recordFor(point).points++;
  }
}

void MutantDump::recordFilterStage(const std::vector<MutationPoint *> &before,
                                   const std::vector<MutationPoint *> &after,
                                   const std::string &filterName) {
  /// Pointer identity, not identifier equality: the filter tasks push the very
  /// same MutationPoint objects through, and two distinct points can share an
  /// identifier when one source location yields several instructions.
  std::unordered_set<const MutationPoint *> survivors(after.begin(), after.end());
  for (auto *point : before) {
    if (survivors.count(point) != 0) {
      continue;
    }
    Record &record = recordFor(point);
    if (record.filteredBy.empty()) {
      record.filteredBy = filterName;
    } else if (record.filteredBy.find(filterName) == std::string::npos) {
      record.filteredBy += ',' + filterName;
    }
  }
}

std::string MutantDump::rejectedBy(const std::string &identifier) const {
  auto it = records.find(identifier);
  if (it == records.end()) {
    return {};
  }
  return it->second.filteredBy;
}

bool MutantDump::writeLines(Diagnostics &diagnostics, const std::string &path,
                            const std::vector<std::string> &lines) {
  std::error_code ec;
  llvm::raw_fd_ostream out(path, ec, llvm::sys::fs::OF_Text);
  if (ec) {
    diagnostics.warning("Cannot write mutant dump to "s + path + ": " + ec.message());
    return false;
  }
  for (const std::string &line : lines) {
    out << line << '\n';
  }
  /// Flushed and checked before returning: a truncated dump would be worse than
  /// no dump, because it looks like a complete population.
  out.flush();
  if (out.has_error()) {
    diagnostics.warning("Error while writing mutant dump to "s + path + ": " +
                        out.error().message());
    out.clear_error();
    return false;
  }
  return true;
}

static std::string joined(const std::set<std::string> &values) {
  std::string result;
  for (const std::string &value : values) {
    if (!result.empty()) {
      result += ',';
    }
    result += value;
  }
  return result;
}

bool MutantDump::write(const std::vector<MutationPoint *> &kept) {
  for (auto *point : kept) {
    recordFor(point).keptPoints++;
  }

  /// `records` is a std::map, so iteration is already in byte order and one
  /// entry per identifier, which is the shape all four files want.
  std::vector<std::string> keptIdentifiers;
  std::vector<std::string> filteredIdentifiers;
  std::vector<std::string> attributions;
  std::vector<std::string> rows;
  rows.push_back(
      "identifier\tregion\tpoints\tkept_points\tfiltered_points\tfunctions\tfiltered_by");

  size_t overlap = 0;
  size_t keptPoints = 0;
  size_t totalPoints = 0;
  std::map<std::string, size_t> keptByRegion;
  std::map<std::string, size_t> filteredByRegion;

  for (auto &pair : records) {
    const std::string &identifier = pair.first;
    Record &record = pair.second;
    /// A point can only be counted once, but a record that was never seen by
    /// recordPopulation (a caller that skipped it) would report 0 points; take
    /// the kept count as the floor so the row can never be self-contradictory.
    if (record.points < record.keptPoints) {
      record.points = record.keptPoints;
    }
    const size_t filteredPoints = record.points - record.keptPoints;
    const char *region = regionName(regions.classify(record.filePath, record.line));

    totalPoints += record.points;
    keptPoints += record.keptPoints;

    if (record.keptPoints > 0) {
      keptIdentifiers.push_back(identifier);
      keptByRegion[region]++;
    }
    if (filteredPoints > 0) {
      filteredIdentifiers.push_back(identifier);
      attributions.push_back(identifier + '\t' + record.filteredBy);
      filteredByRegion[region]++;
    }
    if (record.keptPoints > 0 && filteredPoints > 0) {
      overlap++;
    }

    std::stringstream row;
    row << identifier << '\t' << region << '\t' << record.points << '\t' << record.keptPoints
        << '\t' << filteredPoints << '\t' << joined(record.functions) << '\t' << record.filteredBy;
    rows.push_back(row.str());
  }

  bool ok = writeLines(diagnostics, pathPrefix + ".kept.txt", keptIdentifiers);
  ok = writeLines(diagnostics, pathPrefix + ".filtered.txt", filteredIdentifiers) && ok;
  ok = writeLines(diagnostics, pathPrefix + ".filtered-by.txt", attributions) && ok;
  ok = writeLines(diagnostics, pathPrefix + ".mutants.tsv", rows) && ok;

  std::stringstream message;
  message << "Mutant population dumped to " << pathPrefix
          << ".{kept,filtered,filtered-by}.txt and .mutants.tsv: " << keptIdentifiers.size()
          << " kept, " << filteredIdentifiers.size() << " filtered (" << records.size()
          << " distinct identifiers over " << totalPoints << " points, " << keptPoints
          << " of them kept)";
  diagnostics.info(message.str());

  for (const auto &pair : keptByRegion) {
    std::stringstream regionMessage;
    regionMessage << "  region " << pair.first << ": " << pair.second << " kept, "
                  << filteredByRegion[pair.first] << " filtered";
    diagnostics.info(regionMessage.str());
  }
  for (const auto &pair : filteredByRegion) {
    if (keptByRegion.count(pair.first) == 0) {
      std::stringstream regionMessage;
      regionMessage << "  region " << pair.first << ": 0 kept, " << pair.second << " filtered";
      diagnostics.info(regionMessage.str());
    }
  }

  /// One identifier can belong to two distinct mutation points that were not
  /// filtered alike, in which case the two sets genuinely do overlap. That is
  /// worth saying out loud rather than hiding, because anything downstream that
  /// treats the files as a partition would be wrong about those entries.
  if (overlap != 0) {
    std::stringstream warning;
    warning << "Mutant dump: " << overlap
            << " identifier(s) appear in both the kept and the filtered set. This happens when "
               "one source location yields several mutation points that the filters treated "
               "differently; the two files are then not a partition of the identifier space. See "
               "the kept_points and filtered_points columns of the .mutants.tsv for which.";
    diagnostics.warning(warning.str());
  }

  return ok;
}
