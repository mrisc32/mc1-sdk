# MC1 program loader

This folder contains a bootable program loader for the MC1 computer.

The loader allows the user to select which program to load from the SD card.
The list of loadable programs (ELF32 executables) are listed in a file in
the SD card root directory, called `PROGRAMS.LST` (each line is a filename).

## System requirements

* VRAM: 128 KB
* SD card reader
* Keyboard

## Build & Run

Run `make` to build the loader, which will produce `out/loader.elf`. Then copy the
file to a FAT formatted SD card (be sure to call the target file `MC1BOOT.EXE`),
e.g:

```bash
make
cp out/loader.elf /path/to/sdcard/MC1BOOT.EXE
```

Insert the SD card into the MC1 computer and boot the computer.

