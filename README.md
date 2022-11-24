# Swapk Exam

This repository contains the code for the exam project in the course SWAPK in E22.

Group members:

- Tomas Hagenau Andersen: 201906721

## Getting Started

### Building

`scons` is used as the build system. To compile simply run:

```sh
scons
```

### Installing

Simply use `scons` to build and install (default target is `/usr/lib` and `/usr/include`):

```sh
sudo scons install
```

### Using `libini` in your project

Once the library is installed, use the header file `<libini::libini.h>`, 
which includes the entire library under the `libini` namespace.
Then, add the `-llibini` (or `-lini`) flag to `CXXFLAGS` for your compiler of choice.
