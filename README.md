# MC1 SDK

This is a software development kit (SDK) for the [MC1 computer](https://github.com/mrisc32/mc1).

## Usage

### Build & install the SDK

Prerequisites:

* Install CMake, Make and a compiler for your host system. For Ubuntu: `sudo apt install build-essential cmake`
* Install the [MRISC32 GNU toolchain](https://github.com/mrisc32/mrisc32-gnu-toolchain).

Installation:

```bash
make
make DESTDIR=/foo/bar install
```

### Building programs for MC1

```bash
mrisc32-elf-gcc -O2 -o program program.c         \
    -I/path/to/mc1-sdk/include                   \
    -L/path/to/mc1-sdk/lib                       \
    -mno-crt0 -lmc1crt0-app -lmc1 -T app-xram.ld
```

There are different ways that a program can be linked:

| CRT0 | Linker script | Description |
| --- | --- | --- |
| `-lmc1crt0-app` | `-T app-xram.ld` | Application that is loaded into XRAM |
| `-lmc1crt0-app` | `-T app-vram.ld` | Application that is loaded into VRAM |
| `-lmc1crt0-boot` | `-T boot-vram.ld` | Boot program that is loaded into VRAM |

## Documentation

* [Memory map](docs/memory_map.md)
* [Video logic](docs/video_logic.md)
* [Boot sequence](docs/boot_sequence.md)

## Resources

* [Tools](tools/README.md).
* C/C++ and assembler headers and libraries for interfacing the MC1 hardware.
* Linker scripts for various executable models.
* [Support](support/README.md).
