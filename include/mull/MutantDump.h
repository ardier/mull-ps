#pragma once

#include <map>
#include <string>
#include <vector>

namespace mull {

class Diagnostics;
class MutationPoint;

/// Writes the mutation-point population to disk at the one point in the
/// pipeline where it is both complete and cheap to obtain: after every
/// mutation filter has run, but before the clone phase.
///
/// The clone phase copies the enclosing function once per surviving mutation
/// point and keeps every copy, so its cost is the sum over functions of
/// (points in the function x function size). On the largest machine-generated
/// translation units that product is large enough to exhaust memory, and such
/// a file then yields no mutant list at all -- even though the list was fully
/// known before cloning started. Dumping here makes the population reportable
/// independently of whether instrumentation can finish.
///
/// The identifier written is `MutationPoint::getUserIdentifier()` verbatim,
/// i.e. `<mutatorId>:<absoluteFilePath>:<line>:<column>`. That is the same
/// string Mull uses as the name of the `.mull_mutants` global it emits for a
/// mutant, so a dump can be checked against a linked object file with
/// strings(1) instead of being taken on trust. The longer six-field encoding
/// stored *inside* that global (user identifier plus the end location) is
/// deliberately not used here: several mutation points can share one end
/// location and it would not round-trip against the mutant keys the runner
/// dispatches on.
///
/// Output is a set: identifiers are deduplicated and sorted in byte order, so
/// two runs of the same input produce the same file regardless of how work was
/// distributed across parallel workers.
class MutantDump {
public:
  /// `pathPrefix` is a path prefix, not a directory: the files written are
  /// `<pathPrefix>.kept.txt`, `<pathPrefix>.filtered.txt` and
  /// `<pathPrefix>.filtered-by.txt`.
  MutantDump(Diagnostics &diagnostics, std::string pathPrefix);

  /// Records the rejections made by one filter stage. `before` is the stage's
  /// input and `after` its output, so `before` minus `after` (by pointer
  /// identity) is exactly what `filterName` rejected.
  ///
  /// Filters run as a chain, each stage consuming the previous stage's output,
  /// so calling this once per stage attributes every rejected point to the
  /// first filter that rejected it, and the union of all stages' rejections
  /// telescopes to (pre-filter population minus final kept set).
  void recordFilterStage(const std::vector<MutationPoint *> &before,
                         const std::vector<MutationPoint *> &after, const std::string &filterName);

  /// Writes the three files. `kept` is the population that survived every
  /// filter, i.e. what the clone phase would work on. Returns false if any
  /// file could not be opened; a diagnostic is emitted in that case.
  bool write(const std::vector<MutationPoint *> &kept);

  /// Identifier -> name of the filter that rejected it. On the rare occasion
  /// that one identifier is carried by several distinct mutation points which
  /// were rejected by different filters, the names appear comma-separated.
  const std::map<std::string, std::string> &rejections() const;

  /// The identifiers of `points`, deduplicated and sorted in byte order.
  static std::vector<std::string> sortedIdentifiers(const std::vector<MutationPoint *> &points);

  /// Writes one element of `lines` per line, each newline-terminated. Exposed
  /// so the file-level behaviour can be tested without a Module.
  static bool writeLines(Diagnostics &diagnostics, const std::string &path,
                         const std::vector<std::string> &lines);

private:
  Diagnostics &diagnostics;
  std::string pathPrefix;
  std::map<std::string, std::string> rejectedBy;
};

} // namespace mull
