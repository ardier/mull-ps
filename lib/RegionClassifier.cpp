#include "mull/RegionClassifier.h"

#include "mull/Diagnostics/Diagnostics.h"

#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/Regex.h>

#include <sstream>
#include <vector>

using namespace mull;
using namespace std::string_literals;

const char *mull::regionName(Region region) {
  switch (region) {
  case Region::Boilerplate:
    return "boilerplate";
  case Region::GeneratorSpecific:
    return "generator_specific";
  case Region::WrapperMetadata:
    return "wrapper_metadata";
  case Region::Unknown:
    break;
  }
  return "unknown";
}

RegionClassifier::RegionClassifier(Diagnostics &diagnostics, const RegionsConfig &config)
    : diagnostics(diagnostics), config(config) {}

Region RegionClassifier::classify(size_t line, const RegionBoundaries &boundaries) {
  if (line == 0) {
    return Region::Unknown;
  }
  if (line <= boundaries.boilerplateEnd) {
    return Region::Boilerplate;
  }
  if (line <= boundaries.generatorSpecificEnd) {
    return Region::GeneratorSpecific;
  }
  return Region::WrapperMetadata;
}

bool RegionClassifier::detectBoundaries(const std::string &contents,
                                        const std::string &namespaceCloseMarker,
                                        const std::string &functionAttrsMarker,
                                        RegionBoundaries &boundaries) {
  /// Zero-based indices of the marker lines, mirroring the analysis scripts so
  /// the arithmetic below can be compared against them line for line.
  std::vector<size_t> attrsLines;
  std::vector<size_t> namespaceLines;

  size_t index = 0;
  size_t start = 0;
  while (start <= contents.size()) {
    size_t end = contents.find('\n', start);
    const size_t length = (end == std::string::npos ? contents.size() : end) - start;
    /// Whole-line equality, not a substring search: the markers only delimit a
    /// region when they stand alone on their line.
    if (contents.compare(start, length, functionAttrsMarker) == 0) {
      attrsLines.push_back(index);
    } else if (contents.compare(start, length, namespaceCloseMarker) == 0) {
      namespaceLines.push_back(index);
    }
    if (end == std::string::npos) {
      break;
    }
    start = end + 1;
    index++;
  }

  if (attrsLines.size() < 3) {
    return false;
  }
  const size_t firstAttrs = attrsLines[0];
  const size_t argvAttrs = attrsLines[1];

  size_t lastNamespaceClose = 0;
  bool foundNamespaceClose = false;
  for (size_t line : namespaceLines) {
    if (line >= firstAttrs) {
      break;
    }
    lastNamespaceClose = line;
    foundNamespaceClose = true;
  }
  if (!foundNamespaceClose) {
    return false;
  }

  /// Zero-based index -> 1-based line number for the boilerplate end; the
  /// generator-specific end is the zero-based index of the `_argv` marker,
  /// which is the 1-based number of the line just before it.
  boundaries.boilerplateEnd = lastNamespaceClose + 1;
  boundaries.generatorSpecificEnd = argvAttrs;
  return true;
}

const RegionBoundaries *RegionClassifier::boundariesFor(const std::string &filePath) {
  if (!config.specified) {
    return nullptr;
  }

  auto cached = detected.find(filePath);
  if (cached != detected.end()) {
    return known[filePath] ? &cached->second : nullptr;
  }

  RegionBoundaries boundaries;
  bool ok = false;

  for (const RegionBoundaryConfig &entry : config.boundaries) {
    std::string error;
    llvm::Regex regex(entry.file);
    if (!regex.isValid(error)) {
      diagnostics.warning("regions: invalid file regex '"s + entry.file + "': " + error);
      continue;
    }
    if (!regex.match(filePath)) {
      continue;
    }
    boundaries.boilerplateEnd = entry.boilerplateEnd;
    boundaries.generatorSpecificEnd = entry.generatorSpecificEnd;
    ok = true;
    break;
  }

  if (!ok && config.autodetect) {
    auto buffer = llvm::MemoryBuffer::getFile(filePath);
    if (!buffer) {
      diagnostics.warning("regions: cannot read "s + filePath +
                          " to detect the region boundary: " + buffer.getError().message());
    } else {
      ok = detectBoundaries(buffer.get()->getBuffer().str(),
                            config.namespaceCloseMarker,
                            config.functionAttrsMarker,
                            boundaries);
      if (!ok) {
        diagnostics.warning("regions: no region boundary detected in "s + filePath +
                            "; its mutants are tagged 'unknown'.");
      }
    }
  }

  if (ok && boundaries.generatorSpecificEnd < boundaries.boilerplateEnd) {
    std::stringstream message;
    message << "regions: " << filePath << " has generatorSpecificEnd ("
            << boundaries.generatorSpecificEnd << ") below boilerplateEnd ("
            << boundaries.boilerplateEnd
            << "), which would make the generator-specific region empty and swallow it into "
               "'wrapper_metadata'. Tagging this file 'unknown' instead.";
    diagnostics.warning(message.str());
    ok = false;
  }

  if (ok) {
    std::stringstream message;
    message << "regions: " << filePath << ": boilerplate ends at line " << boundaries.boilerplateEnd
            << ", generator-specific ends at line " << boundaries.generatorSpecificEnd;
    diagnostics.info(message.str());
  }

  known[filePath] = ok;
  detected[filePath] = boundaries;
  return ok ? &detected[filePath] : nullptr;
}

Region RegionClassifier::classify(const std::string &filePath, size_t line) {
  const RegionBoundaries *boundaries = boundariesFor(filePath);
  if (boundaries == nullptr) {
    return Region::Unknown;
  }
  return classify(line, *boundaries);
}
