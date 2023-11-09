# Mutation

This set of instructions is intended to:
1.	Set up the correct version of LLVM for mutation using Mull on your system
2.	Pull, build and set up `Mull`

### Requirements
- A MacBook running Ventura or Sonoma (not tested on other systems)
- Homebrew
- Git SSH keys to be registered with both GitHub enterprise and GitHub

## Cloning
Make sure to clone using the recursive flag as follows:
 ```git clone https://github.com/mull-project/mull.git mull –recursive```

## Setup


### LLVM@14
1.	Run 
    1.	`make llvm` to set up llvm 
    2.	Alternatively, Run `brew install llvm@14`
2.	Make sure all other versions of llvm are not referenced from your environment.
3.	Follow the instructions to update !/.zshrc
4. Restart your terminal 

5. At this point you can run `make` to build mull binaries and the `mull.yml` within the `output` dir. Alternatively, you can follow the rest of this README to produce the same results.

## Setup Mull
6.	To setup mull 
    1.	Run make `make mull` to setup mull in the correct directory
    2.	Alternatively, 

        ```
        mkdir build.dir
        cd build.dir
        cmake -DCMAKE_PREFIX_PATH="/usr/local/opt/llvm@14/lib/cmake/llvm/;/usr/local/opt/llvm@14/lib/cmake/clang/" .. 
        make -j $(sysctl -n hw.ncpu)
        cd ..
        mkdir -p output
        cp build.dir/tools/mull-runner/mull-runner-14 output
        cp build.dir/tools/mull-ir-frontend/mull-ir-frontend-14 output
        ```

## Configure Mull

7.	Make sure you run `make yml` to generate a `mull.yml` file with a timeout of 99999999 ms at the root folder of the project
    1.	Use MUTATORS to pass in mutation operators you would like to enable
    2.	Use TIMEOUT flag to set the timeout
    3.	Use EXCLUDEPATHS to pass in paths to exclude separated by ; in quotation “”
    4.	Use INCLUDEPATHS to pass in paths to include separated by ; in quotation “”
    5.	Use QUIET to pass in false to enable additional logging

## Binaries + Mull.yml
If the above steps have been executed correctly, your output folder should look as follows:
```output
├── mull-ir-frontend-14
├── mull-runner-14
└── mull.yml
```
You can copy this folder to where your project exists and modify your project for mutation. 
### Note on mull.yml
This file should reside at the top directory of your mutated project.
