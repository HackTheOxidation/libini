# libini 

This repository contains the code for `libini`, a shared library for parsing .INI-files (see EBNF-diagram for complete description). 
This also served as exam project in the course SWAPK for the Autumn semester 2022.

## Getting Started

### Building

`scons` is used as the build system. To compile simply run:

```sh
scons
```

Note: the project uses features from the C++20 standard, so you need a compiler that implements that standard.
Compilation is tested with `g++` version 12.2.0 and 11.3.4.

### Installing

Simply use `scons` to build and install (default target is `/usr/lib` and `/usr/include`):

```sh
sudo scons install
```

### Using `libini` in your project

Once the library is installed, use the header file `<libini/libini.h>`, 
which includes the entire library under the `libini` namespace.
Then, add the `-llibini` (or `-lini`) flag to `CXXFLAGS` for your compiler of choice.
