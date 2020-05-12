# Program Optimization Lab 202X

Implementing an LLVM analysis framework based upon the Seidl Program Optimization Lecture.

## Build

### Build against a system-wide installed LLVM
Install the LLVM packages from your distro's package manager, e.g. Ubuntu 20.04:

    # install the necessary LLVM packages
    sudo apt install cmake clang libclang-10-dev llvm-10-dev
    # now continue by building the project
    git clone https://versioncontrolseidl.in.tum.de/petter/llvm-abstractinterpretation.git
    cd llvm-abstractinterpretation
    git checkout origin/equalities
    mkdir build
    cd build
    cmake -G "Unix Makefiles" -DPATH_TO_LLVM=/usr/lib/llvm-10 ..
    make

You can do this, however the precompiled LLVM binaries come without symbol names, thus debugging
might be a little harder this way. Alterntively consider the following route:

### Build against custom downloaded LLVM Sources
Get the LLVM source code from [here](https://releases.llvm.org/download.html). Then get clang as well, into `llvm/tools`. Create a build directory somewhere, initialise CMake, and build. For example

    # From llvm-10.0.0-src, or whatever the version is now
    wget https://github.com/llvm/llvm-project/releases/download/llvmorg-10.0.0/llvm-10.0.0.src.tar.xz
    tar xf llvm-10.0.0.src.tar.xz
    # now also download clang
    cd llvm-10.0.0.src/tools
    wget https://github.com/llvm/llvm-project/releases/download/llvmorg-10.0.0/clang-10.0.0.src.tar.xz
    tar xf clang-10.0.0.src.tar.xz
    mv clang-10.0.0.src clang
    cd ../..
    # now continue by building LLVM
    mkdir llvm_build
    cd llvm_build
    # important: Don't forget to restrict to X86, otherwise prepare for a day of compiling
    cmake ../llvm-10.0.0-src -DLLVM_TARGETS_TO_BUILD=X86
    make -j4

On a 4 core i7-8550U this may take up to 3h.
The parallel make may run out of memory at the end. You can restart it sequentially by issuing another `make -j1`.


If there are errors regarding missing header files, you probably need to rebuild llvm.

## Author during Bachelor Thesis 2019/20

* Tim Gymnich

## Authors Lab Course WS 2019/20

* Florian Stamer
* Dmytro Yakymets

## Authors Lab Course WS 2018/19

* Ramona Brückl
* Philipp Czerner ([github](https://github.com/suyjuris/), [mail](mailto:philipp.czerner@nicze.de))
* Tim Gymnich
* Thomas Frank

## Authors Lab Course SS 2018
* Julian Erhard
* Jakob Gottfriedsen
* Peter Munch
* Alexander Roschlaub
* Michael B. Schwarz
