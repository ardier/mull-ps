#pragma once

#include "mull/Config/ConfigurationOptions.h"
#include "mull/RegionClassifier.h"

#include <map>
#include <set>
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
/// Four files are written, all keyed on that identifier:
///
///   <prefix>.kept.txt         identifiers that survived every filter
///   <prefix>.filtered.txt     identifiers that did not
///   <prefix>.filtered-by.txt  identifier TAB name of the rejecting filter
///   <prefix>.mutants.tsv      one row per identifier, with a header: region,
///                             how many points carry it, how many survived,
///                             the enclosing functions, and the filter
///
/// The two `.txt` sets are deduplicated and sorted in byte order, so a run does
/// not depend on how work was distributed across parallel workers, and they can
/// be diffed directly against a `strings <obj> | sort -u` listing. The `.tsv`
/// carries the same identifiers in the same order plus everything that does not
/// fit a plain set -- in particular the point count, which is how a single
/// source location emitted into two enclosing functions becomes visible.
class MutantDump {
public:
  /// `pathPrefix` is a path prefix, not a directory.
  MutantDump(Diagnostics &diagnostics, std::string pathPrefix, const RegionsConfig &regions);

  /// Records the full pre-filter population. Everything the dump reports about
  /// a point that no filter touched comes from here, so this is called once,
  /// before the first filter runs.
  void recordPopulation(const std::vector<MutationPoint *> &points);

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

  /// Writes the four files. `kept` is the population that survived every
  /// filter, i.e. what the clone phase would work on. Returns false if any
  /// file could not be written; a diagnostic is emitted in that case.
  bool write(const std::vector<MutationPoint *> &kept);

  /// Name of the filter that rejected `identifier`, or the empty string if it
  /// was never rejected. On the rare occasion that one identifier is carried by
  /// several distinct points rejected by different filters, the names appear
  /// comma-separated.
  std::string rejectedBy(const std::string &identifier) const;

  /// The identifiers of `points`, deduplicated and sorted in byte order.
  static std::vector<std::string> sortedIdentifiers(const std::vector<MutationPoint *> &points);

  /// Writes one element of `lines` per line, each newline-terminated. Exposed
  /// so the file-level behaviour can be tested without a Module.
  static bool writeLines(Diagnostics &diagnostics, const std::string &path,
                         const std::vector<std::string> &lines);

private:
  /// Everything known about one identifier. Keyed by identifier rather than by
  /// point because that is what the emitted mutant keys are keyed by: two
  /// points sharing an identifier are two IR mutations that a single key
  /// activates together, which `points` makes visible.
  struct Record {
    std::string filePath;
    size_t line = 0;
    size_t points = 0;
    size_t keptPoints = 0;
    std::set<std::string> functions;
    std::string filteredBy;
  };

  Record &recordFor(MutationPoint *point);

  Diagnostics &diagnostics;
  std::string pathPrefix;
  RegionClassifier regions;
  std::map<std::string, Record> records;
};

} // namespace mull
