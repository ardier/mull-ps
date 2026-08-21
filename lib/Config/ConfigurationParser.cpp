#include "mull/Config/Configuration.h"
#include "mull/Diagnostics/Diagnostics.h"

#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/YAMLTraits.h>

using namespace mull;
using namespace std::string_literals;

template <> struct llvm::yaml::ScalarEnumerationTraits<IDEDiagnosticsKind> {
  static void enumeration(llvm::yaml::IO &io, IDEDiagnosticsKind &value) {
    io.enumCase(value, "none", IDEDiagnosticsKind::None);
    io.enumCase(value, "killed", IDEDiagnosticsKind::Killed);
    io.enumCase(value, "survived", IDEDiagnosticsKind::Survived);
    io.enumCase(value, "all", IDEDiagnosticsKind::All);
  }
};

template <> struct llvm::yaml::MappingTraits<ParallelizationConfig> {
  static void mapping(llvm::yaml::IO &io, ParallelizationConfig &config) {
    io.mapOptional("workers", config.workers);
    io.mapOptional("executionWorkers", config.executionWorkers);
  }
};

template <> struct llvm::yaml::MappingTraits<SliceConfig> {
  static void mapping(llvm::yaml::IO &io, SliceConfig &config) {
    /// Reached only when the `slice:` key is actually present in the file,
    /// which is exactly what `specified` needs to record.
    config.specified = true;
    io.mapOptional("index", config.index);
    io.mapOptional("count", config.count);
  }
};

LLVM_YAML_IS_SEQUENCE_VECTOR(LineRangeConfig)

template <> struct llvm::yaml::MappingTraits<LineRangeConfig> {
  static void mapping(llvm::yaml::IO &io, LineRangeConfig &config) {
    io.mapOptional("file", config.file);
    io.mapOptional("from", config.from);
    /// Left at its default -- the end of the file -- when omitted.
    io.mapOptional("to", config.to);
  }
};

LLVM_YAML_IS_SEQUENCE_VECTOR(RegionBoundaryConfig)

template <> struct llvm::yaml::MappingTraits<RegionBoundaryConfig> {
  static void mapping(llvm::yaml::IO &io, RegionBoundaryConfig &config) {
    io.mapOptional("file", config.file);
    io.mapOptional("boilerplateEnd", config.boilerplateEnd);
    io.mapOptional("generatorSpecificEnd", config.generatorSpecificEnd);
  }
};

template <> struct llvm::yaml::MappingTraits<RegionsConfig> {
  static void mapping(llvm::yaml::IO &io, RegionsConfig &config) {
    /// Reached only when the `regions:` key is present, which is what
    /// `specified` records: without it, region tagging never reads a file and
    /// every record is tagged `unknown`.
    config.specified = true;
    io.mapOptional("autodetect", config.autodetect);
    io.mapOptional("namespaceCloseMarker", config.namespaceCloseMarker);
    io.mapOptional("functionAttrsMarker", config.functionAttrsMarker);
    io.mapOptional("boundaries", config.boundaries);
  }
};

template <> struct llvm::yaml::MappingTraits<DebugConfig> {
  static void mapping(llvm::yaml::IO &io, DebugConfig &config) {
    io.mapOptional("printIR", config.printIR);
    io.mapOptional("printIRAfter", config.printIRAfter);
    io.mapOptional("printIRBefore", config.printIRBefore);
    io.mapOptional("printIRToFile", config.printIRToFile);
    io.mapOptional("traceMutants", config.traceMutants);
    io.mapOptional("coverage", config.coverage);
    io.mapOptional("gitDiff", config.gitDiff);
    io.mapOptional("filters", config.filters);
    io.mapOptional("slowIRVerification", config.slowIRVerification);
  }
};

template <> struct llvm::yaml::MappingTraits<Configuration> {
  static void mapping(llvm::yaml::IO &io, Configuration &config) {
    io.mapOptional("debugEnabled", config.debugEnabled);
    io.mapOptional("quiet", config.quiet);
    io.mapOptional("silent", config.silent);
    io.mapOptional("captureTestOutput", config.captureTestOutput);
    io.mapOptional("captureMutantOutput", config.captureMutantOutput);
    io.mapOptional("includeNotCovered", config.includeNotCovered);
    io.mapOptional("timeout", config.timeout);
    io.mapOptional("mutators", config.mutators);
    io.mapOptional("ignoreMutators", config.ignoreMutators);
    io.mapOptional("parallelization", config.parallelization);
    io.mapOptional("compilationDatabasePath", config.compilationDatabasePath);
    io.mapOptional("compilerFlags", config.compilerFlags);
    io.mapOptional("junkDetectionDisabled", config.junkDetectionDisabled);
    io.mapOptional("gitDiffRef", config.gitDiffRef);
    io.mapOptional("gitProjectRoot", config.gitProjectRoot);
    io.mapOptional("includePaths", config.includePaths);
    io.mapOptional("excludePaths", config.excludePaths);
    io.mapOptional("slice", config.slice);
    io.mapOptional("lineRanges", config.lineRanges);
    io.mapOptional("dumpMutantsTo", config.dumpMutantsTo);
    io.mapOptional("dumpOnly", config.dumpOnly);
    io.mapOptional("regions", config.regions);
    io.mapOptional("debug", config.debug);
  }
};

std::string Configuration::findConfig(Diagnostics &diagnostics) {
  if (getenv("MULL_CONFIG") != nullptr) {
    return getenv("MULL_CONFIG");
  }
  llvm::SmallString<PATH_MAX> cwd;
  std::error_code ec = llvm::sys::fs::current_path(cwd);
  if (ec) {
    diagnostics.warning("Cannot find config: "s + ec.message());
    return {};
  }
  std::string currentDir = cwd.str().str();
  auto config = currentDir + "/mull.yml";
  if (llvm::sys::fs::exists(config)) {
    return config;
  }
  while (llvm::sys::path::has_parent_path(currentDir)) {
    currentDir = llvm::sys::path::parent_path(currentDir);
    config = currentDir + "/mull.yml";
    if (llvm::sys::fs::exists(config)) {
      return config;
    }
  }
  return {};
}

Configuration Configuration::loadFromDisk(Diagnostics &diagnostics, const std::string &path) {
  auto bufferOrError = llvm::MemoryBuffer::getFile(path);
  if (!bufferOrError) {
    diagnostics.warning("Cannot read config: "s + bufferOrError.getError().message());
    return {};
  }
  llvm::yaml::Input input(bufferOrError.get()->getMemBufferRef());
  Configuration configuration;
  input >> configuration;
  configuration.pathOnDisk = path;
  return configuration;
}
