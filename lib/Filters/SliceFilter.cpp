#include "mull/Filters/SliceFilter.h"

#include "mull/MutationPoint.h"

#include <sstream>

using namespace mull;

SliceFilter::SliceFilter(unsigned index, unsigned count) : index(index), count(count) {}

uint64_t SliceFilter::fnv1a64(const std::string &identity) {
  /// Unsigned 64-bit multiplication wraps by definition, so this is portable
  /// across compilers, optimisation levels and machines.
  uint64_t h = 0xcbf29ce484222325ULL;
  for (unsigned char c : identity) {
    h ^= (uint64_t)c;
    h *= 0x100000001b3ULL;
  }
  return h;
}

unsigned SliceFilter::sliceOf(const std::string &identity, unsigned count) {
  if (count <= 1) {
    return 0;
  }
  return (unsigned)(fnv1a64(identity) % (uint64_t)count);
}

bool SliceFilter::shouldSkip(MutationPoint *point) {
  if (count <= 1) {
    return false;
  }
  return sliceOf(point->getUserIdentifier(), count) != index;
}

std::string SliceFilter::name() {
  std::stringstream message;
  message << "slice " << index << " of " << count;
  return message.str();
}
