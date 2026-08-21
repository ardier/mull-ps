#pragma once

#include "mull/Filters/MutationPointFilter.h"

#include <cstdint>
#include <string>

namespace mull {

class MutationPoint;

/// Deterministically partitions the discovered mutation points into `count`
/// disjoint slices and keeps only the points belonging to slice `index`.
///
/// Membership is decided by an FNV-1a 64-bit hash of the point's *user
/// identifier* -- `<mutator>:<absolute file path>:<line>:<column>` -- which is
/// derived from the mutator object and the DWARF source location alone. It
/// therefore does not depend on the point's position in the discovered vector,
/// on which parallel worker found it, or on any container ordering, so the
/// slices produced by independent runs (or on a different machine) are
/// identical and their union is the unsliced set.
///
/// The same string is what Mull writes into the `.mull_mutants` section as the
/// mutant's global-variable name, so slice membership is externally checkable
/// against the emitted object file.
class SliceFilter : public MutationPointFilter {
public:
  /// `count` must be greater than 0 and `index` must be less than `count`;
  /// callers are expected to have validated this (Filters::enableSliceFilter
  /// does). A `count` of 1 is valid and skips nothing.
  SliceFilter(unsigned index, unsigned count);

  /// FNV-1a 64-bit over the raw bytes of `identity`. Spelled out rather than
  /// delegating to std::hash, whose result is implementation-defined and would
  /// make slice membership vary between standard libraries.
  static uint64_t fnv1a64(const std::string &identity);

  /// The slice `identity` belongs to when the set is split `count` ways.
  /// Returns 0 for count <= 1 (also guarding against division by zero).
  static unsigned sliceOf(const std::string &identity, unsigned count);

  bool shouldSkip(MutationPoint *point) override;
  std::string name() override;
  ~SliceFilter() override = default;

private:
  unsigned index;
  unsigned count;
};

} // namespace mull
