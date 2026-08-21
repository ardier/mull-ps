#include "BitcodeLoader.h"
#include "FixturePaths.h"
#include "mull/Filters/SliceFilter.h"
#include "mull/MutantDump.h"
#include "mull/MutationPoint.h"
#include "mull/RegionClassifier.h"
#include <mull/Mutators/CXX/ArithmeticMutators.h>

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <gtest/gtest.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>
#include <mull/Diagnostics/Diagnostics.h>
#include <set>
#include <string>
#include <unistd.h>
#include <vector>

using namespace mull;

namespace {

/// A dump path prefix unique to the running process, so parallel test runs
/// cannot collide on the same files.
std::string uniquePrefix(const char *tag) {
  llvm::SmallString<256> path;
  llvm::sys::path::system_temp_directory(true, path);
  return std::string(path.str()) + "/mull-mutant-dump-" + tag + "-" + std::to_string(getpid());
}

std::vector<std::string> readLines(const std::string &path) {
  std::vector<std::string> lines;
  std::ifstream in(path);
  std::string line;
  while (std::getline(in, line)) {
    lines.push_back(line);
  }
  return lines;
}

void removeDump(const std::string &prefix) {
  for (const char *suffix : { ".kept.txt", ".filtered.txt", ".filtered-by.txt", ".mutants.tsv" }) {
    std::remove((prefix + suffix).c_str());
  }
}

std::vector<MutationPoint *> collectPoints(Bitcode *bitcode) {
  std::vector<MutationPoint *> points;
  cxx::AddToSub mutator;
  for (auto &function : bitcode->getModule()->functions()) {
    FunctionUnderTest functionUnderTest(&function, bitcode);
    functionUnderTest.selectInstructions({});
    auto mutants = mutator.getMutations(bitcode, functionUnderTest);
    std::copy(mutants.begin(), mutants.end(), std::back_inserter(points));
  }
  return points;
}

} // namespace

