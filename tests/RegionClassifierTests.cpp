#include "mull/RegionClassifier.h"

#include <gtest/gtest.h>
#include <mull/Diagnostics/Diagnostics.h>
#include <string>

using namespace mull;

namespace {

/// The shape of an emitted file, reduced to the parts the detector keys on: a
/// SIMD-emulation namespace that closes, then the pipeline entry point, then
/// the `_argv` wrapper, then the `_metadata` accessor.
std::string emittedFile() {
  return "// line 1\n"                     // 1
         "namespace {\n"                   // 2
         "struct CppVector {};\n"          // 3
         "}  // namespace\n"               // 4  <- an early close, not the last
         "namespace {\n"                   // 5
         "struct NativeVector {};\n"       // 6
         "}  // namespace\n"               // 7  <- boilerplate ends here
         "int halide_blur(void *a) {\n"    // 8
         "  return 0;\n"                   // 9
         "}\n"                             // 10
         "\n"                              // 11
         "HALIDE_FUNCTION_ATTRS\n"         // 12 <- first marker
         "int halide_blur_entry() {\n"     // 13
         "  return 0;\n"                   // 14
         "}\n"                             // 15
         "HALIDE_FUNCTION_ATTRS\n"         // 16 <- second marker, argv wrapper
         "int halide_blur_argv() {\n"      // 17
         "  return 0;\n"                   // 18
         "}\n"                             // 19
         "HALIDE_FUNCTION_ATTRS\n"         // 20 <- third marker, metadata
         "void *halide_blur_metadata();\n" // 21
      ;
}

const char *kNamespaceMarker = "}  // namespace";
const char *kAttrsMarker = "HALIDE_FUNCTION_ATTRS";

} // namespace

TEST(RegionClassifier, regionNamesAreTheEstablishedVocabulary) {
  ASSERT_STREQ(regionName(Region::Boilerplate), "boilerplate");
  ASSERT_STREQ(regionName(Region::GeneratorSpecific), "generator_specific");
  ASSERT_STREQ(regionName(Region::WrapperMetadata), "wrapper_metadata");
  ASSERT_STREQ(regionName(Region::Unknown), "unknown");
}

/// The two boundaries: the LAST namespace close before the first marker (not
/// the first one), and the SECOND marker.
TEST(RegionClassifier, detectBoundariesFindsTheMarkers) {
  RegionBoundaries boundaries;
  ASSERT_TRUE(RegionClassifier::detectBoundaries(
      emittedFile(), kNamespaceMarker, kAttrsMarker, boundaries));
  ASSERT_EQ(boundaries.boilerplateEnd, size_t(7));
  ASSERT_EQ(boundaries.generatorSpecificEnd, size_t(15));
}

/// Ordinary source has no such markers and must not be classified at all,
/// rather than being bucketed by an accidental boundary.
TEST(RegionClassifier, detectBoundariesRejectsFilesWithoutTheMarkers) {
  RegionBoundaries boundaries;
  ASSERT_FALSE(RegionClassifier::detectBoundaries(
      "int main() { return 0; }\n", kNamespaceMarker, kAttrsMarker, boundaries));

  /// Fewer than three markers is not an emitted file either: the entry point,
  /// the argv wrapper and the metadata accessor must all be present.
  ASSERT_FALSE(RegionClassifier::detectBoundaries("}  // namespace\nHALIDE_FUNCTION_ATTRS\n"
                                                  "HALIDE_FUNCTION_ATTRS\n",
                                                  kNamespaceMarker,
                                                  kAttrsMarker,
                                                  boundaries));

  /// Three markers but no namespace close before the first one.
  ASSERT_FALSE(RegionClassifier::detectBoundaries("HALIDE_FUNCTION_ATTRS\nHALIDE_FUNCTION_ATTRS\n"
                                                  "HALIDE_FUNCTION_ATTRS\n}  // namespace\n",
                                                  kNamespaceMarker,
                                                  kAttrsMarker,
                                                  boundaries));
}

