# MC1 example demo 2

This folder contains a bootable demo application for the MC1 computer.

## System requirements

* VRAM: 128 KB
* SD card reader

## Build & Run

Run `make` to build the demo, which will produce `out/demo2.elf`. Then copy the
file to a FAT formatted SD card (be sure to call the target file `MC1BOOT.EXE`),
e.g:

```bash
make
cp out/demo2.elf /path/to/sdcard/MC1BOOT.EXE
```

Insert the SD card into the MC1 computer and boot the computer.

