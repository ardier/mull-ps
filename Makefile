# Makefile to install LLVM 14 using Homebrew

# Default target
default: all

all: clean mull yml

# Help message
help:
	@echo "Usage: make TARGET"

# Setup LLVM@14
llvm:
	@echo "Installing LLVM 14..."
	@brew install llvm@14
	@echo "LLVM 14 installed successfully."

# Setup Mull
mull:
	@echo "Setting up Mull..."
	@(mkdir build.dir && cd build.dir && cmake -DCMAKE_PREFIX_PATH="/usr/local/opt/llvm@14/lib/cmake/llvm/;/usr/local/opt/llvm@14/lib/cmake/clang/" .. && make -j $(sysctl -n hw.ncpu) && cd .. && mkdir -p output && cp build.dir/tools/mull-runner/mull-runner-14 output && cp build.dir/tools/mull-ir-frontend/mull-ir-frontend-14 output)
	@echo "Setting up Mull completed. The Frontend and Runner Binaries could be found in the 'output' directory."



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
	@rm -rf build.dir/
	@rm -rf output/
	
