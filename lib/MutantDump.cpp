#include "mull/MutantDump.h"

#include "mull/Diagnostics/Diagnostics.h"
#include "mull/MutationPoint.h"

#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>

#include <algorithm>
#include <sstream>
#include <unordered_set>
#include <utility>

using namespace mull;
using namespace std::string_literals;

MutantDump::MutantDump(Diagnostics &diagnostics, std::string pathPrefix)
    : diagnostics(diagnostics), pathPrefix(std::move(pathPrefix)) {}

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
    const std::string identifier = point->getUserIdentifier();
    auto it = rejectedBy.find(identifier);
    if (it == rejectedBy.end()) {
      rejectedBy.emplace(identifier, filterName);
      continue;
    }
    if (it->second.find(filterName) == std::string::npos) {
      it->second += ',' + filterName;
    }
  }
}

const std::map<std::string, std::string> &MutantDump::rejections() const {
  return rejectedBy;
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
  out.flush();
  if (out.has_error()) {
    diagnostics.warning("Error while writing mutant dump to "s + path + ": " +
                        out.error().message());
    out.clear_error();
    return false;
  }
  return true;
}

bool MutantDump::write(const std::vector<MutationPoint *> &kept) {
  std::vector<std::string> keptIdentifiers = sortedIdentifiers(kept);

  /// rejectedBy is a std::map, so its keys already come out in byte order and
  /// deduplicated, which is the same shape as the kept file.
  std::vector<std::string> filteredIdentifiers;
  std::vector<std::string> attributions;
  filteredIdentifiers.reserve(rejectedBy.size());
  attributions.reserve(rejectedBy.size());
  for (const auto &pair : rejectedBy) {
    filteredIdentifiers.push_back(pair.first);
    attributions.push_back(pair.first + '\t' + pair.second);
  }

  /// One identifier can belong to two distinct mutation points that were not
  /// filtered alike, in which case the two sets genuinely do overlap. That is
  /// worth saying out loud rather than hiding, because anything downstream
  /// that treats the files as a partition would be wrong about those entries.
  size_t overlap = 0;
  for (const std::string &identifier : keptIdentifiers) {
    if (rejectedBy.count(identifier) != 0) {
      overlap++;
    }
  }

  bool ok = writeLines(diagnostics, pathPrefix + ".kept.txt", keptIdentifiers);
  ok = writeLines(diagnostics, pathPrefix + ".filtered.txt", filteredIdentifiers) && ok;
  ok = writeLines(diagnostics, pathPrefix + ".filtered-by.txt", attributions) && ok;

  std::stringstream message;
  message << "Mutant population dumped to " << pathPrefix
          << ".{kept,filtered,filtered-by}.txt: " << keptIdentifiers.size() << " kept, "
          << filteredIdentifiers.size() << " filtered";
  diagnostics.info(message.str());

  if (overlap != 0) {
    std::stringstream warning;
    warning << "Mutant dump: " << overlap
            << " identifier(s) appear in both the kept and the filtered set. This happens when "
               "one source location yields several mutation points that the filters treated "
               "differently; the two files are then not a partition of the identifier space.";
    diagnostics.warning(warning.str());
  }

  return ok;
}
