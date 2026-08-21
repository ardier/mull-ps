#include "BitcodeLoader.h"
#include "FixturePaths.h"
#include "mull/Filters/SliceFilter.h"
#include "mull/MutationPoint.h"
#include "mull/MutationsFinder.h"
#include "mull/Program/Program.h"
#include <mull/Mutators/CXX/ArithmeticMutators.h>

#include <algorithm>
#include <gtest/gtest.h>
#include <llvm/IR/LLVMContext.h>
#include <mull/Diagnostics/Diagnostics.h>
#include <string>
#include <vector>

using namespace mull;

/// Pinned golden values for the hash itself. If a refactor ever changes these,
/// every previously recorded slice assignment silently becomes a different set
/// of mutants, so this test is the tripwire for that.
TEST(SliceFilter, fnv1a64GoldenValues) {
  ASSERT_EQ(SliceFilter::fnv1a64(""), 0xcbf29ce484222325ULL);
  ASSERT_EQ(SliceFilter::fnv1a64("a"), 0xaf63dc4c8601ec8cULL);
  ASSERT_EQ(SliceFilter::fnv1a64("mull"), 0xb6ba56a29324e397ULL);
  ASSERT_EQ(SliceFilter::fnv1a64("cxx_add_to_sub:/tmp/x.cpp:10:3"), 0x913e1d12c50faaabULL);
}

/// count <= 1 must never divide by zero and must never skip anything.
TEST(SliceFilter, degenerateCountsAreSafe) {
  const std::string identity = "cxx_add_to_sub:/tmp/x.cpp:10:3";
  ASSERT_EQ(SliceFilter::sliceOf(identity, 0), 0u);
  ASSERT_EQ(SliceFilter::sliceOf(identity, 1), 0u);
  ASSERT_EQ(SliceFilter::sliceOf("", 0), 0u);
}

/// Deterministic input strings and a deterministic hash, so this cannot flake:
/// every slice is non-empty and the slices exactly cover the input.
TEST(SliceFilter, sliceOfCoversEveryBucket) {
  const unsigned count = 8;
  std::vector<unsigned> histogram(count, 0);
  for (unsigned line = 0; line < 200; line++) {
    std::string identity = "cxx_add_to_sub:/tmp/x.cpp:" + std::to_string(line) + ":3";
    unsigned slice = SliceFilter::sliceOf(identity, count);
    ASSERT_LT(slice, count);
    histogram[slice]++;
  }
  unsigned total = 0;
  for (unsigned i = 0; i < count; i++) {
    ASSERT_GT(histogram[i], 0u) << "slice " << i << " got no identities";
    total += histogram[i];
  }
  ASSERT_EQ(total, 200u);
}

static std::vector<MutationPoint *> collectPoints(Bitcode *bitcode) {
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

/// The property the whole design rests on: running every slice and unioning the
/// survivors reproduces the unsliced set exactly -- nothing dropped, nothing
/// counted twice.
TEST(SliceFilter, slicesPartitionTheMutationPoints) {
  Diagnostics diagnostics;
  BitcodeLoader loader;
  auto path = fixtures::mutation_filters_no_debug_filter_with_debug_bc_path();
  auto bitcode = loader.loadBitcodeAtPath(path, diagnostics);

  std::vector<MutationPoint *> points = collectPoints(bitcode.get());
  ASSERT_NE(points.size(), size_t(0));

  std::vector<std::string> everything;
  for (auto *point : points) {
    everything.push_back(point->getUserIdentifier());
  }

  const unsigned count = 4;
  std::vector<std::string> unionOfSlices;
  for (unsigned index = 0; index < count; index++) {
    SliceFilter filter(index, count);
    for (auto *point : points) {
      if (!filter.shouldSkip(point)) {
        unionOfSlices.push_back(point->getUserIdentifier());
      }
    }
  }

  std::sort(everything.begin(), everything.end());
  std::sort(unionOfSlices.begin(), unionOfSlices.end());
  ASSERT_EQ(unionOfSlices, everything);
}

/// A single slice keeps everything, matching the unfiltered run.
TEST(SliceFilter, singleSliceKeepsEverything) {
  Diagnostics diagnostics;
  BitcodeLoader loader;
  auto path = fixtures::mutation_filters_no_debug_filter_with_debug_bc_path();
  auto bitcode = loader.loadBitcodeAtPath(path, diagnostics);

  std::vector<MutationPoint *> points = collectPoints(bitcode.get());
  ASSERT_NE(points.size(), size_t(0));

  SliceFilter filter(0, 1);
  size_t kept = 0;
  for (auto *point : points) {
    if (!filter.shouldSkip(point)) {
      kept++;
    }
  }
  ASSERT_EQ(kept, points.size());
}

/// Membership follows the identity string, not the order the points were
/// discovered in: shuffling the vector cannot move a point between slices.
TEST(SliceFilter, membershipFollowsIdentityNotOrder) {
  Diagnostics diagnostics;
  BitcodeLoader loader;
  auto path = fixtures::mutation_filters_no_debug_filter_with_debug_bc_path();
  auto bitcode = loader.loadBitcodeAtPath(path, diagnostics);

  std::vector<MutationPoint *> points = collectPoints(bitcode.get());
  ASSERT_NE(points.size(), size_t(0));

  const unsigned count = 3;
  SliceFilter filter(1, count);
  for (auto *point : points) {
    bool skipped = filter.shouldSkip(point);
    unsigned expected = SliceFilter::sliceOf(point->getUserIdentifier(), count);
    ASSERT_EQ(skipped, expected != 1u);
  }
}

TEST(SliceFilter, nameReportsTheSlice) {
  SliceFilter filter(3, 10);
  ASSERT_EQ(filter.name(), "slice 3 of 10");
}
