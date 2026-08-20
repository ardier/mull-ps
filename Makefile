# Build Mull against a system LLVM installation (Linux).
#
# The original macOS version of this file installed llvm@14 via Homebrew and
# read /usr/local/opt/llvm@14. On Linux the LLVM packages are installed by the
# distribution, so `make llvm` only verifies the expected version is present.
#
# Override LLVM_PREFIX to build against a different LLVM:
#     make LLVM_PREFIX=/usr/lib/llvm-16

LLVM_PREFIX     ?= /usr/lib/llvm-14
LLVM_CONFIG     := $(LLVM_PREFIX)/bin/llvm-config
BUILD_DIR       ?= build.dir
OUTPUT_DIR      ?= output
CMAKE_BUILD_TYPE?= Release
JOBS            ?= $(shell nproc)

# LLVM's CMake packages. Both llvm and clang are required; lld is optional and
# only present in some distributions.
CMAKE_PREFIX_PATH := $(LLVM_PREFIX)/lib/cmake/llvm;$(LLVM_PREFIX)/lib/cmake/clang

# Derived once LLVM is known, used to name the built binaries.
LLVM_VERSION_MAJOR = $(shell $(LLVM_CONFIG) --version 2>/dev/null | cut -d. -f1)

default: all

all: clean mull yml

help:
	@echo "Usage: make TARGET [LLVM_PREFIX=/usr/lib/llvm-14]"
	@echo ""
	@echo "Targets:"
	@echo "  llvm     Verify the expected LLVM installation is present"
	@echo "  mull     Configure and build Mull, stage binaries into $(OUTPUT_DIR)/"
	@echo "  yml      Generate $(OUTPUT_DIR)/mull.yml"
	@echo "  test     Run the libirm and Mull unit test suites"
	@echo "  clean    Remove $(BUILD_DIR)/ and $(OUTPUT_DIR)/"

# Verify LLVM rather than install it: on Linux this comes from the distro
# (Ubuntu: apt install llvm-14-dev libclang-14-dev clang-14).
llvm:
	@if [ ! -x "$(LLVM_CONFIG)" ]; then \
	  echo "error: $(LLVM_CONFIG) not found."; \
	  echo "       Install LLVM, or set LLVM_PREFIX to your installation."; \
	  echo "       Ubuntu/Debian: sudo apt install llvm-14-dev libclang-14-dev clang-14"; \
	  exit 1; \
	fi
	@for d in lib/cmake/llvm lib/cmake/clang; do \
	  if [ ! -d "$(LLVM_PREFIX)/$$d" ]; then \
	    echo "error: $(LLVM_PREFIX)/$$d not found (development package missing?)"; \
	    exit 1; \
	  fi; \
	done
	@echo "Found LLVM $$($(LLVM_CONFIG) --version) at $(LLVM_PREFIX)"

# vendor/libirm carries the Halide mutation operators and must match this
# checkout; the rest are stock third-party dependencies.
submodules:
	@git submodule update --init vendor/libirm vendor/json11 vendor/spdlog vendor/reproc

mull: llvm submodules
	@echo "Configuring Mull..."
	@cmake -S . -B $(BUILD_DIR) \
	  -DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) \
	  -DCMAKE_PREFIX_PATH="$(CMAKE_PREFIX_PATH)"
	@echo "Building Mull with $(JOBS) jobs..."
	@cmake --build $(BUILD_DIR) -j $(JOBS)
	@mkdir -p $(OUTPUT_DIR)
	@cp $(BUILD_DIR)/tools/mull-runner/mull-runner-$(LLVM_VERSION_MAJOR) $(OUTPUT_DIR)/
	@cp $(BUILD_DIR)/tools/mull-ir-frontend/mull-ir-frontend-$(LLVM_VERSION_MAJOR) $(OUTPUT_DIR)/
	@cp $(BUILD_DIR)/tools/mull-cxx-frontend/libmull-cxx-frontend-$(LLVM_VERSION_MAJOR).so $(OUTPUT_DIR)/
	@echo ""
	@echo "Build complete. Binaries in $(OUTPUT_DIR)/:"
	@echo "  mull-ir-frontend-$(LLVM_VERSION_MAJOR)        LLVM IR mutation pass (-fpass-plugin)"
	@echo "  libmull-cxx-frontend-$(LLVM_VERSION_MAJOR).so Clang AST mutation plugin (-fplugin)"
	@echo "  mull-runner-$(LLVM_VERSION_MAJOR)             mutant runner / reporter"

# Unit tests. libirm's suite is the regression barrier for the Halide
# mangled-name call-swap mechanism and is built from its own project, so it
# gets a separate configure.
test: llvm submodules
	@git -C vendor/libirm submodule update --init vendor/googletest
	@cmake -S vendor/libirm -B $(BUILD_DIR)/libirm-tests \
	  -DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) \
	  -DCMAKE_PREFIX_PATH="$(CMAKE_PREFIX_PATH)"
	@cmake --build $(BUILD_DIR)/libirm-tests -j $(JOBS) --target irm-tests
	@$(BUILD_DIR)/libirm-tests/tests/irm-tests

# Generate Mull config file by calling mull_config_generator.sh
# This target can take all or none of the flags below
# The script takes up to 5 arguments. If no arguments are passed, the script will print usage and set the timeout to 99999999 ms:
# 1. Use mutators to pass in mutation operators you would like to enable
# 2. Use timeout flag to set the timeout
# 3. Use excludePaths to pass in paths to exclude separated by ; in quotation “”
# 4. Use includePaths to pass in paths to include separated by ; in quotation “”
# 5. Use quiet to pass in false to enable additional logging

MUTATORS ?= ""
TIMEOUT ?= 99999999
EXCLUDEPATHS ?= ""
INCLUDEPATHS ?= ""
QUIET ?= false
yml:
	@echo "Generating Mull config file..."
	@./mull_config_generator.sh --mutators="$(MUTATORS)" --timeout="$(TIMEOUT)" --excludePaths="$(EXCLUDEPATHS)" --includePaths="$(INCLUDEPATHS)" --quiet="$(QUIET)"
	@echo "Generating Mull config file completed."

filter ?= --filter=""

clean:
	@rm -rf $(BUILD_DIR)/
	@rm -rf $(OUTPUT_DIR)/

.PHONY: default all help llvm submodules mull test yml clean
