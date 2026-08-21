#include "BitcodeLoader.h"
#include "FixturePaths.h"
#include "mull/Filters/LineRangeFilter.h"
#include "mull/Filters/SliceFilter.h"
#include "mull/MutationPoint.h"
#include <mull/Mutators/CXX/ArithmeticMutators.h>

#include <algorithm>
#include <gtest/gtest.h>
#include <limits>
#include <mull/Diagnostics/Diagnostics.h>
#include <set>
#include <string>
#include <vector>

using namespace mull;

namespace {

LineRangeConfig range(const std::string &file, unsigned from, unsigned to) {
  LineRangeConfig config;
  config.file = file;
  config.from = from;
  config.to = to;
  return config;
}

/// `to` left at its default, which is what an omitted `to:` produces.
LineRangeConfig openEndedRange(const std::string &file, unsigned from) {
  LineRangeConfig config;
  config.file = file;
  config.from = from;
  return config;
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

/// Both ends are inclusive. The whole point of the filter is to express "this
/// half of the file", so an end that is off by one silently moves the boundary.
TEST(LineRangeFilter, boundsAreInclusiveAtBothEnds) {
  LineRangeFilter filter({ range(".*", 10, 20) });
  ASSERT_FALSE(filter.contains("/x/a.cpp", 9));
  ASSERT_TRUE(filter.contains("/x/a.cpp", 10));
  ASSERT_TRUE(filter.contains("/x/a.cpp", 15));
  ASSERT_TRUE(filter.contains("/x/a.cpp", 20));
  ASSERT_FALSE(filter.contains("/x/a.cpp", 21));
}

/// A one-line range is a legitimate range, not a degenerate one.
TEST(LineRangeFilter, aSingleLineRangeKeepsExactlyThatLine) {
  LineRangeFilter filter({ range(".*", 42, 42) });
  ASSERT_FALSE(filter.contains("/x/a.cpp", 41));
  ASSERT_TRUE(filter.contains("/x/a.cpp", 42));
  ASSERT_FALSE(filter.contains("/x/a.cpp", 43));
}

/// Ranges are a union: a point inside any of them is kept, and they may
/// overlap without double-counting or cancelling out.
TEST(LineRangeFilter, multipleRangesAreAUnion) {
  LineRangeFilter filter({ range(".*", 10, 20), range(".*", 100, 110), range(".*", 15, 30) });
  ASSERT_TRUE(filter.contains("/x/a.cpp", 10));
  ASSERT_TRUE(filter.contains("/x/a.cpp", 25));
  ASSERT_TRUE(filter.contains("/x/a.cpp", 30));
  ASSERT_FALSE(filter.contains("/x/a.cpp", 31));
  ASSERT_FALSE(filter.contains("/x/a.cpp", 99));
  ASSERT_TRUE(filter.contains("/x/a.cpp", 105));
  ASSERT_FALSE(filter.contains("/x/a.cpp", 111));
}

/// The range only applies to the files its regex names. A file that matches no
/// range keeps nothing, which is what makes the filter safe to point at one
/// file in a build of many.
TEST(LineRangeFilter, rangesApplyOnlyToMatchingFiles) {
  LineRangeFilter filter({ range(".*halide_blur.*", 3467, 4204) });
  ASSERT_TRUE(filter.contains("/x/halide_blur.halide_generated.cpp", 3500));
  ASSERT_FALSE(filter.contains("/x/harris.halide_generated.cpp", 3500));
  ASSERT_FALSE(filter.contains("/x/unrelated.cpp", 3500));
}

/// Per-file ranges: two files with different boundaries must not borrow each
/// other's, which is the case that matters because the boundary really does
/// differ per generated file.
TEST(LineRangeFilter, eachFileGetsItsOwnRange) {
  LineRangeFilter filter({ range(".*blur.*", 3467, 4000), range(".*harris.*", 3501, 4200) });
  ASSERT_TRUE(filter.contains("/x/blur.cpp", 3467));
  ASSERT_FALSE(filter.contains("/x/blur.cpp", 3466));
  ASSERT_FALSE(filter.contains("/x/harris.cpp", 3467));
  ASSERT_TRUE(filter.contains("/x/harris.cpp", 3501));
  ASSERT_TRUE(filter.contains("/x/blur.cpp", 4000));
  ASSERT_FALSE(filter.contains("/x/blur.cpp", 4001));
  ASSERT_TRUE(filter.contains("/x/harris.cpp", 4200));
}

/// An omitted `to:` means "to the end of the file", not "to line 0".
TEST(LineRangeFilter, omittedEndRunsToTheEndOfTheFile) {
  ASSERT_EQ(LineRangeConfig().to, std::numeric_limits<unsigned>::max());

  LineRangeFilter filter({ openEndedRange(".*", 3467) });
  ASSERT_FALSE(filter.contains("/x/a.cpp", 3466));
  ASSERT_TRUE(filter.contains("/x/a.cpp", 3467));
  ASSERT_TRUE(filter.contains("/x/a.cpp", 1000000));
}

/// Degenerate input must not be undefined behaviour and must not silently keep
/// something. Filters::enableLineRangeFilter reports these as errors; the
/// filter itself simply matches nothing.
TEST(LineRangeFilter, degenerateRangesKeepNothing) {
  LineRangeFilter inverted({ range(".*", 500, 100) });
  ASSERT_FALSE(inverted.contains("/x/a.cpp", 100));
  ASSERT_FALSE(inverted.contains("/x/a.cpp", 300));
  ASSERT_FALSE(inverted.contains("/x/a.cpp", 500));

  /// `from: 0` is not a line -- line numbers are 1-based -- and must not become
  /// an accidental "from the beginning".
  LineRangeFilter fromZero({ range(".*", 0, 0) });
  ASSERT_FALSE(fromZero.contains("/x/a.cpp", 0));
  ASSERT_FALSE(fromZero.contains("/x/a.cpp", 1));

  LineRangeFilter empty({});
  ASSERT_FALSE(empty.contains("/x/a.cpp", 1));
}

/// A point with no line information cannot be shown to be inside any range, so
/// it is not kept. Mull records line 0 when debug information is missing.
TEST(LineRangeFilter, pointsWithoutLineInformationAreSkipped) {
  LineRangeFilter filter({ range(".*", 1, std::numeric_limits<unsigned>::max()) });
  ASSERT_FALSE(filter.contains("/x/a.cpp", 0));
}

/// An unusable regex is dropped rather than silently matching nothing, and the
/// filter says so in its name so it shows up in the run log.
TEST(LineRangeFilter, invalidRegexesAreDroppedAndReportedInTheName) {
  LineRangeFilter filter({ range("[", 1, 10), range(".*", 1, 10) });
  ASSERT_TRUE(filter.contains("/x/a.cpp", 5));
  ASSERT_EQ(filter.name(), "line range (1 of 2 ranges usable)");
}

TEST(LineRangeFilter, nameIsPlainWhenEveryRangeIsUsable) {
  LineRangeFilter filter({ range(".*", 1, 10) });
  ASSERT_EQ(filter.name(), "line range");
}

/// shouldSkip is the inverse of contains, over real mutation points.
TEST(LineRangeFilter, shouldSkipFollowsThePointSourceLine) {
  Diagnostics diagnostics;
  BitcodeLoader loader;
  auto bitcode =
      loader.loadBitcodeAtPath(fixtures::mutators_math_add_module_bc_path(), diagnostics);
  std::vector<MutationPoint *> points = collectPoints(bitcode.get());
  ASSERT_NE(points.size(), size_t(0));

  size_t minLine = points.front()->getSourceLocation().line;
  size_t maxLine = minLine;
  for (auto *point : points) {
    minLine = std::min(minLine, point->getSourceLocation().line);
    maxLine = std::max(maxLine, point->getSourceLocation().line);
  }
  ASSERT_GT(maxLine, minLine) << "fixture needs points on more than one line";

  LineRangeFilter everything({ range(".*", 1, std::numeric_limits<unsigned>::max()) });
  LineRangeFilter nothing({ range(".*", maxLine + 1, maxLine + 100) });
  const unsigned split = static_cast<unsigned>((minLine + maxLine) / 2);
  LineRangeFilter lowerHalf({ range(".*", static_cast<unsigned>(minLine), split) });

  size_t keptByLowerHalf = 0;
  for (auto *point : points) {
    ASSERT_FALSE(everything.shouldSkip(point));
    ASSERT_TRUE(nothing.shouldSkip(point));
    if (!lowerHalf.shouldSkip(point)) {
      keptByLowerHalf++;
      /// Checked directly rather than inferred from the count.
      ASSERT_GE(point->getSourceLocation().line, minLine);
      ASSERT_LE(point->getSourceLocation().line, split);
    }
  }
  ASSERT_GT(keptByLowerHalf, size_t(0));
  ASSERT_LT(keptByLowerHalf, points.size());
}

/// Composition with slicing: the two are independent predicates, so running
/// every slice of a line-filtered population reproduces the line-filtered
/// population exactly -- nothing lost between them, nothing counted twice.
TEST(LineRangeFilter, composesWithSlicingWithoutLosingOrDuplicatingPoints) {
  Diagnostics diagnostics;
  BitcodeLoader loader;
  auto bitcode =
      loader.loadBitcodeAtPath(fixtures::mutators_math_add_module_bc_path(), diagnostics);
  std::vector<MutationPoint *> points = collectPoints(bitcode.get());
  ASSERT_NE(points.size(), size_t(0));

  size_t minLine = points.front()->getSourceLocation().line;
  size_t maxLine = minLine;
  for (auto *point : points) {
    minLine = std::min(minLine, point->getSourceLocation().line);
    maxLine = std::max(maxLine, point->getSourceLocation().line);
  }
  const unsigned split = static_cast<unsigned>((minLine + maxLine) / 2);

  LineRangeFilter lineFilter({ range(".*", static_cast<unsigned>(minLine), split) });
  std::vector<std::string> lineOnly;
  for (auto *point : points) {
    if (!lineFilter.shouldSkip(point)) {
      lineOnly.push_back(point->getUserIdentifier());
    }
  }
  ASSERT_GT(lineOnly.size(), size_t(0));

  const unsigned count = 4;
  std::vector<std::string> unionOfSlices;
  for (unsigned index = 0; index < count; index++) {
    LineRangeFilter perSliceLineFilter({ range(".*", static_cast<unsigned>(minLine), split) });
    SliceFilter sliceFilter(index, count);
    for (auto *point : points) {
      if (!perSliceLineFilter.shouldSkip(point) && !sliceFilter.shouldSkip(point)) {
        unionOfSlices.push_back(point->getUserIdentifier());
      }
    }
  }

  std::sort(lineOnly.begin(), lineOnly.end());
  std::sort(unionOfSlices.begin(), unionOfSlices.end());
  ASSERT_EQ(unionOfSlices, lineOnly);
}