/// The file format is the contract with everything downstream: one identifier
/// per line, newline-terminated, no header, no trailing blank line beyond the
/// final terminator.
TEST(MutantDump, writeLinesWritesOneLinePerElement) {
  Diagnostics diagnostics;
  diagnostics.makeQuiet();
  diagnostics.makeSilent();
  const std::string path = uniquePrefix("writelines") + ".kept.txt";
  ASSERT_TRUE(MutantDump::writeLines(diagnostics, path, { "a", "b", "c" }));

  std::ifstream in(path, std::ios::binary);
  std::string contents((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  ASSERT_EQ(contents, "a\nb\nc\n");
  std::remove(path.c_str());
}

TEST(MutantDump, writeLinesOfAnEmptySetProducesAnEmptyFile) {
  Diagnostics diagnostics;
  diagnostics.makeQuiet();
  diagnostics.makeSilent();
  const std::string path = uniquePrefix("empty") + ".kept.txt";
  ASSERT_TRUE(MutantDump::writeLines(diagnostics, path, {}));
  std::ifstream in(path, std::ios::binary);
  std::string contents((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  ASSERT_EQ(contents, "");
  std::remove(path.c_str());
}

/// An unwritable destination must be reported, not silently swallowed, and
/// must not take the compilation down with it.
TEST(MutantDump, writeLinesReportsAnUnwritablePath) {
  Diagnostics diagnostics;
  diagnostics.makeQuiet();
  diagnostics.makeSilent();
  ASSERT_FALSE(MutantDump::writeLines(
      diagnostics, "/nonexistent-directory-for-mull-tests/x.kept.txt", { "a" }));
}

/// Output is a set in byte order, so a dump does not depend on the order the
/// parallel workers happened to produce the points in.
TEST(MutantDump, sortedIdentifiersDeduplicatesAndSortsInByteOrder) {
  Diagnostics diagnostics;
  diagnostics.makeQuiet();
  diagnostics.makeSilent();
  BitcodeLoader loader;
  auto bitcode =
      loader.loadBitcodeAtPath(fixtures::mutators_math_add_module_bc_path(), diagnostics);
  std::vector<MutationPoint *> points = collectPoints(bitcode.get());
  ASSERT_NE(points.size(), size_t(0));

  std::vector<std::string> forward = MutantDump::sortedIdentifiers(points);
  ASSERT_TRUE(std::is_sorted(forward.begin(), forward.end()));
  ASSERT_EQ(std::set<std::string>(forward.begin(), forward.end()).size(), forward.size());

  std::vector<MutationPoint *> reversed(points.rbegin(), points.rend());
  ASSERT_EQ(MutantDump::sortedIdentifiers(reversed), forward);
}

/// The rejection attribution: `before` minus `after` is what this filter took
/// out, and the name recorded is the one that did it.
TEST(MutantDump, recordFilterStageAttributesRejectionsToTheFilter) {
  Diagnostics diagnostics;
  diagnostics.makeQuiet();
  diagnostics.makeSilent();
  BitcodeLoader loader;
  auto bitcode =
      loader.loadBitcodeAtPath(fixtures::mutators_math_add_module_bc_path(), diagnostics);
  std::vector<MutationPoint *> points = collectPoints(bitcode.get());
  ASSERT_GT(points.size(), size_t(1));

  std::vector<MutationPoint *> survivors(points.begin() + 1, points.end());

  RegionsConfig noRegions;
  MutantDump dump(diagnostics, uniquePrefix("attribution"), noRegions);
  dump.recordFilterStage(points, survivors, "first filter");

  /// The dropped point's identifier may be shared with a survivor, in which
  /// case it is legitimately present in both; the invariant that always holds
  /// is that the rejected identifier was recorded against the right filter.
  const std::string dropped = points.front()->getUserIdentifier();
  ASSERT_EQ(dump.rejectedBy(dropped), "first filter");
}

TEST(MutantDump, recordFilterStageRecordsNothingWhenAFilterRejectsNothing) {
  Diagnostics diagnostics;
  diagnostics.makeQuiet();
  diagnostics.makeSilent();
  BitcodeLoader loader;
  auto bitcode =
      loader.loadBitcodeAtPath(fixtures::mutators_math_add_module_bc_path(), diagnostics);
  std::vector<MutationPoint *> points = collectPoints(bitcode.get());
  ASSERT_NE(points.size(), size_t(0));

  RegionsConfig noRegions;
  MutantDump dump(diagnostics, uniquePrefix("norejections"), noRegions);
  dump.recordPopulation(points);
  dump.recordFilterStage(points, points, "pass-through filter");
  for (auto *point : points) {
    ASSERT_EQ(dump.rejectedBy(point->getUserIdentifier()), "");
  }
}

/// The chain property the Driver relies on: staging the filters one after the
/// other attributes every point exactly once, and kept plus filtered
/// telescopes back to the population that went in.
TEST(MutantDump, chainedStagesPartitionThePopulation) {
  Diagnostics diagnostics;
  diagnostics.makeQuiet();
  diagnostics.makeSilent();
  BitcodeLoader loader;
  auto bitcode =
      loader.loadBitcodeAtPath(fixtures::mutators_math_add_module_bc_path(), diagnostics);
  std::vector<MutationPoint *> points = collectPoints(bitcode.get());
  ASSERT_GT(points.size(), size_t(2));

  const std::string prefix = uniquePrefix("chain");
  RegionsConfig noRegions;
  MutantDump dump(diagnostics, prefix, noRegions);
  dump.recordPopulation(points);

  /// Two real filters in a chain, each taking a different, deterministic share.
  SliceFilter first(0, 2);
  SliceFilter second(0, 3);

  std::vector<MutationPoint *> afterFirst;
  for (auto *point : points) {
    if (!first.shouldSkip(point)) {
      afterFirst.push_back(point);
    }
  }
  dump.recordFilterStage(points, afterFirst, first.name());

  std::vector<MutationPoint *> afterSecond;
  for (auto *point : afterFirst) {
    if (!second.shouldSkip(point)) {
      afterSecond.push_back(point);
    }
  }
  dump.recordFilterStage(afterFirst, afterSecond, second.name());

  ASSERT_TRUE(dump.write(afterSecond));

  std::vector<std::string> kept = readLines(prefix + ".kept.txt");
  std::vector<std::string> filtered = readLines(prefix + ".filtered.txt");
  std::vector<std::string> attributions = readLines(prefix + ".filtered-by.txt");

  ASSERT_EQ(kept, MutantDump::sortedIdentifiers(afterSecond));
  ASSERT_EQ(filtered.size(), attributions.size());
  ASSERT_TRUE(std::is_sorted(filtered.begin(), filtered.end()));

  /// Union is the whole pre-filter population, and (for this fixture, whose
  /// points all carry distinct identifiers) the two sets are disjoint.
  std::set<std::string> keptSet(kept.begin(), kept.end());
  std::set<std::string> filteredSet(filtered.begin(), filtered.end());
  std::set<std::string> unionSet = keptSet;
  unionSet.insert(filteredSet.begin(), filteredSet.end());

  std::vector<std::string> all = MutantDump::sortedIdentifiers(points);
  ASSERT_EQ(all.size(), points.size()) << "fixture carries duplicate identifiers, so the two "
                                          "sets are not expected to be disjoint here";
  ASSERT_EQ(std::vector<std::string>(unionSet.begin(), unionSet.end()), all);
  ASSERT_EQ(keptSet.size() + filteredSet.size(), all.size());

  /// Every attribution line names one of the two filters that actually ran.
  for (const std::string &line : attributions) {
    auto tab = line.find('\t');
    ASSERT_NE(tab, std::string::npos);
    const std::string who = line.substr(tab + 1);
    ASSERT_TRUE(who == first.name() || who == second.name()) << "unexpected filter name: " << who;
    ASSERT_EQ(filteredSet.count(line.substr(0, tab)), size_t(1));
  }

  /// The table carries the same identifiers, in the same order, plus a header.
  std::vector<std::string> rows = readLines(prefix + ".mutants.tsv");
  ASSERT_EQ(rows.front(),
            "identifier\tregion\tpoints\tkept_points\tfiltered_points\tfunctions\tfiltered_by");
  ASSERT_EQ(rows.size(), all.size() + 1);
  for (size_t i = 1; i < rows.size(); i++) {
    ASSERT_EQ(rows[i].substr(0, rows[i].find('\t')), all[i - 1]);
    /// Region tagging is off, so every row must say so rather than guess.
    ASSERT_NE(rows[i].find("\tunknown\t"), std::string::npos) << rows[i];
  }

  removeDump(prefix);
}

/// A population that survives everything still produces both files, the
/// filtered one simply empty. Downstream tooling can then rely on the files
/// existing whenever the key is set.
TEST(MutantDump, writeProducesBothFilesWhenNothingWasFiltered) {
  Diagnostics diagnostics;
  diagnostics.makeQuiet();
  diagnostics.makeSilent();
  BitcodeLoader loader;
  auto bitcode =
      loader.loadBitcodeAtPath(fixtures::mutators_math_add_module_bc_path(), diagnostics);
  std::vector<MutationPoint *> points = collectPoints(bitcode.get());
  ASSERT_NE(points.size(), size_t(0));

  const std::string prefix = uniquePrefix("nofiltering");
  RegionsConfig noRegions;
  MutantDump dump(diagnostics, prefix, noRegions);
  dump.recordPopulation(points);
  ASSERT_TRUE(dump.write(points));

  ASSERT_EQ(readLines(prefix + ".kept.txt"), MutantDump::sortedIdentifiers(points));
  ASSERT_TRUE(readLines(prefix + ".filtered.txt").empty());
  ASSERT_TRUE(readLines(prefix + ".filtered-by.txt").empty());

  removeDump(prefix);
}
