# MC1 tools

This directory contains various tools that help in the development of MC1
software.

## Building the tools

Some tools are written in C/C++ and need to be compiled for your host system
before use. Use CMake to build the tools, e.g. like this:

```bash
mkdir build
cd build
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=.. ../src
ninja
ninja install
```

## Tools

### mkmc1bootimage.py

Create an MC1 boot image that can be installed onto an SD card.

### png2mci

Convert a PNG image to an MCI image, which is suitable for the MC1 video
system. The tool changes the color format to one that is supported by MC1,
and can generate optimized palettes (including alpha), etc.

### raw2asm.py

Convert a raw binary file to MRISC32 assembler.

### vcpas.py

A VCP assembler (VCP = Video Control Program).