/// Whole-line equality: a marker that appears as a substring of a longer line
/// is code, not a delimiter.
TEST(RegionClassifier, detectBoundariesIgnoresMarkersInsideLongerLines) {
  RegionBoundaries boundaries;
  const std::string contents = "}  // namespace foo\n"
                               "}  // namespace\n"
                               "  HALIDE_FUNCTION_ATTRS\n"
                               "HALIDE_FUNCTION_ATTRS\n"
                               "HALIDE_FUNCTION_ATTRS\n"
                               "HALIDE_FUNCTION_ATTRS\n";
  ASSERT_TRUE(
      RegionClassifier::detectBoundaries(contents, kNamespaceMarker, kAttrsMarker, boundaries));
  /// Only the bare `}  // namespace` on line 2 counts, and the indented marker
  /// on line 3 is not one, so the first real marker is line 4.
  ASSERT_EQ(boundaries.boilerplateEnd, size_t(2));
  ASSERT_EQ(boundaries.generatorSpecificEnd, size_t(4));
}

/// Inclusive at both ends, and line 0 (which is what Mull records when debug
/// information is missing) is never assigned a region.
TEST(RegionClassifier, classifyIsInclusiveAtBothBoundaries) {
  RegionBoundaries boundaries;
  boundaries.boilerplateEnd = 7;
  boundaries.generatorSpecificEnd = 15;

  ASSERT_EQ(RegionClassifier::classify(0, boundaries), Region::Unknown);
  ASSERT_EQ(RegionClassifier::classify(1, boundaries), Region::Boilerplate);
  ASSERT_EQ(RegionClassifier::classify(7, boundaries), Region::Boilerplate);
  ASSERT_EQ(RegionClassifier::classify(8, boundaries), Region::GeneratorSpecific);
  ASSERT_EQ(RegionClassifier::classify(15, boundaries), Region::GeneratorSpecific);
  ASSERT_EQ(RegionClassifier::classify(16, boundaries), Region::WrapperMetadata);
  ASSERT_EQ(RegionClassifier::classify(1000000, boundaries), Region::WrapperMetadata);
}

/// Off by default: with no `regions` key nothing is read and nothing is tagged.
TEST(RegionClassifier, isOffUntilConfigured) {
  Diagnostics diagnostics;
  diagnostics.makeQuiet();
  diagnostics.makeSilent();
  RegionsConfig config;
  RegionClassifier classifier(diagnostics, config);
  ASSERT_EQ(classifier.classify("/some/emitted.cpp", 42), Region::Unknown);
  ASSERT_EQ(classifier.boundariesFor("/some/emitted.cpp"), nullptr);
}

/// Explicit per-file boundaries, selected by regex. The boundary is per file
/// precisely because it differs between apps, so a second file that does not
/// match the regex must not inherit the first one's numbers.
TEST(RegionClassifier, explicitBoundariesApplyOnlyToMatchingFiles) {
  Diagnostics diagnostics;
  diagnostics.makeQuiet();
  diagnostics.makeSilent();

  RegionsConfig config;
  config.specified = true;
  RegionBoundaryConfig blur;
  blur.file = ".*blur.*";
  blur.boilerplateEnd = 3467;
  blur.generatorSpecificEnd = 4000;
  RegionBoundaryConfig harris;
  harris.file = ".*harris.*";
  harris.boilerplateEnd = 3501;
  harris.generatorSpecificEnd = 4200;
  config.boundaries = { blur, harris };

  RegionClassifier classifier(diagnostics, config);
  ASSERT_EQ(classifier.classify("/x/halide_blur.cpp", 3467), Region::Boilerplate);
  ASSERT_EQ(classifier.classify("/x/halide_blur.cpp", 3468), Region::GeneratorSpecific);
  ASSERT_EQ(classifier.classify("/x/harris.cpp", 3467), Region::Boilerplate);
  ASSERT_EQ(classifier.classify("/x/harris.cpp", 3502), Region::GeneratorSpecific);
  ASSERT_EQ(classifier.classify("/x/unrelated.cpp", 10), Region::Unknown);
}

/// A boundary pair that cannot describe a real file must not silently produce
/// an all-`wrapper_metadata` tagging.
TEST(RegionClassifier, rejectsInvertedBoundaries) {
  Diagnostics diagnostics;
  diagnostics.makeQuiet();
  diagnostics.makeSilent();

  RegionsConfig config;
  config.specified = true;
  RegionBoundaryConfig entry;
  entry.file = ".*";
  entry.boilerplateEnd = 500;
  entry.generatorSpecificEnd = 100;
  config.boundaries = { entry };

  RegionClassifier classifier(diagnostics, config);
  ASSERT_EQ(classifier.classify("/x/anything.cpp", 600), Region::Unknown);
}
